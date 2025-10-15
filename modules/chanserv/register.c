/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005 William Pitcock, et al.
 *
 * This file contains code for the CService REGISTER function.
 */

#include <atheme.h>
#include "chanserv.h"

static unsigned int ratelimit_count = 0;
static time_t ratelimit_firsttime = 0;

static void
cs_cmd_register(struct sourceinfo *si, int parc, char *parv[])
{
	struct channel *c;
	struct chanuser *cu;
	struct mychan *mc;
	char *name = parv[0];
	char str[21];
	struct hook_channel_register_check hdatac;
	struct hook_channel_req hdata;
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

	if (si->smu->flags & MU_WAITAUTH)
	{
		command_fail(si, fault_notverified, STR_EMAIL_NOT_VERIFIED);
		return;
	}

	// make sure it isn't already registered
	if ((mc = mychan_find(name)))
	{
		if (! (mc->flags & MC_PRIVATE))
			command_fail(si, fault_alreadyexists, _("\2%s\2 is already registered to \2%s\2."), mc->name, mychan_founder_names(mc));
		else
			command_fail(si, fault_alreadyexists, _("\2%s\2 is already registered."), mc->name);
		return;
	}

	// make sure the channel exists
	if (!(c = channel_find(name)))
	{
		command_fail(si, fault_nosuch_target, _("The channel \2%s\2 must exist in order to register it."), name);
		return;
	}

	// make sure they're in it
	if (!(cu = chanuser_find(c, si->su)))
	{
		command_fail(si, fault_noprivs, _("You must be in \2%s\2 in order to register it."), name);
		return;
	}

	// make sure they're opped (or protected/owner on unreal/inspircd)
	if (!((CSTATUS_OP | CSTATUS_PROTECT | CSTATUS_OWNER) & cu->modes))
	{
		command_fail(si, fault_noprivs, _("You must be a channel operator in \2%s\2 in order to register it."), name);
		return;
	}

	if (metadata_find(si->smu, "private:restrict:setter"))
	{
		command_fail(si, fault_noprivs, _("You have been restricted from registering channels by network staff."));
		return;
	}

	if ((unsigned int)(CURRTIME - ratelimit_firsttime) > config_options.ratelimit_period)
	{
		ratelimit_count = 0;
		ratelimit_firsttime = CURRTIME;
	}

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
	mc->flags |= config_options.defcflags;

	if (chansvs.default_mlock)
	{
		unsigned int dir = MTYPE_ADD;

		for (const char *mchar = chansvs.default_mlock; *mchar; mchar++)
		{
			switch (*mchar)
			{
				case '+':
					dir = MTYPE_ADD;
					break;
				case '-':
					dir = MTYPE_DEL;
					break;

				/* 'k' and 'l' are not in mode_list[] and so mode_to_flag() does not parse them
				 *
				 * Ignore +k if there is currently no channel key; ignore -k if there is
				 * Ignore +l if there is currently no channel limit; ignore -l if there is
				 *
				 * Reason being we wouldn't want to remove an active channel key or limit on
				 * registration if the admin has put -kl in default_mlock, and we can't enforce
				 * a key or limit where none exists either
				 *
				 * -- amdj
				 */
				case 'k':
					if (dir == MTYPE_ADD && c->key != NULL)
						mc->mlock_on |= CMODE_KEY;
					if (dir == MTYPE_DEL && c->key == NULL)
						mc->mlock_off |= CMODE_KEY;
					break;
				case 'l':
					if (dir == MTYPE_ADD && c->limit != 0)
						mc->mlock_on |= CMODE_LIMIT;
					if (dir == MTYPE_DEL && c->limit == 0)
						mc->mlock_off |= CMODE_LIMIT;
					break;

				default:
					if (dir == MTYPE_ADD)
						mc->mlock_on |= mode_to_flag(*mchar);
					else
						mc->mlock_off |= mode_to_flag(*mchar);
					break;
			}
		}
	}
	else
	{
		mc->mlock_on |= (CMODE_NOEXT | CMODE_TOPIC);
	}

	mc->mlock_off &= ~mc->mlock_on;

	chanacs_add(mc, entity(si->smu), custom_founder_check(), CURRTIME, entity(si->smu));

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

	// Allow the hook to override this.
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

static struct command cs_register = {
	.name           = "REGISTER",
	.desc           = N_("Registers a channel."),
	.access         = AC_AUTHENTICATED,
	.maxparc        = 3,
	.cmd            = &cs_cmd_register,
	.help           = { .path = "cservice/register" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "chanserv/main")

        service_named_bind_command("chanserv", &cs_register);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("chanserv", &cs_register);
}

SIMPLE_DECLARE_MODULE_V1("chanserv/register", MODULE_UNLOAD_CAPABILITY_OK)
