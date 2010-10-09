/*
 * Copyright (c) 2005-2007 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains a CService UNBAN which can only unbans the source
 * user, not others.
 * Do not load chanserv/ban and chanserv/unban_self together.
 *
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/unban_self", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_unban(sourceinfo_t *si, int parc, char *parv[]);

command_t cs_unban = { "UNBAN", N_("Unbans you on a channel."),
			AC_NONE, 2, cs_cmd_unban, { .path = "cservice/unban_self" } };

void _modinit(module_t *m)
{
	service_named_bind_command("chanserv", &cs_unban);
}

void _moddeinit()
{
	service_named_unbind_command("chanserv", &cs_unban);
}

static void cs_cmd_unban(sourceinfo_t *si, int parc, char *parv[])
{
        char *channel = parv[0];
        char *target = parv[1];
        channel_t *c = channel_find(channel);
	mychan_t *mc = mychan_find(channel);
	user_t *tu;
	chanban_t *cb;

	if (si->su == NULL)
	{
		command_fail(si, fault_noprivs, _("\2%s\2 can only be executed via IRC."), "UNBAN");
		return;
	}

	if (!channel)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "UNBAN");
		command_fail(si, fault_needmoreparams, _("Syntax: UNBAN <#channel>"));
		return;
	}

	if (target && irccasecmp(target, si->su->nick))
	{
		command_fail(si, fault_noprivs, _("You may only unban yourself via %s."), si->service->nick);
		if (validhostmask(target))
			command_fail(si, fault_noprivs, _("Try \2/mode %s -b %s\2"), channel, target);
		return;
	}
	target = si->su->nick;

	if (!mc)
	{
		command_fail(si, fault_nosuch_target, _("Channel \2%s\2 is not registered."), channel);
		return;
	}

	if (!c)
	{
		command_fail(si, fault_nosuch_target, _("\2%s\2 is currently empty."), channel);
		return;
	}

	if (!si->smu)
	{
		command_fail(si, fault_noprivs, _("You are not logged in."));
		return;
	}

	if (!chanacs_source_has_flag(mc, si, CA_REMOVE))
	{
		command_fail(si, fault_noprivs, _("You are not authorized to perform this operation."));
		return;
	}

	tu = si->su;
	{
		mowgli_node_t *n, *tn;
		char hostbuf2[BUFSIZE];
		int count = 0;

		snprintf(hostbuf2, BUFSIZE, "%s!%s@%s", tu->nick, tu->user, tu->vhost);
		for (n = next_matching_ban(c, tu, 'b', c->bans.head); n != NULL; n = next_matching_ban(c, tu, 'b', tn))
		{
			tn = n->next;
			cb = n->data;

			logcommand(si, CMDLOG_DO, "UNBAN: \2%s\2 \2%s\2 (for user \2%s\2)", mc->name, cb->mask, hostbuf2);
			modestack_mode_param(chansvs.nick, c, MTYPE_DEL, cb->type, cb->mask);
			chanban_delete(cb);
			count++;
		}
		if (count > 0)
			command_success_nodata(si, _("Unbanned \2%s\2 on \2%s\2 (%d ban%s removed)."),
				target, channel, count, (count != 1 ? "s" : ""));
		else
			command_success_nodata(si, _("No bans found matching \2%s\2 on \2%s\2."), target, channel);
		return;
	}
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
