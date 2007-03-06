/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService BAN/UNBAN function.
 *
 * $Id: ban.c 7855 2007-03-06 00:43:08Z pippijn $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/ban", FALSE, _modinit, _moddeinit,
	"$Id: ban.c 7855 2007-03-06 00:43:08Z pippijn $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_ban(sourceinfo_t *si, int parc, char *parv[]);
static void cs_cmd_unban(sourceinfo_t *si, int parc, char *parv[]);

command_t cs_ban = { "BAN", N_("Sets a ban on a channel."),
                        AC_NONE, 2, cs_cmd_ban };
command_t cs_unban = { "UNBAN", N_("Removes a ban on a channel."),
			AC_NONE, 2, cs_cmd_unban };


list_t *cs_cmdtree;
list_t *cs_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(cs_cmdtree, "chanserv/main", "cs_cmdtree");
	MODULE_USE_SYMBOL(cs_helptree, "chanserv/main", "cs_helptree");

        command_add(&cs_ban, cs_cmdtree);
	command_add(&cs_unban, cs_cmdtree);

	help_addentry(cs_helptree, "BAN", "help/cservice/ban", NULL);
	help_addentry(cs_helptree, "UNBAN", "help/cservice/unban", NULL);
}

void _moddeinit()
{
	command_delete(&cs_ban, cs_cmdtree);
	command_delete(&cs_unban, cs_cmdtree);

	help_delentry(cs_helptree, "BAN");
	help_delentry(cs_helptree, "UNBAN");
}

static void cs_cmd_ban(sourceinfo_t *si, int parc, char *parv[])
{
	char *channel = parv[0];
	char *target = parv[1];
	channel_t *c = channel_find(channel);
	mychan_t *mc = mychan_find(channel);
	user_t *tu;

	if (!channel || !target)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "BAN");
		command_fail(si, fault_needmoreparams, "Syntax: BAN <#channel> <nickname|hostmask>");
		return;
	}

	if (!c)
	{
		command_fail(si, fault_nosuch_target, "Channel \2%s\2 does not exist.", channel);
		return;
	}

	if (!mc)
	{
		command_fail(si, fault_nosuch_target, "\2%s\2 is not registered.", channel);
		return;
	}

	if (!si->smu)
	{
		command_fail(si, fault_noprivs, "You are not logged in.");
		return;
	}

	if (!chanacs_source_has_flag(mc, si, CA_REMOVE))
	{
		command_fail(si, fault_noprivs, "You are not authorized to perform this operation.");
		return;
	}
	
	if (metadata_find(mc, METADATA_CHANNEL, "private:close:closer"))
	{
		command_fail(si, fault_noprivs, "\2%s\2 is closed.", channel);
		return;
	}

	if (validhostmask(target))
	{
		modestack_mode_param(chansvs.nick, c->name, MTYPE_ADD, 'b', target);
		chanban_add(c, target, 'b');
		logcommand(si, CMDLOG_DO, "%s BAN %s", mc->name, target);
		if (!chanuser_find(mc->chan, si->su))
			command_success_nodata(si, "Banned \2%s\2 on \2%s\2.", target, channel);
		return;
	}
	else if ((tu = user_find_named(target)))
	{
		char hostbuf[BUFSIZE];

		hostbuf[0] = '\0';

		strlcat(hostbuf, "*!*@", BUFSIZE);
		strlcat(hostbuf, tu->vhost, BUFSIZE);

		modestack_mode_param(chansvs.nick, c->name, MTYPE_ADD, 'b', hostbuf);
		chanban_add(c, hostbuf, 'b');
		logcommand(si, CMDLOG_DO, "%s BAN %s (for user %s!%s@%s)", mc->name, hostbuf, tu->nick, tu->user, tu->vhost);
		if (!chanuser_find(mc->chan, si->su))
			command_success_nodata(si, "Banned \2%s\2 on \2%s\2.", target, channel);
		return;
	}
	else
	{
		command_fail(si, fault_badparams, "Invalid nickname/hostmask provided: \2%s\2", target);
		command_fail(si, fault_badparams, "Syntax: BAN <#channel> <nickname|hostmask>");
		return;
	}
}

