/*
 * Copyright (c) 2005-2006 William Pitcock <nenolod@nenolod.net> et al
 * Rights to this code are documented in doc/LICENSE.
 *
 * Choose a random user on the channel.
 */

#include "atheme.h"
#include "gameserv_common.h"

DECLARE_MODULE_V1
(
	"gameserv/lottery", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void command_lottery(sourceinfo_t *si, int parc, char *parv[]);

command_t cmd_lottery = { "LOTTERY", N_("Choose a random user on a channel."), AC_NONE, 2, command_lottery, { .path = "gameserv/lottery" } };

void _modinit(module_t * m)
{
	service_named_bind_command("gameserv", &cmd_lottery);

	service_named_bind_command("chanserv", &cmd_lottery);
}

void _moddeinit(module_unload_intent_t intent)
{
	service_named_unbind_command("gameserv", &cmd_lottery);

	service_named_unbind_command("chanserv", &cmd_lottery);
}

static user_t *pick_a_sucker(channel_t *c)
{
	int slot = arc4random() % MOWGLI_LIST_LENGTH(&c->members);
	chanuser_t *cu;

	cu = (chanuser_t *) mowgli_node_nth_data(&c->members, slot);
	while (cu != NULL && is_internal_client(cu->user))
	{
		slot = arc4random() % MOWGLI_LIST_LENGTH(&c->members);
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

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
