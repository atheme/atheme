/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService OWNER functions.
 *
 */

#include "atheme.h"
#include "chanserv.h"

DECLARE_MODULE_V1
(
	"chanserv/protect", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_protect(sourceinfo_t *si, int parc, char *parv[]);
static void cs_cmd_deprotect(sourceinfo_t *si, int parc, char *parv[]);

command_t cs_protect = { "PROTECT", N_("Gives the channel protection flag to a user."),
                        AC_NONE, 2, cs_cmd_protect, { .path = "cservice/protect" } };
command_t cs_deprotect = { "DEPROTECT", N_("Removes channel protection flag from a user."),
                        AC_NONE, 2, cs_cmd_deprotect, { .path = "cservice/protect" } };

void _modinit(module_t *m)
{
	if (ircd != NULL && !ircd->uses_protect)
	{
		slog(LG_INFO, "Module %s requires protect support, refusing to load.", m->name);
		m->mflags = MODTYPE_FAIL;
		return;
	}

        service_named_bind_command("chanserv", &cs_protect);
        service_named_bind_command("chanserv", &cs_deprotect);
}

void _moddeinit(module_unload_intent_t intent)
{
	service_named_unbind_command("chanserv", &cs_protect);
	service_named_unbind_command("chanserv", &cs_deprotect);
}

static mowgli_list_t protect_actions;

static void cmd_protect(sourceinfo_t *si, bool protecting, int parc, char *parv[])
{
	char *chan = parv[0];
	char *nick = parv[1];
	mychan_t *mc;
	user_t *tu;
	chanuser_t *cu;
	char *nicks;
	bool protect;
	mowgli_node_t *n;

	if (ircd->uses_protect == false)
	{
		command_fail(si, fault_noprivs, _("The IRCd software you are running does not support this feature."));
		return;
	}

	mc = mychan_find(chan);
	if (!mc)
	{
		command_fail(si, fault_nosuch_target, _("Channel \2%s\2 is not registered."), chan);
		return;
	}

	if (!chanacs_source_has_flag(mc, si, CA_USEPROTECT))
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
	prefix_action_set_all(&protect_actions, protecting, nicks);
	free(nicks);

	MOWGLI_LIST_FOREACH(n, protect_actions.head)
	{
		struct prefix_action *act = n->data;
		nick = act->nick;
		protect = act->en;

		/* figure out who we're going to protect */
		if (!(tu = user_find_named(nick)))
		{
			command_fail(si, fault_nosuch_target, _("\2%s\2 is not online."), nick);
			continue;
		}

		if (is_internal_client(tu))
			continue;

		/* SECURE check; we can skip this if deprotecting or sender == target, because we already verified */
		if (protect && (si->su != tu) && (mc->flags & MC_SECURE) && !chanacs_user_has_flag(mc, tu, CA_OP) && !chanacs_user_has_flag(mc, tu, CA_AUTOOP))
		{
			command_fail(si, fault_noprivs, _("You are not authorized to perform this operation."));
			command_fail(si, fault_noprivs, _("\2%s\2 has the SECURE option enabled, and \2%s\2 does not have appropriate access."), mc->name, tu->nick);
			return;
		}

		cu = chanuser_find(mc->chan, tu);
		if (!cu)
		{
			command_fail(si, fault_nosuch_target, _("\2%s\2 is not on \2%s\2."), tu->nick, mc->name);
			return;
		}

		modestack_mode_param(chansvs.nick, mc->chan, protect ? MTYPE_ADD : MTYPE_DEL, ircd->protect_mchar[1], CLIENT_NAME(tu));
		if (protect)
			cu->modes |= CSTATUS_PROTECT;
		else
			cu->modes &= ~CSTATUS_PROTECT;

		if (si->c == NULL && tu != si->su)
			change_notify(chansvs.nick, tu, "You have been %sset as protected on %s by %s", protect ? "" : "un", mc->name, get_source_name(si));

		logcommand(si, CMDLOG_DO, "%sPROTECT: \2%s!%s@%s\2 on \2%s\2", protect ? "" : "DE", tu->nick, tu->user, tu->vhost, mc->name);
		if (si->su == NULL || !chanuser_find(mc->chan, si->su))
			command_success_nodata(si, _("\2%s\2 has been %sset as protected on \2%s\2."), tu->nick, protect ? "" : "un", mc->name);
	}

	prefix_action_clear(&protect_actions);
}

static void cs_cmd_protect(sourceinfo_t *si, int parc, char *parv[])
{
	if (!parv[0])
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "PROTECT");
		command_fail(si, fault_needmoreparams, _("Syntax: PROTECT <#channel> [nickname]"));
		return;
	}

	cmd_protect(si, true, parc, parv);
}

static void cs_cmd_deprotect(sourceinfo_t *si, int parc, char *parv[])
{
	if (!parv[0])
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "DEPROTECT");
		command_fail(si, fault_needmoreparams, _("Syntax: DEPROTECT <#channel> [nickname]"));
		return;
	}

	cmd_protect(si, false, parc, parv);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
