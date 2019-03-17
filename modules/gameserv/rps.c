/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2007 William Pitcock <nenolod@nenolod.net>, et al.
 *
 * Rock Paper Scissors
 */

#include <atheme.h>
#include "gameserv_common.h"

static void
command_rps(struct sourceinfo *si, int parc, char *parv[])
{
	struct mychan *mc;
	static const char *rps_responses[3] = {
		N_("Rock"),
		N_("Paper"),
		N_("Scissors")
	};

	if (!gs_do_parameters(si, &parc, &parv, &mc))
		return;

	gs_command_report(si, "%s", _(rps_responses[rand() % 3]));
}

static struct command cmd_rps = {
	.name           = "RPS",
	.desc           = N_("Rock Paper Scissors."),
	.access         = AC_NONE,
	.maxparc        = 2,
	.cmd            = &command_rps,
	.help           = { .path = "gameserv/rps" },
};

static void
mod_init(struct module ATHEME_VATTR_UNUSED *const restrict m)
{
	service_named_bind_command("gameserv", &cmd_rps);

	service_named_bind_command("chanserv", &cmd_rps);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("gameserv", &cmd_rps);

	service_named_unbind_command("chanserv", &cmd_rps);
}

SIMPLE_DECLARE_MODULE_V1("gameserv/rps", MODULE_UNLOAD_CAPABILITY_OK)
