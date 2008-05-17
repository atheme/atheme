/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService REGISTER function.
 *
 * $Id: register.c 8425 2007-06-09 21:15:26Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/register", FALSE, _modinit, _moddeinit,
	"$Id: register.c 8425 2007-06-09 21:15:26Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_register(sourceinfo_t *si, int parc, char *parv[]);

command_t cs_register = { "REGISTER", N_("Registers a channel."),
                           AC_NONE, 3, cs_cmd_register };

list_t *cs_cmdtree;
list_t *cs_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(cs_cmdtree, "chanserv/main", "cs_cmdtree");
	MODULE_USE_SYMBOL(cs_helptree, "chanserv/main", "cs_helptree");

        command_add(&cs_register, cs_cmdtree);
	help_addentry(cs_helptree, "REGISTER", "help/cservice/register", NULL);
}

void _moddeinit()
{
	command_delete(&cs_register, cs_cmdtree);
	help_delentry(cs_helptree, "REGISTER");
}

static void cs_cmd_register(sourceinfo_t *si, int parc, char *parv[])
{
	channel_t *c;
	chanuser_t *cu;
	mychan_t *mc;
	char *name = parv[0];
	char str[21];
	hook_channel_register_check_t hdatac;
	hook_channel_req_t hdata;

	if (!name)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "REGISTER");
		command_fail(si, fault_needmoreparams, _("To register a channel: REGISTER <#channel>"));
		return;
	}

	if (*name != '#')
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "REGISTER");
		command_fail(si, fault_badparams, _("Syntax: REGISTER <#channel>"));
		return;
	}

	/* make sure they're logged in */
	if (!si->smu)
	{
		command_fail(si, fault_noprivs, _("You are not logged in."));
		return;
	}

	if (si->smu->flags & MU_WAITAUTH)
	{
		command_fail(si, fault_notverified, _("You need to verify your email address before you may register channels."));
		return;
	}
	
	/* make sure it isn't already registered */
	if ((mc = mychan_find(name)))
	{
		if (!use_channel_private || !(mc->flags & MC_PRIVATE))
			command_fail(si, fault_alreadyexists, _("\2%s\2 is already registered to \2%s\2."), mc->name, mychan_founder_names(mc));
		else
			command_fail(si, fault_alreadyexists, _("\2%s\2 is already registered."), mc->name);
		return;
	}

	/* make sure the channel exists */
	if (!(c = channel_find(name)))
	{
		command_fail(si, fault_nosuch_target, _("The channel \2%s\2 must exist in order to register it."), name);
		return;
	}

	/* make sure they're in it */
	if (!(cu = chanuser_find(c, si->su)))
	{
		command_fail(si, fault_noprivs, _("You must be in \2%s\2 in order to register it."), name);
		return;
	}

	/* make sure they're opped */
	if (!(CMODE_OP & cu->modes))
	{
		command_fail(si, fault_noprivs, _("You must be a channel operator in \2%s\2 in order to register it."), name);
		return;
	}

	hdatac.si = si;
	hdatac.name = name;
	hdatac.chan = c;
	hdatac.approved = 0;
	hook_call_event("channel_can_register", &hdatac);
	if (hdatac.approved != 0)
		return;

	if ((myuser_num_channels(si->smu) >= me.maxchans) && !has_priv(si, PRIV_REG_NOLIMIT))
	{
		command_fail(si, fault_toomany, _("You have too many channels registered."));
		return;
	}

	logcommand(si, CMDLOG_REGISTER, "%s REGISTER", name);
	snoop("REGISTER: \2%s\2 to \2%s\2", name, get_oper_name(si));

	mc = mychan_add(name);
	mc->registered = CURRTIME;
	mc->used = CURRTIME;
	mc->mlock_on |= (CMODE_NOEXT | CMODE_TOPIC);
	if (c->limit == 0)
		mc->mlock_off |= CMODE_LIMIT;
	if (c->key == NULL)
		mc->mlock_off |= CMODE_KEY;
	mc->flags |= config_options.defcflags;

	chanacs_add(mc, si->smu, CA_INITIAL & ca_all, CURRTIME);

	if (c->ts > 0)
	{
		snprintf(str, sizeof str, "%lu", (unsigned long)c->ts);
		metadata_add(mc, METADATA_CHANNEL, "private:channelts", str);
	}

	if (chansvs.deftemplates != NULL && *chansvs.deftemplates != '\0')
		metadata_add(mc, METADATA_CHANNEL, "private:templates",
				chansvs.deftemplates);

	command_success_nodata(si, _("\2%s\2 is now registered to \2%s\2."), mc->name, si->smu->name);

	hdata.si = si;
	hdata.mc = mc;
	hook_call_event("channel_register", &hdata);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
