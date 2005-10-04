/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService BAN/UNBAN function.
 *
 * $Id: ban.c 2551 2005-10-04 06:14:07Z nenolod $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/ban", FALSE, _modinit, _moddeinit,
	"$Id: ban.c 2551 2005-10-04 06:14:07Z nenolod $",
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
	cs_cmdtree = module_locate_symbol("chanserv/main", "cs_cmdtree");
	cs_fcmdtree = module_locate_symbol("chanserv/main", "cs_fcmdtree");
	cs_helptree = module_locate_symbol("chanserv/main", "cs_helptree");

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
	chanacs_t *ca;
	user_t *u = user_find(origin);
	user_t *tu;

	if (!channel || !target)
	{
		notice(chansvs.nick, origin, "Insufficient parameters provided for \2BAN\2.");
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

	if (!(ca = chanacs_find(mc, u->myuser, CA_REMOVE)) && !(ca = chanacs_find(mc, u->myuser, CA_FLAGS)))
	{
		notice(chansvs.nick, origin, "You are not authorized to perform this operation.");
		return;
	}

	if (validhostmask(target))
	{
		cmode(chansvs.nick, c->name, "+b", target);
		chanban_add(c, target);
		notice(chansvs.nick, origin, "Banned \2%s\2 on \2%s\2.", target, channel);
		return;
	}
	else if ((tu = user_find_named(target)))
	{
		char hostbuf[BUFSIZE];

		hostbuf[0] = '\0';

		strlcat(hostbuf, "*!*@", BUFSIZE);
		strlcat(hostbuf, tu->vhost, BUFSIZE);

		cmode(chansvs.nick, c->name, "+b", hostbuf);
		chanban_add(c, hostbuf);
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
	chanacs_t *ca;
	user_t *u = user_find(origin);
	user_t *tu;

	if (!channel || !target)
	{
		notice(chansvs.nick, origin, "Insufficient parameters provided for \2UNBAN\2.");
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

	if (!(ca = chanacs_find(mc, u->myuser, CA_REMOVE)) && !(ca = chanacs_find(mc, u->myuser, CA_FLAGS)))
	{
		notice(chansvs.nick, origin, "You are not authorized to perform this operation.");
		return;
	}

	if (validhostmask(target))
	{
		chanban_t *cb = chanban_find(c, target);

		if (cb)
		{
			cmode(chansvs.nick, c->name, "-b", target);
			chanban_delete(cb);
			notice(chansvs.nick, origin, "Unbanned \2%s\2 on \2%s\2.", target, channel);
		}

		return;
	}
	else if ((tu = user_find_named(target)))
	{
		node_t *n;
		char hostbuf[BUFSIZE], hostbuf2[BUFSIZE];

		strlcat(hostbuf, tu->nick, BUFSIZE);
		strlcat(hostbuf, "!", BUFSIZE);
		strlcat(hostbuf, tu->user, BUFSIZE);
		strlcat(hostbuf, "@", BUFSIZE);

		strlcat(hostbuf2, hostbuf, BUFSIZE);

		strlcat(hostbuf, tu->host, BUFSIZE);
		strlcat(hostbuf2, tu->vhost, BUFSIZE);

		LIST_FOREACH(n, c->bans.head)
		{
			chanban_t *cb = n->data;

			slog(LG_DEBUG, "cs_unban(): iterating %s on %s", cb->mask, c->name);

			if (!match(cb->mask, hostbuf) || !match(cb->mask, hostbuf2))
			{
				cmode(chansvs.nick, c->name, "-b", cb->mask);
				chanban_delete(cb);
			}
		}
		notice(chansvs.nick, origin, "Unbanned \2%s\2 on \2%s\2.", target, channel);
		return;
	}
        else
        {
		notice(chansvs.nick, origin, "Invalid nickname/hostmask provided: \2%s\2", target);
		notice(chansvs.nick, origin, "Syntax: UNBAN <#channel> <nickname|hostmask>");
		return;
        }
}

static void cs_fcmd_ban (char *origin, char *channel)
{
	char *target = strtok(NULL, " ");
	channel_t *c = channel_find(channel);
	mychan_t *mc = mychan_find(channel);
	chanacs_t *ca;
	user_t *u = user_find(origin);
	user_t *tu;

	if (!channel || !target)
	{
		notice(chansvs.nick, origin, "Insufficient parameters provided for \2!BAN\2.");
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

	if (!(ca = chanacs_find(mc, u->myuser, CA_REMOVE)) && !(ca = chanacs_find(mc, u->myuser, CA_FLAGS)))
	{
		notice(chansvs.nick, origin, "You are not authorized to perform this operation.");
		return;
	}

	if (validhostmask(target))
	{
		cmode(chansvs.nick, c->name, "+b", target);
		chanban_add(c, target);
		notice(chansvs.nick, origin, "Banned \2%s\2 on \2%s\2.", target, channel);
		return;
	}
	else if ((tu = user_find_named(target)))
	{
		char hostbuf[BUFSIZE];

		hostbuf[0] = '\0';

		strlcat(hostbuf, "*!*@", BUFSIZE);
		strlcat(hostbuf, tu->vhost, BUFSIZE);

		cmode(chansvs.nick, c->name, "+b", hostbuf);
		chanban_add(c, hostbuf);
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

static void cs_fcmd_unban (char *origin, char *channel)
{
        char *target = strtok(NULL, " ");
        channel_t *c = channel_find(channel);
	mychan_t *mc = mychan_find(channel);
	chanacs_t *ca;
	user_t *u = user_find(origin);
	user_t *tu;

	if (!channel || !target)
	{
		notice(chansvs.nick, origin, "Insufficient parameters provided for \2!UNBAN\2.");
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

	if (!(ca = chanacs_find(mc, u->myuser, CA_REMOVE)) && !(ca = chanacs_find(mc, u->myuser, CA_FLAGS)))
	{
		notice(chansvs.nick, origin, "You are not authorized to perform this operation.");
		return;
	}

	if (validhostmask(target))
	{
		chanban_t *cb = chanban_find(c, target);

		if (cb)
		{
			cmode(chansvs.nick, c->name, "-b", target);
			chanban_delete(cb);
			notice(chansvs.nick, origin, "Unbanned \2%s\2 on \2%s\2.", target, channel);
		}

		return;
	}
	else if ((tu = user_find_named(target)))
	{
		node_t *n;
		char hostbuf[BUFSIZE], hostbuf2[BUFSIZE];

		strlcat(hostbuf, tu->nick, BUFSIZE);
		strlcat(hostbuf, "!", BUFSIZE);
		strlcat(hostbuf, tu->user, BUFSIZE);
		strlcat(hostbuf, "@", BUFSIZE);

		strlcat(hostbuf2, hostbuf, BUFSIZE);

		strlcat(hostbuf, tu->host, BUFSIZE);
		strlcat(hostbuf2, tu->vhost, BUFSIZE);

		LIST_FOREACH(n, c->bans.head)
		{
			chanban_t *cb = n->data;

			slog(LG_DEBUG, "cs_unban(): iterating %s on %s", cb->mask, c->name);

			if (!match(cb->mask, hostbuf) || !match(cb->mask, hostbuf2))
			{
				cmode(chansvs.nick, c->name, "-b", cb->mask);
				chanban_delete(cb);
			}
		}
		notice(chansvs.nick, origin, "Unbanned \2%s\2 on \2%s\2.", target, channel);
		return;
	}
        else
        {
		notice(chansvs.nick, origin, "Invalid nickname/hostmask provided: \2%s\2", target);
		notice(chansvs.nick, origin, "Syntax: UNBAN <#channel> <nickname|hostmask>");
		return;
        }
}

