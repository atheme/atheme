/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService KICK functions.
 *
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/kick", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_kick(sourceinfo_t *si, int parc, char *parv[]);
static void cs_cmd_kickban(sourceinfo_t *si, int parc, char *parv[]);

command_t cs_kick = { "KICK", N_("Removes a user from a channel."),
                        AC_NONE, 3, cs_cmd_kick, { .path = "cservice/kick" } };
command_t cs_kickban = { "KICKBAN", N_("Removes and bans a user from a channel."),
			AC_NONE, 3, cs_cmd_kickban, { .path = "cservice/kickban" } };

void _modinit(module_t *m)
{
        service_named_bind_command("chanserv", &cs_kick);
	service_named_bind_command("chanserv", &cs_kickban);
}

void _moddeinit()
{
	service_named_unbind_command("chanserv", &cs_kick);
	service_named_unbind_command("chanserv", &cs_kickban);
}

static void cs_cmd_kick(sourceinfo_t *si, int parc, char *parv[])
{
	char *chan = parv[0];
	char *nick = parv[1];
	char *reason = parv[2];
	mychan_t *mc;
	user_t *tu;
	chanuser_t *cu;
	char reasonbuf[BUFSIZE];

	if (!chan || !nick)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "KICK");
		command_fail(si, fault_needmoreparams, _("Syntax: KICK <#channel> <nickname> [reason]"));
		return;
	}

	mc = mychan_find(chan);
	if (!mc)
	{
		command_fail(si, fault_nosuch_target, _("Channel \2%s\2 is not registered."), chan);
		return;
	}

	if (!chanacs_source_has_flag(mc, si, CA_REMOVE))
	{
		command_fail(si, fault_noprivs, _("You are not authorized to perform this operation."));
		return;
	}
	
	if (metadata_find(mc, "private:close:closer"))
	{
		command_fail(si, fault_noprivs, _("\2%s\2 is closed."), chan);
		return;
	}

	/* figure out who we're going to kick */
	if (!(tu = user_find_named(nick)))
	{
		command_fail(si, fault_nosuch_target, _("\2%s\2 is not online."), nick);
		return;
	}

	/* if target is a service, bail. --nenolod */
	if (is_internal_client(tu))
		return;

	cu = chanuser_find(mc->chan, tu);
	if (!cu)
	{
		command_fail(si, fault_nosuch_target, _("\2%s\2 is not on \2%s\2."), tu->nick, mc->name);
		return;
	}

	if (cu->modes & CSTATUS_OWNER || cu->modes & CSTATUS_PROTECT)
	{
		command_fail(si, fault_noprivs, _("\2%s\2 is protected from kicks; you cannot kick them."), tu->nick);
		return;
	}

	snprintf(reasonbuf, BUFSIZE, "(%s) %s", get_source_name(si), reason ? reason : "No reason given");
	try_kick(chansvs.me->me, mc->chan, tu, reasonbuf);
	logcommand(si, CMDLOG_DO, "KICK: \2%s!%s@%s\2 from \2%s\2", tu->nick, tu->user, tu->vhost, mc->name);
	if (si->su != tu && !chanuser_find(mc->chan, si->su))
		command_success_nodata(si, _("\2%s\2 has been kicked from \2%s\2."), tu->nick, mc->name);
}

static void cs_cmd_kickban(sourceinfo_t *si, int parc, char *parv[])
{
	char *chan = parv[0];
	char *nick = parv[1];
	char *reason = parv[2];
	mychan_t *mc;
	user_t *tu;
	chanuser_t *cu;
	char reasonbuf[BUFSIZE];
	int n;

	if (!chan || !nick)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "KICKBAN");
		command_fail(si, fault_needmoreparams, _("Syntax: KICKBAN <#channel> <nickname> [reason]"));
		return;
	}

	mc = mychan_find(chan);
	if (!mc)
	{
		command_fail(si, fault_nosuch_target, _("Channel \2%s\2 is not registered."), chan);
		return;
	}

	if (!chanacs_source_has_flag(mc, si, CA_REMOVE))
	{
		command_fail(si, fault_noprivs, _("You are not authorized to perform this operation."));
		return;
	}

	/* figure out who we're going to kick */
	if (!(tu = user_find_named(nick)))
	{
		command_fail(si, fault_nosuch_target, _("\2%s\2 is not online."), nick);
		return;
	}

        /* if target is a service, bail. --nenolod */
	if (is_internal_client(tu))
		return;

	cu = chanuser_find(mc->chan, tu);
	if (!cu)
	{
		command_fail(si, fault_nosuch_target, _("\2%s\2 is not on \2%s\2."), tu->nick, mc->name);
		return;
	}

	if (cu->modes & CSTATUS_OWNER || cu->modes & CSTATUS_PROTECT)
	{
		command_fail(si, fault_noprivs, _("\2%s\2 is protected from kicks; you cannot kick them."), tu->nick);
		return;
	}

	snprintf(reasonbuf, BUFSIZE, "(%s) %s", get_source_name(si), reason ? reason : "No reason given");
	ban(si->service->me, mc->chan, tu);
	n = remove_ban_exceptions(si->service->me, mc->chan, tu);
	if (n > 0)
		command_success_nodata(si, _("To avoid rejoin, %d ban exception(s) matching \2%s\2 have been removed from \2%s\2."), n, tu->nick, mc->name);
	try_kick(chansvs.me->me, mc->chan, tu, reasonbuf);
	logcommand(si, CMDLOG_DO, "KICKBAN: \2%s!%s@%s\2 from \2%s\2", tu->nick, tu->user, tu->vhost, mc->name);
	if (si->su != tu && !chanuser_find(mc->chan, si->su))
		command_success_nodata(si, _("\2%s\2 has been kickbanned from \2%s\2."), tu->nick, mc->name);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
