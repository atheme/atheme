/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService OP functions.
 *
 */

#include "atheme.h"
#include "chanserv.h"

DECLARE_MODULE_V1
(
	"chanserv/op", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_op(sourceinfo_t *si, int parc, char *parv[]);
static void cs_cmd_deop(sourceinfo_t *si, int parc, char *parv[]);

command_t cs_op = { "OP", N_("Gives channel ops to a user."),
                        AC_NONE, 2, cs_cmd_op, { .path = "cservice/op_voice" } };
command_t cs_deop = { "DEOP", N_("Removes channel ops from a user."),
                        AC_NONE, 2, cs_cmd_deop, { .path = "cservice/op_voice" } };

void _modinit(module_t *m)
{
        service_named_bind_command("chanserv", &cs_op);
        service_named_bind_command("chanserv", &cs_deop);
}

void _moddeinit(module_unload_intent_t intent)
{
	service_named_unbind_command("chanserv", &cs_op);
	service_named_unbind_command("chanserv", &cs_deop);
}

static mowgli_list_t op_actions;

static void cmd_op(sourceinfo_t *si, bool opping, int parc, char *parv[])
{
	char *chan = parv[0];
	char *nick = parv[1];
	mychan_t *mc;
	user_t *tu;
	chanuser_t *cu;
	char *nicks;
	bool op;
	mowgli_node_t *n;

	mc = mychan_find(chan);
	if (!mc)
	{
		command_fail(si, fault_nosuch_target, _("Channel \2%s\2 is not registered."), chan);
		return;
	}

	if (!chanacs_source_has_flag(mc, si, CA_OP) && (tu != si->su ||
				!chanacs_source_has_flag(mc, si, CA_AUTOOP)))
	{
		command_fail(si, fault_noprivs, _("You are not authorized to perform this operation."));
		return;
	}

	if (metadata_find(mc, "private:close:closer"))
	{
		command_fail(si, fault_noprivs, _("\2%s\2 is closed."), chan);
		return;
	}

	nicks = (!nick ? strdup(si->su->nick) : strdup(nick));
	prefix_action_set_all(&op_actions, opping, nicks);
	free(nicks);

	MOWGLI_LIST_FOREACH(n, op_actions.head)
	{
		struct prefix_action *act = n->data;
		nick = act->nick;
		op = act->en;

		/* figure out who we're going to op */
		if (!(tu = user_find_named(nick)))
		{
			command_fail(si, fault_nosuch_target, _("\2%s\2 is not online."), nick);
			continue;
		}

		if (is_internal_client(tu))
			continue;

		/* SECURE check; we can skip this if deopping or sender == target, because we already verified */
		if (op && (si->su != tu) && (mc->flags & MC_SECURE) && !chanacs_user_has_flag(mc, tu, CA_OP) && !chanacs_user_has_flag(mc, tu, CA_AUTOOP))
		{
			command_fail(si, fault_noprivs, _("You are not authorized to perform this operation."));
			command_fail(si, fault_noprivs, _("\2%s\2 has the SECURE option enabled, and \2%s\2 does not have appropriate access."), mc->name, tu->nick);
			continue;
		}

		cu = chanuser_find(mc->chan, tu);
		if (!cu)
		{
			command_fail(si, fault_nosuch_target, _("\2%s\2 is not on \2%s\2."), tu->nick, mc->name);
			continue;
		}

		modestack_mode_param(chansvs.nick, mc->chan, op ? MTYPE_ADD : MTYPE_DEL, 'o', CLIENT_NAME(tu));
		if (op)
			cu->modes |= CSTATUS_OP;
		else
			cu->modes &= ~CSTATUS_OP;

		if (si->c == NULL && tu != si->su)
			change_notify(chansvs.nick, tu, "You have been %sopped on %s by %s", op ? "" : "de", mc->name, get_source_name(si));

		logcommand(si, CMDLOG_DO, "%sOP: \2%s!%s@%s\2 on \2%s\2", op ? "" : "DE", tu->nick, tu->user, tu->vhost, mc->name);
		if (si->su == NULL || !chanuser_find(mc->chan, si->su))
			command_success_nodata(si, _("\2%s\2 has been %sopped on \2%s\2."), tu->nick, op ? "" : "de", mc->name);
	}

	prefix_action_clear(&op_actions);
}

static void cs_cmd_op(sourceinfo_t *si, int parc, char *parv[])
{
	if (!parv[0])
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "OP");
		command_fail(si, fault_needmoreparams, _("Syntax: OP <#channel> [nickname] [...]"));
		return;
	}

	cmd_op(si, true, parc, parv);
}

static void cs_cmd_deop(sourceinfo_t *si, int parc, char *parv[])
{
	if (!parv[0])
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "DEOP");
		command_fail(si, fault_needmoreparams, _("Syntax: DEOP <#channel> [nickname] [...]"));
		return;
	}

	cmd_op(si, false, parc, parv);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
