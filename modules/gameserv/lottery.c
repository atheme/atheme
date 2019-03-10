/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2010 William Pitcock <nenolod@nenolod.net>, et al.
 *
 * Choose a random user on the channel.
 */

#include <atheme.h>
#include "gameserv_common.h"

static struct user *
pick_a_sucker(struct channel *c)
{
	int slot = atheme_random_uniform(MOWGLI_LIST_LENGTH(&c->members));
	struct chanuser *cu;

	cu = (struct chanuser *) mowgli_node_nth_data(&c->members, slot);
	while (cu != NULL && is_internal_client(cu->user))
	{
		slot = atheme_random_uniform(MOWGLI_LIST_LENGTH(&c->members));
		cu = (struct chanuser *) mowgli_node_nth_data(&c->members, slot);
	}

	return cu != NULL ? cu->user : NULL;
}

static void
command_lottery(struct sourceinfo *si, int parc, char *parv[])
{
	struct mychan *mc;
	struct user *u;

	if (!gs_do_parameters(si, &parc, &parv, &mc))
		return;

	if (si->c == NULL)
	{
		command_fail(si, fault_nosuch_target, _("This command may only be used on a channel."));
		return;
	}

	u = pick_a_sucker(si->c);
	return_if_fail(u != NULL);

	gs_command_report(si, "%s", u->nick);
}

static struct command cmd_lottery = {
	.name           = "LOTTERY",
	.desc           = N_("Choose a random user on a channel."),
	.access         = AC_NONE,
	.maxparc        = 2,
	.cmd            = &command_lottery,
	.help           = { .path = "gameserv/lottery" },
};

static void
mod_init(struct module ATHEME_VATTR_UNUSED *const restrict m)
{
	service_named_bind_command("gameserv", &cmd_lottery);

	service_named_bind_command("chanserv", &cmd_lottery);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("gameserv", &cmd_lottery);

	service_named_unbind_command("chanserv", &cmd_lottery);
}

SIMPLE_DECLARE_MODULE_V1("gameserv/lottery", MODULE_UNLOAD_CAPABILITY_OK)
