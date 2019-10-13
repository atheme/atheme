/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005 William Pitcock, et al.
 *
 * This file contains code for the CService KICK functions.
 */

#include <atheme.h>

static mowgli_patricia_t **cs_clear_cmds = NULL;

static void
cs_cmd_clear_bans(struct sourceinfo *si, int parc, char *parv[])
{
	struct channel *c;
	struct mychan *mc = mychan_find(parv[0]);
	struct chanban *cb;
	mowgli_node_t *n, *tn;
	const char *item = parv[1], *p;
	unsigned int hits;

	if (item == NULL)
		item = "b";
	if (*item == '+' || *item == '-')
		item++;
	if (!strcmp(item, "*"))
		item = ircd->ban_like_modes;
	for (p = item; *p != '\0'; p++)
	{
		if (!strchr(ircd->ban_like_modes, *p))
		{
			command_fail(si, fault_badparams, _("Invalid mode; valid ones are %s."), ircd->ban_like_modes);
			return;
		}
	}
	if (*item == '\0')
	{
		command_fail(si, fault_badparams, _("Invalid mode; valid ones are %s."), ircd->ban_like_modes);
		return;
	}

	if (!mc)
	{
		command_fail(si, fault_nosuch_target, STR_IS_NOT_REGISTERED, parv[0]);
		return;
	}

	if (!(c = channel_find(parv[0])))
	{
		command_fail(si, fault_nosuch_target, STR_CHANNEL_IS_EMPTY, parv[0]);
		return;
	}

	if (!chanacs_source_has_flag(mc, si, CA_RECOVER))
	{
		command_fail(si, fault_noprivs, STR_NOT_AUTHORIZED);
		return;
	}

	if (metadata_find(mc, "private:close:closer"))
	{
		command_fail(si, fault_noprivs, STR_CHANNEL_IS_CLOSED, parv[0]);
		return;
	}

	hits = 0;
	MOWGLI_ITER_FOREACH_SAFE(n, tn, c->bans.head)
	{
		cb = n->data;
		if (!strchr(item, cb->type))
			continue;
		modestack_mode_param(chansvs.nick, c, MTYPE_DEL, cb->type, cb->mask);
		chanban_delete(cb);
		hits++;
	}

	if (hits > 4)
		command_add_flood(si, FLOOD_MODERATE);

	logcommand(si, CMDLOG_DO, "CLEAR:BANS: \2%s\2 on \2%s\2",
			item, mc->name);

	command_success_nodata(si, ngettext(N_("Cleared \2%s\2 modes on \2%s\2 (%u entry removed)."),
	                                    N_("Cleared \2%s\2 modes on \2%s\2 (%u entries removed)."),
	                                    hits), item, parv[0], hits);
}

static struct command cs_clear_bans = {
	.name           = "BANS",
	.desc           = N_("Clears bans or other lists of a channel."),
	.access         = AC_NONE,
	.maxparc        = 2,
	.cmd            = &cs_cmd_clear_bans,
	.help           = { .path = "cservice/clear_bans" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_SYMBOL(m, cs_clear_cmds, "chanserv/clear", "cs_clear_cmds")

	command_add(&cs_clear_bans, *cs_clear_cmds);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	command_delete(&cs_clear_bans, *cs_clear_cmds);
}

SIMPLE_DECLARE_MODULE_V1("chanserv/clear_bans", MODULE_UNLOAD_CAPABILITY_OK)
