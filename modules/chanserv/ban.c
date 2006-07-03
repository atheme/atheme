/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService BAN/UNBAN function.
 *
 * $Id: ban.c 5686 2006-07-03 16:25:03Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/ban", FALSE, _modinit, _moddeinit,
	"$Id: ban.c 5686 2006-07-03 16:25:03Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_ban(char *origin);
static void cs_cmd_unban(char *origin);
static void cs_fcmd_ban(char *origin, char *channel);
static void cs_fcmd_unban(char *origin, char *channel);

command_t cs_ban = { "BAN", "Sets a ban on a channel.",
                        AC_NONE, cs_cmd_ban };
command_t cs_unban = { "UNBAN", "Removes a ban on a channel.",
			AC_NONE, cs_cmd_unban };

fcommand_t fc_ban = { "!ban", AC_NONE, cs_fcmd_ban };
fcommand_t fc_unban = { "!unban", AC_NONE, cs_fcmd_unban };

list_t *cs_cmdtree;
list_t *cs_fcmdtree;
list_t *cs_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(cs_cmdtree, "chanserv/main", "cs_cmdtree");
	MODULE_USE_SYMBOL(cs_fcmdtree, "chanserv/main", "cs_fcmdtree");
	MODULE_USE_SYMBOL(cs_helptree, "chanserv/main", "cs_helptree");

        command_add(&cs_ban, cs_cmdtree);
	command_add(&cs_unban, cs_cmdtree);
	fcommand_add(&fc_ban, cs_fcmdtree);
	fcommand_add(&fc_unban, cs_fcmdtree);

	help_addentry(cs_helptree, "BAN", "help/cservice/ban", NULL);
	help_addentry(cs_helptree, "UNBAN", "help/cservice/unban", NULL);
}

void _moddeinit()
{
	command_delete(&cs_ban, cs_cmdtree);
	command_delete(&cs_unban, cs_cmdtree);
	fcommand_delete(&fc_ban, cs_fcmdtree);
	fcommand_delete(&fc_unban, cs_fcmdtree);

	help_delentry(cs_helptree, "BAN");
	help_delentry(cs_helptree, "UNBAN");
}

static void cs_cmd_ban (char *origin)
{
	char *channel = strtok(NULL, " ");
	char *target = strtok(NULL, " ");
	channel_t *c = channel_find(channel);
	mychan_t *mc = mychan_find(channel);
	user_t *u = user_find_named(origin);
	user_t *tu;

	if (!channel || !target)
	{
		notice(chansvs.nick, origin, STR_INSUFFICIENT_PARAMS, "BAN");
		notice(chansvs.nick, origin, "Syntax: BAN <#channel> <nickname|hostmask>");
		return;
	}

	if (!c)
	{
		notice(chansvs.nick, origin, "Channel \2%s\2 does not exist.", channel);
		return;
	}

	if (!mc)
	{
		notice(chansvs.nick, origin, "\2%s\2 is not registered.", channel);
		return;
	}

	if (!u->myuser)
	{
		notice(chansvs.nick, origin, "You are not logged in.");
		return;
	}

	if (!chanacs_user_has_flag(mc, u, CA_REMOVE))
	{
		notice(chansvs.nick, origin, "You are not authorized to perform this operation.");
		return;
	}
	
	if (metadata_find(mc, METADATA_CHANNEL, "private:close:closer"))
	{
		notice(chansvs.nick, origin, "\2%s\2 is closed.", channel);
		return;
	}

	if (validhostmask(target))
	{
		modestack_mode_param(chansvs.nick, c->name, MTYPE_ADD, 'b', target);
		chanban_add(c, target, 'b');
		logcommand(chansvs.me, u, CMDLOG_DO, "%s BAN %s", mc->name, target);
		if (!chanuser_find(mc->chan, u))
			notice(chansvs.nick, origin, "Banned \2%s\2 on \2%s\2.", target, channel);
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
		logcommand(chansvs.me, u, CMDLOG_DO, "%s BAN %s (for user %s!%s@%s)", mc->name, hostbuf, tu->nick, tu->user, tu->vhost);
		if (!chanuser_find(mc->chan, u))
			notice(chansvs.nick, origin, "Banned \2%s\2 on \2%s\2.", target, channel);
		return;
	}
	else
	{
		notice(chansvs.nick, origin, "Invalid nickname/hostmask provided: \2%s\2", target);
		notice(chansvs.nick, origin, "Syntax: BAN <#channel> <nickname|hostmask>");
		return;
	}
}

