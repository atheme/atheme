/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2011 William Pitcock <nenolod@dereferenced.org>
 *
 * main.c - rpgserv main() routine.
 * based on elly's rpgserv for atheme-6.x --nenolod
 */

#include <atheme.h>
#include "prettyprint.h"

static void
rs_cmd_info(struct sourceinfo *si, int parc, char *parv[])
{
	struct mychan *mc;
	struct metadata *md;

	if (parc < 1)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "INFO");
		command_fail(si, fault_needmoreparams, _("Syntax: INFO <channel>"));
		return;
	}

	mc = mychan_find(parv[0]);
	if (!mc)
	{
		command_fail(si, fault_nosuch_target, STR_IS_NOT_REGISTERED, parv[0]);
		return;
	}

	if (!metadata_find(mc, "private:rpgserv:enabled"))
	{
		command_fail(si, fault_nosuch_target, _("Channel \2%s\2 does not have RPGServ enabled."), parv[0]);
		return;
	}

	command_success_nodata(si, _("Information on \2%s\2:"), parv[0]);
	md = metadata_find(mc, "private:rpgserv:genre");
	command_success_nodata(si, _("Genre    : %s"), rs_prettyprint_keywords(md, genre_keys, genre_names, ARRAY_SIZE(genre_keys)));
	md = metadata_find(mc, "private:rpgserv:period");
	command_success_nodata(si, _("Period   : %s"), rs_prettyprint_keywords(md, period_keys, period_names, ARRAY_SIZE(period_keys)));
	md = metadata_find(mc, "private:rpgserv:ruleset");
	command_success_nodata(si, _("Ruleset  : %s"), rs_prettyprint_keywords(md, ruleset_keys, ruleset_names, ARRAY_SIZE(ruleset_keys)));
	md = metadata_find(mc, "private:rpgserv:rating");
	command_success_nodata(si, _("Rating   : %s"), rs_prettyprint_keywords(md, rating_keys, rating_names, ARRAY_SIZE(rating_keys)));
	md = metadata_find(mc, "private:rpgserv:system");
	command_success_nodata(si, _("System   : %s"), rs_prettyprint_keywords(md, system_keys, system_names, ARRAY_SIZE(system_keys)));
	md = metadata_find(mc, "private:rpgserv:setting");
	command_success_nodata(si, _("Setting  : %s"), md ? md->value : "<none>");
	md = metadata_find(mc, "private:rpgserv:storyline");
	command_success_nodata(si, _("Storyline: %s"), md ? md->value : "<none>");
	md = metadata_find(mc, "private:rpgserv:summary");
	command_success_nodata(si, _("Summary  : %s"), md ? md->value : "<none>");
	command_success_nodata(si, _("\2*** End of Info ***\2"));

	logcommand(si, CMDLOG_GET, "RPGSERV:INFO: \2%s\2", mc->name);
}

static struct command rs_info = {
	.name           = "INFO",
	.desc           = N_("Displays info for a particular game."),
	.access         = AC_NONE,
	.maxparc        = 1,
	.cmd            = &rs_cmd_info,
	.help           = { .path = "rpgserv/info" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "rpgserv/main")

	service_named_bind_command("rpgserv", &rs_info);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("rpgserv", &rs_info);
}

SIMPLE_DECLARE_MODULE_V1("rpgserv/info", MODULE_UNLOAD_CAPABILITY_OK)
