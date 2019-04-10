/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2007 William Pitcock <nenolod@nenolod.net>, et al.
 *
 * Name generator.
 */

#include <atheme.h>
#include "gameserv_common.h"
#include "namegen_tab.h"

static void
command_namegen(struct sourceinfo *si, int parc, char *parv[])
{
	unsigned int iter;
	unsigned int amt = 20;
	char buf[BUFSIZE];
	struct mychan *mc;

	if (!gs_do_parameters(si, &parc, &parv, &mc))
		return;

	// limit to 20
	if ((parv[0] && ! string_to_uint(parv[0], &amt)) || amt > 20)
		amt = 20;

	*buf = '\0';

	for (iter = 0; iter < amt; iter++)
	{
		char namebuf[BUFSIZE];
		unsigned int medial_iter;

		// Here we generate the name.
		mowgli_strlcpy(namebuf, begin_sym[rand() % BEGIN_SYM_SZ], BUFSIZE);

		for (medial_iter = rand() % 3; medial_iter > 0; medial_iter--)
			mowgli_strlcat(namebuf, medial_sym[rand() % MEDIAL_SYM_SZ], BUFSIZE);

		mowgli_strlcat(namebuf, end_sym[rand() % END_SYM_SZ], BUFSIZE);

		if (iter == 0)
			mowgli_strlcpy(buf, namebuf, BUFSIZE);
		else
			mowgli_strlcat(buf, namebuf, BUFSIZE);

		mowgli_strlcat(buf, iter + 1 < amt ? ", " : ".", BUFSIZE);
	}

	gs_command_report(si, _("Some names to ponder: %s"), buf);
}

static struct command cmd_namegen = {
	.name           = "NAMEGEN",
	.desc           = N_("Generates some names to ponder."),
	.access         = AC_NONE,
	.maxparc        = 2,
	.cmd            = &command_namegen,
	.help           = { .path = "gameserv/namegen" },
};

static void
mod_init(struct module ATHEME_VATTR_UNUSED *const restrict m)
{
	service_named_bind_command("gameserv", &cmd_namegen);

	service_named_bind_command("chanserv", &cmd_namegen);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("gameserv", &cmd_namegen);

	service_named_unbind_command("chanserv", &cmd_namegen);
}

SIMPLE_DECLARE_MODULE_V1("gameserv/namegen", MODULE_UNLOAD_CAPABILITY_OK)