static void cs_cmd_unban (char *origin)
{
        char *channel = strtok(NULL, " ");
        char *target = strtok(NULL, " ");
        channel_t *c = channel_find(channel);
	mychan_t *mc = mychan_find(channel);
	user_t *u = user_find_named(origin);
	user_t *tu;
	chanban_t *cb;

	if (!target)
		target = origin;

	if (!channel)
	{
		notice(chansvs.nick, origin, STR_INSUFFICIENT_PARAMS, "UNBAN");
		notice(chansvs.nick, origin, "Syntax: UNBAN <#channel> <nickname|hostmask>");
		return;
	}

	if (!c)
	{
		notice(chansvs.nick, origin, "Channel \2%s\2 does not exist.", channel);
		return;
	}

	if (!mc)
	{
		notice(chansvs.nick, origin, "Channel \2%s\2 is not registered.", channel);
		return;
	}

	if (!u->myuser)
	{
		notice(chansvs.nick, origin, "You are not logged in.");
		return;
	}

	if (!chanacs_user_has_flag(mc, u, CA_REMOVE))
	{
		notice(chansvs.nick, origin, "You are not authorized to perform this operation.");
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

			/* XXX doesn't do CIDR bans */
			if (!match(cb->mask, hostbuf) || !match(cb->mask, hostbuf2) || !match(cb->mask, hostbuf3))
			{
				logcommand(chansvs.me, u, CMDLOG_DO, "%s UNBAN %s (for user %s)", mc->name, cb->mask, hostbuf2);
				modestack_mode_param(chansvs.nick, c->name, MTYPE_DEL, 'b', cb->mask);
				chanban_delete(cb);
				count++;
			}
		}
		if (count > 0)
			notice(chansvs.nick, origin, "Unbanned \2%s\2 on \2%s\2 (%d ban%s removed).",
				target, channel, count, (count != 1 ? "s" : ""));
		else
			notice(chansvs.nick, origin, "No bans found matching \2%s\2 on \2%s\2.", target, channel);
		return;
	}
	else if ((cb = chanban_find(c, target, 'b')) != NULL || validhostmask(target))
	{
		if (cb)
		{
			modestack_mode_param(chansvs.nick, c->name, MTYPE_DEL, 'b', target);
			chanban_delete(cb);
			logcommand(chansvs.me, u, CMDLOG_DO, "%s UNBAN %s", mc->name, target);
			if (!chanuser_find(mc->chan, u))
				notice(chansvs.nick, origin, "Unbanned \2%s\2 on \2%s\2.", target, channel);
		}

		return;
	}
        else
        {
		notice(chansvs.nick, origin, "Invalid nickname/hostmask provided: \2%s\2", target);
		notice(chansvs.nick, origin, "Syntax: UNBAN <#channel> [nickname|hostmask]");
		return;
        }
}

