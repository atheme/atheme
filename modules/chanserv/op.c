/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService OP functions.
 *
 */

#include "atheme.h"

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

void _moddeinit()
{
	service_named_unbind_command("chanserv", &cs_op);
	service_named_unbind_command("chanserv", &cs_deop);
}

static void cs_cmd_op(sourceinfo_t *si, int parc, char *parv[])
{
	char *chan = parv[0];
	char *nick = parv[1];
	mychan_t *mc;
	user_t *tu;
	chanuser_t *cu;

	if (!chan)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "OP");
		command_fail(si, fault_needmoreparams, _("Syntax: OP <#channel> [nickname]"));
		return;
	}

	if (nick && *nick == '-')
	{
		parv[1]++;
		cs_cmd_deop(si, parc, parv);
		return;
	}

	mc = mychan_find(chan);
	if (!mc)
	{
		command_fail(si, fault_nosuch_target, _("Channel \2%s\2 is not registered."), chan);
		return;
	}

	if (!chanacs_source_has_flag(mc, si, CA_OP))
	{
		command_fail(si, fault_noprivs, _("You are not authorized to perform this operation."));
		return;
	}
	
	if (metadata_find(mc, "private:close:closer"))
	{
		command_fail(si, fault_noprivs, _("\2%s\2 is closed."), chan);
		return;
	}

	/* figure out who we're going to op */
	if (!nick)
		tu = si->su;
	else
	{
		if (!(tu = user_find_named(nick)))
		{
			command_fail(si, fault_nosuch_target, _("\2%s\2 is not online."), nick);
			return;
		}
	}

	if (is_internal_client(tu))
		return;

	/* SECURE check; we can skip this if sender == target, because we already verified */
	if ((si->su != tu) && (mc->flags & MC_SECURE) && !chanacs_user_has_flag(mc, tu, CA_OP) && !chanacs_user_has_flag(mc, tu, CA_AUTOOP))
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

	modestack_mode_param(chansvs.nick, mc->chan, MTYPE_ADD, 'o', CLIENT_NAME(tu));
	cu->modes |= CSTATUS_OP;

	if (si->c == NULL && tu != si->su)
		change_notify(chansvs.nick, tu, "You have been opped on %s by %s", mc->name, get_source_name(si));

	logcommand(si, CMDLOG_DO, "OP: \2%s!%s@%s\2 on \2%s\2", tu->nick, tu->user, tu->vhost, mc->name);
	if (!chanuser_find(mc->chan, si->su))
		command_success_nodata(si, _("\2%s\2 has been opped on \2%s\2."), tu->nick, mc->name);
}

static void cs_cmd_deop(sourceinfo_t *si, int parc, char *parv[])
{
	char *chan = parv[0];
	char *nick = parv[1];
	mychan_t *mc;
	user_t *tu;
	chanuser_t *cu;

	if (!chan)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "DEOP");
		command_fail(si, fault_needmoreparams, _("Syntax: DEOP <#channel> [nickname]"));
		return;
	}

	mc = mychan_find(chan);
	if (!mc)
	{
		command_fail(si, fault_nosuch_target, _("Channel \2%s\2 is not registered."), chan);
		return;
	}

	if (!chanacs_source_has_flag(mc, si, CA_OP))
	{
		command_fail(si, fault_noprivs, _("You are not authorized to perform this operation."));
		return;
	}
	
	if (metadata_find(mc, "private:close:closer"))
	{
		command_fail(si, fault_noprivs, _("\2%s\2 is closed."), chan);
		return;
	}

	/* figure out who we're going to deop */
	if (!nick)
		tu = si->su;
	else
	{
		if (!(tu = user_find_named(nick)))
		{
			command_fail(si, fault_nosuch_target, _("\2%s\2 is not online."), nick);
			return;
		}
	}

	if (is_internal_client(tu))
		return;

	cu = chanuser_find(mc->chan, tu);
	if (!cu)
	{
		command_fail(si, fault_nosuch_target, _("\2%s\2 is not on \2%s\2."), tu->nick, mc->name);
		return;
	}

	modestack_mode_param(chansvs.nick, mc->chan, MTYPE_DEL, 'o', CLIENT_NAME(tu));
	cu->modes &= ~CSTATUS_OP;

	if (si->c == NULL && tu != si->su)
		change_notify(chansvs.nick, tu, "You have been deopped on %s by %s", mc->name, get_source_name(si));

	logcommand(si, CMDLOG_DO, "DEOP: \2%s!%s@%s\2 on \2%s\2", tu->nick, tu->user, tu->vhost, mc->name);
	if (!chanuser_find(mc->chan, si->su))
		command_success_nodata(si, _("\2%s\2 has been deopped on \2%s\2."), tu->nick, mc->name);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
