/*
 * Copyright (c) 2005-2006 William Pitcock <nenolod@nenolod.net> et al
 * Rights to this code are documented in doc/LICENSE.
 *
 * Choose a random user on the channel.
 */

#include "atheme.h"
#include "gameserv_common.h"

static void command_lottery(sourceinfo_t *si, int parc, char *parv[]);

command_t cmd_lottery = { "LOTTERY", N_("Choose a random user on a channel."), AC_NONE, 2, command_lottery, { .path = "gameserv/lottery" } };

static void
mod_init(module_t *const restrict m)
{
	service_named_bind_command("gameserv", &cmd_lottery);

	service_named_bind_command("chanserv", &cmd_lottery);
}

static void
mod_deinit(const module_unload_intent_t intent)
{
	service_named_unbind_command("gameserv", &cmd_lottery);

	service_named_unbind_command("chanserv", &cmd_lottery);
}

static user_t *pick_a_sucker(struct channel *c)
{
	int slot = arc4random_uniform(MOWGLI_LIST_LENGTH(&c->members));
	chanuser_t *cu;

	cu = (chanuser_t *) mowgli_node_nth_data(&c->members, slot);
	while (cu != NULL && is_internal_client(cu->user))
	{
		slot = arc4random_uniform(MOWGLI_LIST_LENGTH(&c->members));
		cu = (chanuser_t *) mowgli_node_nth_data(&c->members, slot);
	}

	return cu != NULL ? cu->user : NULL;
}

static void command_lottery(sourceinfo_t *si, int parc, char *parv[])
{
	mychan_t *mc;
	user_t *u;

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

SIMPLE_DECLARE_MODULE_V1("gameserv/lottery", MODULE_UNLOAD_CAPABILITY_OK)