static void cs_fcmd_ban (char *origin, char *channel)
{
	char *target = strtok(NULL, " ");
	channel_t *c = channel_find(channel);
	mychan_t *mc = mychan_find(channel);
	user_t *u = user_find_named(origin);
	user_t *tu;

	if (!channel || !target)
	{
		notice(chansvs.nick, origin, STR_INSUFFICIENT_PARAMS, "!BAN");
		notice(chansvs.nick, origin, "Syntax: !BAN <nickname|hostmask>");
		return;
	}

	if (!c)
	{
		notice(chansvs.nick, origin, "Channel \2%s\2 does not exist.", channel);
		return;
	}

	if (!mc)
	{
		notice(chansvs.nick, origin, "\2%s\2 is not registered.", channel);
		return;
	}

	if (!u->myuser)
	{
		notice(chansvs.nick, origin, "You are not logged in.");
		return;
	}

	if (!chanacs_user_has_flag(mc, u, CA_REMOVE))
	{
		notice(chansvs.nick, origin, "You are not authorized to perform this operation.");
		return;
	}

	if (validhostmask(target))
	{
		modestack_mode_param(chansvs.nick, c->name, MTYPE_ADD, 'b', target);
		chanban_add(c, target, 'b');
		logcommand(chansvs.me, u, CMDLOG_DO, "%s BAN %s", mc->name, target);
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
		logcommand(chansvs.me, u, CMDLOG_DO, "%s BAN %s (for user %s!%s@%s)", mc->name, hostbuf, tu->nick, tu->user, tu->vhost);
		return;
	}
	else
	{
		notice(chansvs.nick, origin, "Invalid nickname/hostmask provided: \2%s\2", target);
		notice(chansvs.nick, origin, "Syntax: !BAN <nickname|hostmask>");
		return;
	}
}

static void cs_fcmd_unban (char *origin, char *channel)
{
        char *target = strtok(NULL, " ");
        channel_t *c = channel_find(channel);
	mychan_t *mc = mychan_find(channel);
	user_t *u = user_find_named(origin);
	user_t *tu;
	chanban_t *cb;

	if (!target)
		target = origin;

	if (!channel)
	{
		notice(chansvs.nick, origin, STR_INSUFFICIENT_PARAMS, "!UNBAN");
		notice(chansvs.nick, origin, "Syntax: !UNBAN <nickname|hostmask>");
		return;
	}

	if (!c)
	{
		notice(chansvs.nick, origin, "Channel \2%s\2 does not exist.", channel);
		return;
	}

	if (!mc)
	{
		notice(chansvs.nick, origin, "Channel \2%s\2 is not registered.", channel);
		return;
	}

	if (!u->myuser)
	{
		notice(chansvs.nick, origin, "You are not logged in.");
		return;
	}

	if (!chanacs_user_has_flag(mc, u, CA_REMOVE))
	{
		notice(chansvs.nick, origin, "You are not authorized to perform this operation.");
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

			/* XXX doesn't do CIDR bans */
			if (!match(cb->mask, hostbuf) || !match(cb->mask, hostbuf2) || !match(cb->mask, hostbuf3))
			{
				logcommand(chansvs.me, u, CMDLOG_DO, "%s UNBAN %s (for user %s)", mc->name, cb->mask, hostbuf2);
				modestack_mode_param(chansvs.nick, c->name, MTYPE_DEL, 'b', cb->mask);
				chanban_delete(cb);
				count++;
			}
		}
		if (count > 0)
			notice(chansvs.nick, origin, "Unbanned \2%s\2 on \2%s\2 (%d ban%s removed).",
				target, channel, count, (count != 1 ? "s" : ""));
		else
			notice(chansvs.nick, origin, "No bans found matching \2%s\2 on \2%s\2.", target, channel);
		return;
	}
	else if ((cb = chanban_find(c, target, 'b')) != NULL || validhostmask(target))
	{
		if (cb)
		{
			modestack_mode_param(chansvs.nick, c->name, MTYPE_DEL, 'b', target);
			chanban_delete(cb);
			logcommand(chansvs.me, u, CMDLOG_DO, "%s UNBAN %s", mc->name, target);
		}

		return;
	}
        else
        {
		notice(chansvs.nick, origin, "Invalid nickname/hostmask provided: \2%s\2", target);
		notice(chansvs.nick, origin, "Syntax: !UNBAN [nickname|hostmask]");
		return;
        }
}