static void cs_cmd_unban(sourceinfo_t *si, int parc, char *parv[])
{
        char *channel = parv[0];
        char *target = parv[1];
        channel_t *c = channel_find(channel);
	mychan_t *mc = mychan_find(channel);
	user_t *tu;
	chanban_t *cb;

	if (!channel)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "UNBAN");
		command_fail(si, fault_needmoreparams, "Syntax: UNBAN <#channel> <nickname|hostmask>");
		return;
	}

	if (!target)
	{
		if (si->su == NULL)
		{
			command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "UNBAN");
			command_fail(si, fault_needmoreparams, "Syntax: UNBAN <#channel> <nickname|hostmask>");
			return;
		}
		target = si->su->nick;
	}

	if (!c)
	{
		command_fail(si, fault_nosuch_target, "Channel \2%s\2 does not exist.", channel);
		return;
	}

	if (!mc)
	{
		command_fail(si, fault_nosuch_target, "Channel \2%s\2 is not registered.", channel);
		return;
	}

	if (!si->smu)
	{
		command_fail(si, fault_noprivs, "You are not logged in.");
		return;
	}

	if (!chanacs_source_has_flag(mc, si, CA_REMOVE))
	{
		command_fail(si, fault_noprivs, "You are not authorized to perform this operation.");
		return;
	}

	if ((tu = user_find_named(target)))
	{
		node_t *n, *tn;
		char hostbuf[BUFSIZE], hostbuf2[BUFSIZE], hostbuf3[BUFSIZE];
		int count = 0;

		snprintf(hostbuf, BUFSIZE, "%s!%s@%s", tu->nick, tu->user, tu->host);
		snprintf(hostbuf2, BUFSIZE, "%s!%s@%s", tu->nick, tu->user, tu->vhost);
		/* will be nick!user@ if ip unknown, doesn't matter */
		snprintf(hostbuf3, BUFSIZE, "%s!%s@%s", tu->nick, tu->user, tu->ip);

		LIST_FOREACH_SAFE(n, tn, c->bans.head)
		{
			cb = n->data;

			if (cb->type != 'b')
				continue;
			slog(LG_DEBUG, "cs_unban(): iterating %s on %s", cb->mask, c->name);

			if (!match(cb->mask, hostbuf) || !match(cb->mask, hostbuf2) || !match(cb->mask, hostbuf3) || !match_cidr(cb->mask, hostbuf3))
			{
				logcommand(si, CMDLOG_DO, "%s UNBAN %s (for user %s)", mc->name, cb->mask, hostbuf2);
				modestack_mode_param(chansvs.nick, c->name, MTYPE_DEL, 'b', cb->mask);
				chanban_delete(cb);
				count++;
			}
		}
		if (count > 0)
			command_success_nodata(si, "Unbanned \2%s\2 on \2%s\2 (%d ban%s removed).",
				target, channel, count, (count != 1 ? "s" : ""));
		else
			command_success_nodata(si, "No bans found matching \2%s\2 on \2%s\2.", target, channel);
		return;
	}
	else if ((cb = chanban_find(c, target, 'b')) != NULL || validhostmask(target))
	{
		if (cb)
		{
			modestack_mode_param(chansvs.nick, c->name, MTYPE_DEL, 'b', target);
			chanban_delete(cb);
			logcommand(si, CMDLOG_DO, "%s UNBAN %s", mc->name, target);
			if (!chanuser_find(mc->chan, si->su))
				command_success_nodata(si, "Unbanned \2%s\2 on \2%s\2.", target, channel);
		}
		else
			command_fail(si, fault_nosuch_key, "No such ban \2%s\2 on \2%s\2.", target, channel);

		return;
	}
        else
        {
		command_fail(si, fault_badparams, "Invalid nickname/hostmask provided: \2%s\2", target);
		command_fail(si, fault_badparams, "Syntax: UNBAN <#channel> [nickname|hostmask]");
		return;
        }
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
