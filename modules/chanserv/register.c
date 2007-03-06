/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService REGISTER function.
 *
 * $Id: register.c 7855 2007-03-06 00:43:08Z pippijn $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/register", FALSE, _modinit, _moddeinit,
	"$Id: register.c 7855 2007-03-06 00:43:08Z pippijn $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_register(sourceinfo_t *si, int parc, char *parv[]);

command_t cs_register = { "REGISTER", N_("Registers a channel."),
                           AC_NONE, 1, cs_cmd_register };

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
	uint32_t tcnt;
	char str[21];
	node_t *n;
	chanacs_t *ca;

	if (!name)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "REGISTER");
		command_fail(si, fault_needmoreparams, "To register a channel: REGISTER <#channel>");
		return;
	}

	if (*name != '#')
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "REGISTER");
		command_fail(si, fault_badparams, "Syntax: REGISTER <#channel>");
		return;
	}

	/* make sure they're logged in */
	if (!si->smu)
	{
		command_fail(si, fault_noprivs, "You are not logged in.");
		return;
	}

	if (si->smu->flags & MU_WAITAUTH)
	{
		command_fail(si, fault_notverified, "You need to verify your email address before you may register channels.");
		return;
	}
	
	/* make sure it isn't already registered */
	if ((mc = mychan_find(name)))
	{
		command_fail(si, fault_alreadyexists, "\2%s\2 is already registered to \2%s\2.", mc->name, mc->founder->name);
		return;
	}

	/* make sure the channel exists */
	if (!(c = channel_find(name)))
	{
		command_fail(si, fault_nosuch_target, "The channel \2%s\2 must exist in order to register it.", name);
		return;
	}

	/* make sure they're in it */
	if (!(cu = chanuser_find(c, si->su)))
	{
		command_fail(si, fault_noprivs, "You must be in \2%s\2 in order to register it.", name);
		return;
	}

	/* make sure they're opped */
	if (!(CMODE_OP & cu->modes))
	{
		command_fail(si, fault_noprivs, "You must be a channel operator in \2%s\2 in order to " "register it.", name);
		return;
	}

	/* make sure they're within limits */
	tcnt = 0;
	LIST_FOREACH(n, si->smu->chanacs.head)
	{
		ca = n->data;
		if (is_founder(ca->mychan, si->smu))
			tcnt++;
	}

	if ((tcnt >= me.maxchans) && !has_priv(si, PRIV_REG_NOLIMIT))
	{
		command_fail(si, fault_toomany, "You have too many channels registered.");
		return;
	}

	logcommand(si, CMDLOG_REGISTER, "%s REGISTER", name);
	snoop("REGISTER: \2%s\2 to \2%s\2 as \2%s\2", name, get_oper_name(si), si->smu->name);

	mc = mychan_add(name);
	mc->founder = si->smu;
	mc->registered = CURRTIME;
	mc->used = CURRTIME;
	mc->mlock_on |= (CMODE_NOEXT | CMODE_TOPIC);
	if (c->limit == 0)
		mc->mlock_off |= CMODE_LIMIT;
	if (c->key == NULL)
		mc->mlock_off |= CMODE_KEY;
	mc->flags |= config_options.defcflags;

	chanacs_add(mc, si->smu, CA_INITIAL & ca_all);

	if (c->ts > 0)
	{
		snprintf(str, sizeof str, "%lu", (unsigned long)c->ts);
		metadata_add(mc, METADATA_CHANNEL, "private:channelts", str);
	}

	command_success_nodata(si, "\2%s\2 is now registered to \2%s\2.", mc->name, mc->founder->name);

	hook_call_event("channel_register", mc);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
