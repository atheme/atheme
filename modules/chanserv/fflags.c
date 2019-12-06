/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2006 William Pitcock, et al.
 *
 * This file contains code for the CService FFLAGS functions.
 */

#include <atheme.h>

// FFLAGS <channel> <user> <flags>
static void
cs_cmd_fflags(struct sourceinfo *si, int parc, char *parv[])
{
	const char *channel = parv[0];
	const char *target = parv[1];
	const char *flagstr = parv[2];
	struct mychan *mc;
	struct myentity *mt;
	unsigned int addflags, removeflags;
	struct chanacs *ca;
	struct hook_channel_acl_req req;

	if (parc < 3)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "FFLAGS");
		command_fail(si, fault_needmoreparams, _("Syntax: FFLAGS <channel> <target> <flags>"));
		return;
	}

	mc = mychan_find(channel);
	if (!mc)
	{
		command_fail(si, fault_nosuch_target, STR_IS_NOT_REGISTERED, channel);
		return;
	}

	if (*flagstr == '+' || *flagstr == '-' || *flagstr == '=')
	{
		flags_make_bitmasks(flagstr, &addflags, &removeflags);
		if (addflags == 0 && removeflags == 0)
		{
			command_fail(si, fault_badparams, _("No valid flags given; use \2/msg %s HELP FLAGS\2 "
			                                    "for a list"), chansvs.me->disp);
			return;
		}
	}
	else
	{
		addflags = get_template_flags(mc, flagstr);
		if (addflags == 0)
		{
			// Hack -- jilles
			if (*target == '+' || *target == '-' || *target == '=')
				command_fail(si, fault_badparams, _("Usage: FFLAGS %s <target> <flags>"), mc->name);
			else
				command_fail(si, fault_badparams, _("Invalid template name given; use \2/msg %s "
				                                    "TEMPLATE %s\2 for a list"), chansvs.me->disp,
				                                    mc->name);
			return;
		}
		removeflags = ca_all & ~addflags;
	}

	if (!validhostmask(target))
	{
		if (!(mt = myentity_find_ext(target)))
		{
			command_fail(si, fault_nosuch_target, STR_IS_NOT_REGISTERED, target);
			return;
		}
		target = mt->name;

		ca = chanacs_open(mc, mt, NULL, true, entity(si->smu));

		// XXX this should be more like flags.c
		if (removeflags & CA_FLAGS)
		{
			removeflags |= CA_FOUNDER;
			addflags &= ~CA_FOUNDER;
		}
		else if (addflags & CA_FOUNDER)
		{
			if (!myentity_allow_foundership(mt))
			{
				command_fail(si, fault_toomany, _("\2%s\2 cannot take foundership of a channel."), mt->name);
				chanacs_close(ca);
				return;
			}

			addflags |= CA_FLAGS;
			removeflags &= ~CA_FLAGS;
		}

		if (is_founder(mc, mt) && removeflags & CA_FOUNDER && mychan_num_founders(mc) == 1)
		{
			command_fail(si, fault_noprivs, _("You may not remove the last founder."));
			return;
		}

		req.ca = ca;
		req.oldlevel = ca->level;

		if (!chanacs_modify(ca, &addflags, &removeflags, ca_all, si->smu))
		{
			// this shouldn't happen
			command_fail(si, fault_noprivs, _("You are not allowed to set \2%s\2 on \2%s\2 in \2%s\2."), bitmask_to_flags2(addflags, removeflags), mt->name, mc->name);
			chanacs_close(ca);
			return;
		}

		req.newlevel = ca->level;

		hook_call_channel_acl_change(&req);
		chanacs_close(ca);
	}
	else
	{
		if (addflags & CA_FOUNDER)
		{
			command_fail(si, fault_badparams, _("You may not set founder status on a hostmask."));
			return;
		}

		ca = chanacs_open(mc, NULL, target, true, entity(si->smu));

		req.ca = ca;
		req.oldlevel = ca->level;

		if (!chanacs_modify(ca, &addflags, &removeflags, ca_all, si->smu))
		{
			// this shouldn't happen
			command_fail(si, fault_noprivs, _("You are not allowed to set \2%s\2 on \2%s\2 in \2%s\2."), bitmask_to_flags2(addflags, removeflags), target, mc->name);
			chanacs_close(ca);
			return;
		}

		req.newlevel = ca->level;

		hook_call_channel_acl_change(&req);
		chanacs_close(ca);
	}

	if ((addflags | removeflags) == 0)
	{
		command_fail(si, fault_nochange, _("Channel access to \2%s\2 for \2%s\2 unchanged."), channel, target);
		return;
	}
	flagstr = bitmask_to_flags2(addflags, removeflags);
	wallops("\2%s\2 is forcing flags change \2%s\2 on \2%s\2 in \2%s\2.", get_oper_name(si), flagstr, target, mc->name);
	command_success_nodata(si, _("Flags \2%s\2 were set on \2%s\2 in \2%s\2."), flagstr, target, channel);
	logcommand(si, CMDLOG_ADMIN, "FFLAGS: \2%s\2 \2%s\2 \2%s\2", mc->name, target, flagstr);
	verbose(mc, "\2%s\2 forced flags change \2%s\2 on \2%s\2.", get_source_name(si), flagstr, target);
}

static struct command cs_fflags = {
	.name           = "FFLAGS",
	.desc           = N_("Forces a flags change on a channel."),
	.access         = PRIV_CHAN_ADMIN,
	.maxparc        = 3,
	.cmd            = &cs_cmd_fflags,
	.help           = { .path = "cservice/fflags" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "chanserv/main")

        service_named_bind_command("chanserv", &cs_fflags);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("chanserv", &cs_fflags);
}

SIMPLE_DECLARE_MODULE_V1("chanserv/fflags", MODULE_UNLOAD_CAPABILITY_OK)
