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
	"chanserv/halfop", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_halfop(sourceinfo_t *si, int parc, char *parv[]);
static void cs_cmd_dehalfop(sourceinfo_t *si, int parc, char *parv[]);

command_t cs_halfop = { "HALFOP", N_("Gives channel halfops to a user."),
                        AC_NONE, 2, cs_cmd_halfop, { .path = "cservice/halfop" } };
command_t cs_dehalfop = { "DEHALFOP", N_("Removes channel halfops from a user."),
                        AC_NONE, 2, cs_cmd_dehalfop, { .path = "cservice/halfop" } };

void _modinit(module_t *m)
{
	if (ircd != NULL && !ircd->uses_halfops)
	{
		slog(LG_INFO, "Module %s requires halfop support, refusing to load.", m->name);
		m->mflags = MODTYPE_FAIL;
		return;
	}

        service_named_bind_command("chanserv", &cs_halfop);
        service_named_bind_command("chanserv", &cs_dehalfop);
}

void _moddeinit(module_unload_intent_t intent)
{
	service_named_unbind_command("chanserv", &cs_halfop);
	service_named_unbind_command("chanserv", &cs_dehalfop);
}

static mowgli_list_t halfop_actions;

static void cmd_halfop(sourceinfo_t *si, bool halfopping, int parc, char *parv[])
{
	char *chan = parv[0];
	char *nick = parv[1];
	mychan_t *mc;
	user_t *tu;
	chanuser_t *cu;
	char *nicks;
	bool halfop;
	mowgli_node_t *n;

	if (!ircd->uses_halfops)
	{
		command_fail(si, fault_noprivs, _("Your IRC server does not support halfops."));
		return;
	}

	mc = mychan_find(chan);
	if (!mc)
	{
		command_fail(si, fault_nosuch_target, _("Channel \2%s\2 is not registered."), chan);
		return;
	}

	if (!chanacs_source_has_flag(mc, si, CA_HALFOP))
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
	prefix_action_set_all(&halfop_actions, halfopping, nicks);
	free(nicks);

	MOWGLI_LIST_FOREACH(n, halfop_actions.head)
	{
		struct prefix_action *act = n->data;
		nick = act->nick;
		halfop = act->en;

		/* figure out who we're going to halfop */
		if (!(tu = user_find_named(nick)))
		{
			command_fail(si, fault_nosuch_target, _("\2%s\2 is not online."), nick);
			continue;
		}

		if (is_internal_client(tu))
			continue;

		/* SECURE check; we can skip this if deopping or sender == target, because we already verified */
		if (halfop && (si->su != tu) && (mc->flags & MC_SECURE) && !chanacs_user_has_flag(mc, tu, CA_HALFOP) && !chanacs_user_has_flag(mc, tu, CA_AUTOHALFOP))
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

		modestack_mode_param(chansvs.nick, mc->chan, halfop ? MTYPE_ADD : MTYPE_DEL, 'h', CLIENT_NAME(tu));
		if (halfop)
			cu->modes |= ircd->halfops_mode;
		else
			cu->modes &= ~ircd->halfops_mode;

		if (si->c == NULL && tu != si->su)
			change_notify(chansvs.nick, tu, "You have been %shalfopped on %s by %s", halfop ? "" : "de", mc->name, get_source_name(si));

		logcommand(si, CMDLOG_DO, "%sHALFOP: \2%s!%s@%s\2 on \2%s\2", halfop ? "" : "DE", tu->nick, tu->user, tu->vhost, mc->name);
		if (si->su == NULL || !chanuser_find(mc->chan, si->su))
			command_success_nodata(si, _("\2%s\2 has been %shalfopped on \2%s\2."), tu->nick, halfop ? "" : "de", mc->name);
	}

	prefix_action_clear(&halfop_actions);
}

static void cs_cmd_halfop(sourceinfo_t *si, int parc, char *parv[])
{
	if (!parv[0])
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "HALFOP");
		command_fail(si, fault_needmoreparams, _("Syntax: HALFOP <#channel> [nickname] [...]"));
		return;
	}

	cmd_halfop(si, true, parc, parv);
}

static void cs_cmd_dehalfop(sourceinfo_t *si, int parc, char *parv[])
{
	if (!parv[0])
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "DEHALFOP");
		command_fail(si, fault_needmoreparams, _("Syntax: DEHALFOP <#channel> [nickname] [...]"));
		return;
	}

	cmd_halfop(si, false, parc, parv);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
