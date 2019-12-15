/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2003-2004 E. Will, et al.
 * Copyright (C) 2006-2010 Atheme Project (http://atheme.org/)
 *
 * This file contains routines to handle the CService SET RESTRICTED command.
 */

#include <atheme.h>

static mowgli_patricia_t **cs_set_cmdtree = NULL;

static void
chanuser_sync(struct hook_chanuser_sync *hdata)
{
	struct chanuser *cu    = hdata->cu;
	bool             take  = hdata->take_prefixes;
	unsigned int     flags = hdata->flags;

	if (!cu)
		return;

	struct channel *chan = cu->chan;
	struct user    *u    = cu->user;
	struct mychan  *mc   = chan->mychan;

	return_if_fail(mc != NULL);

	if ((mc->flags & MC_RESTRICTED) && !(flags & CA_ALLPRIVS) && !has_priv_user(u, PRIV_JOIN_STAFFONLY))
	{
		// Stay on channel if this would empty it -- jilles
		if (chan->nummembers - chan->numsvcmembers == 1)
		{
			mc->flags |= MC_INHABIT;
			if (chan->numsvcmembers == 0)
				join(chan->name, chansvs.nick);
		}
		if (mc->mlock_on & CMODE_INVITE || chan->modes & CMODE_INVITE)
		{
			if (!(chan->modes & CMODE_INVITE))
				check_modes(mc, true);
			remove_banlike(chansvs.me->me, chan, ircd->invex_mchar, u);
			modestack_flush_channel(chan);
		}
		else
		{
			ban(chansvs.me->me, chan, u);
			remove_ban_exceptions(chansvs.me->me, chan, u);
		}

		if (try_kick(chansvs.me->me, chan, u, "You are not authorized to be on this channel"))
		{
			hdata->cu = NULL;
			return;
		}
	}
}

static void
cs_cmd_set_restricted(struct sourceinfo *si, int parc, char *parv[])
{
	struct mychan *mc;

	if (!(mc = mychan_find(parv[0])))
	{
		command_fail(si, fault_nosuch_target, STR_IS_NOT_REGISTERED, parv[0]);
		return;
	}

	if (!parv[1])
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SET RESTRICTED");
		return;
	}

	if (!chanacs_source_has_flag(mc, si, CA_SET))
	{
		command_fail(si, fault_noprivs, STR_NOT_AUTHORIZED);
		return;
	}

	if (!strcasecmp("ON", parv[1]))
	{
		if (MC_RESTRICTED & mc->flags)
		{
			command_fail(si, fault_nochange, _("The \2%s\2 flag is already set for channel \2%s\2."), "RESTRICTED", mc->name);
			return;
		}

		logcommand(si, CMDLOG_SET, "SET:RESTRICTED:ON: \2%s\2", mc->name);
		verbose(mc, "\2%s\2 enabled the RESTRICTED flag", get_source_name(si));

		mc->flags |= MC_RESTRICTED;

		command_success_nodata(si, _("The \2%s\2 flag has been set for channel \2%s\2."), "RESTRICTED", mc->name);
		return;
	}
	else if (!strcasecmp("OFF", parv[1]))
	{
		if (!(MC_RESTRICTED & mc->flags))
		{
			command_fail(si, fault_nochange, _("The \2%s\2 flag is not set for channel \2%s\2."), "RESTRICTED", mc->name);
			return;
		}

		logcommand(si, CMDLOG_SET, "SET:RESTRICTED:OFF: \2%s\2", mc->name);
		verbose(mc, "\2%s\2 disabled the RESTRICTED flag", get_source_name(si));

		mc->flags &= ~MC_RESTRICTED;

		command_success_nodata(si, _("The \2%s\2 flag has been removed for channel \2%s\2."), "RESTRICTED", mc->name);
		return;
	}
	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "SET RESTRICTED");
		return;
	}
}

static struct command cs_set_restricted = {
	.name           = "RESTRICTED",
	.desc           = N_("Restricts access to the channel to users on the access list. (Other users are kickbanned.)"),
	.access         = AC_NONE,
	.maxparc        = 2,
	.cmd            = &cs_cmd_set_restricted,
	.help           = { .path = "cservice/set_restricted" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_SYMBOL(m, cs_set_cmdtree, "chanserv/set_core", "cs_set_cmdtree")

	command_add(&cs_set_restricted, *cs_set_cmdtree);
	hook_add_first_chanuser_sync(&chanuser_sync);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	hook_del_chanuser_sync(&chanuser_sync);
	command_delete(&cs_set_restricted, *cs_set_cmdtree);
}

SIMPLE_DECLARE_MODULE_V1("chanserv/set_restricted", MODULE_UNLOAD_CAPABILITY_OK)
