/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService REGISTER function.
 *
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/register", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

unsigned int ratelimit_count = 0;
time_t ratelimit_firsttime = 0;

static void cs_cmd_register(sourceinfo_t *si, int parc, char *parv[]);

command_t cs_register = { "REGISTER", N_("Registers a channel."),
                           AC_NONE, 3, cs_cmd_register, { .path = "cservice/register" } };

void _modinit(module_t *m)
{
        service_named_bind_command("chanserv", &cs_register);
}

void _moddeinit()
{
	service_named_unbind_command("chanserv", &cs_register);
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
	unsigned int fl;

	/* This command is not useful on registered channels, ignore it if
	 * it is a fantasy command so users can program bots to react on
	 * it without interference from ChanServ.
	 */
	if (si->c != NULL)
		return;

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
	if (!(CSTATUS_OP & cu->modes))
	{
		command_fail(si, fault_noprivs, _("You must be a channel operator in \2%s\2 in order to register it."), name);
		return;
	}

	if ((unsigned int)(CURRTIME - ratelimit_firsttime) > config_options.ratelimit_period)
		ratelimit_count = 0, ratelimit_firsttime = CURRTIME;

	if (ratelimit_count > config_options.ratelimit_uses && !has_priv(si, PRIV_FLOOD))
	{
		command_fail(si, fault_toomany, _("The system is currently too busy to process your registration, please try again later."));
		slog(LG_INFO, "CHANSERV:REGISTER:THROTTLED: \2%s\2 by \2%s\2", name, entity(si->smu)->name);
		return;
	}

	hdatac.si = si;
	hdatac.name = name;
	hdatac.chan = c;
	hdatac.approved = 0;
	hook_call_channel_can_register(&hdatac);
	if (hdatac.approved != 0)
		return;

	if (!myentity_can_register_channel(entity(si->smu)))
	{
		command_fail(si, fault_toomany, _("You have too many channels registered."));
		return;
	}

	if (config_options.ratelimit_uses && config_options.ratelimit_period)
		ratelimit_count++;

	logcommand(si, CMDLOG_REGISTER, "REGISTER: \2%s\2", name);

	mc = mychan_add(name);
	mc->registered = CURRTIME;
	mc->used = CURRTIME;
	mc->mlock_on |= (CMODE_NOEXT | CMODE_TOPIC);
	if (c->limit == 0)
		mc->mlock_off |= CMODE_LIMIT;
	if (c->key == NULL)
		mc->mlock_off |= CMODE_KEY;
	mc->flags |= config_options.defcflags;

	chanacs_add(mc, entity(si->smu), CA_INITIAL & ca_all, CURRTIME);

	if (c->ts > 0)
	{
		snprintf(str, sizeof str, "%lu", (unsigned long)c->ts);
		metadata_add(mc, "private:channelts", str);
	}

	if (chansvs.deftemplates != NULL && *chansvs.deftemplates != '\0')
		metadata_add(mc, "private:templates",
				chansvs.deftemplates);

	command_success_nodata(si, _("\2%s\2 is now registered to \2%s\2."), mc->name, entity(si->smu)->name);

	hdata.si = si;
	hdata.mc = mc;
	hook_call_channel_register(&hdata);
	/* Allow the hook to override this. */
	fl = chanacs_source_flags(mc, si);
	cu = chanuser_find(mc->chan, si->su);
	if (cu == NULL)
		;
	else if (ircd->uses_owner && fl & CA_USEOWNER && fl & CA_AUTOOP &&
			!(cu->modes & CSTATUS_OWNER))
	{
		modestack_mode_param(si->service->nick, mc->chan, MTYPE_ADD,
				ircd->owner_mchar[1], CLIENT_NAME(si->su));
		cu->modes |= CSTATUS_OWNER;
	}
	else if (ircd->uses_protect && fl & CA_USEPROTECT && fl & CA_AUTOOP &&
			!(cu->modes & CSTATUS_PROTECT))
	{
		modestack_mode_param(si->service->nick, mc->chan, MTYPE_ADD,
				ircd->protect_mchar[1], CLIENT_NAME(si->su));
		cu->modes |= CSTATUS_PROTECT;
	}
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
