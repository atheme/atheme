/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService KICK functions.
 *
 * $Id: kick.c 5686 2006-07-03 16:25:03Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/kick", FALSE, _modinit, _moddeinit,
	"$Id: kick.c 5686 2006-07-03 16:25:03Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_kick(char *origin);
static void cs_cmd_kickban(char *origin);
static void cs_fcmd_kick(char *origin, char *chan);
static void cs_fcmd_kickban(char *origin, char *chan);

command_t cs_kick = { "KICK", "Removes a user from a channel.",
                        AC_NONE, cs_cmd_kick };
command_t cs_kickban = { "KICKBAN", "Removes and bans a user from a channel.",
			AC_NONE, cs_cmd_kickban };

fcommand_t fc_kick = { "!kick", AC_NONE, cs_fcmd_kick };
fcommand_t fc_kickban = { "!kickban", AC_NONE, cs_fcmd_kickban };
fcommand_t fc_kb = { "!kb", AC_NONE, cs_fcmd_kickban };

list_t *cs_cmdtree;
list_t *cs_fcmdtree;
list_t *cs_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(cs_cmdtree, "chanserv/main", "cs_cmdtree");
	MODULE_USE_SYMBOL(cs_fcmdtree, "chanserv/main", "cs_fcmdtree");
	MODULE_USE_SYMBOL(cs_helptree, "chanserv/main", "cs_helptree");

        command_add(&cs_kick, cs_cmdtree);
	command_add(&cs_kickban, cs_cmdtree);
	fcommand_add(&fc_kick, cs_fcmdtree);
	fcommand_add(&fc_kickban, cs_fcmdtree);
	fcommand_add(&fc_kb, cs_fcmdtree);

	help_addentry(cs_helptree, "KICK", "help/cservice/kick", NULL);
	help_addentry(cs_helptree, "KICKBAN", "help/cservice/kickban", NULL);
}

void _moddeinit()
{
	command_delete(&cs_kick, cs_cmdtree);
	command_delete(&cs_kickban, cs_cmdtree);
	fcommand_delete(&fc_kick, cs_fcmdtree);
	fcommand_delete(&fc_kickban, cs_fcmdtree);
	fcommand_delete(&fc_kb, cs_fcmdtree);

	help_delentry(cs_helptree, "KICK");
	help_delentry(cs_helptree, "KICKBAN");
}

static void cs_cmd_kick(char *origin)
{
	char *chan = strtok(NULL, " ");
	char *nick = strtok(NULL, " ");
	char *reason = strtok(NULL, "");
	mychan_t *mc;
	user_t *u, *tu;
	chanuser_t *cu;
	char reasonbuf[BUFSIZE];

	if (!chan || !nick)
	{
		notice(chansvs.nick, origin, STR_INSUFFICIENT_PARAMS, "KICK");
		notice(chansvs.nick, origin, "Syntax: KICK <#channel> <nickname> [reason]");
		return;
	}

	mc = mychan_find(chan);
	if (!mc)
	{
		notice(chansvs.nick, origin, "\2%s\2 is not registered.", chan);
		return;
	}

	u = user_find_named(origin);

	if (!chanacs_user_has_flag(mc, u, CA_REMOVE))
	{
		notice(chansvs.nick, origin, "You are not authorized to perform this operation.");
		return;
	}
	
	if (metadata_find(mc, METADATA_CHANNEL, "private:close:closer"))
	{
		notice(chansvs.nick, origin, "\2%s\2 is closed.", chan);
		return;
	}

	/* figure out who we're going to kick */
	if (!(tu = user_find_named(nick)))
	{
		notice(chansvs.nick, origin, "\2%s\2 is not online.", nick);
		return;
	}

	/* if target is a service, bail. --nenolod */
	if (is_internal_client(tu))
		return;

	cu = chanuser_find(mc->chan, tu);
	if (!cu)
	{
		notice(chansvs.nick, origin, "\2%s\2 is not on \2%s\2.", tu->nick, mc->name);
		return;
	}

	snprintf(reasonbuf, BUFSIZE, "%s (%s)", reason ? reason : "No reason given", origin);
	kick(chansvs.nick, chan, tu->nick, reasonbuf);
	logcommand(chansvs.me, u, CMDLOG_SET, "%s KICK %s!%s@%s", mc->name, tu->nick, tu->user, tu->vhost);
	if (u != tu && !chanuser_find(mc->chan, u))
		notice(chansvs.nick, origin, "\2%s\2 has been kicked from \2%s\2.", tu->nick, mc->name);
}

static void cs_cmd_kickban(char *origin)
{
	char *chan = strtok(NULL, " ");
	char *nick = strtok(NULL, " ");
	char *reason = strtok(NULL, "");
	mychan_t *mc;
	user_t *u, *tu;
	chanuser_t *cu;
	char reasonbuf[BUFSIZE];
	int n;

	if (!chan || !nick)
	{
		notice(chansvs.nick, origin, STR_INSUFFICIENT_PARAMS, "KICKBAN");
		notice(chansvs.nick, origin, "Syntax: KICKBAN <#channel> <nickname> [reason]");
		return;
	}

	mc = mychan_find(chan);
	if (!mc)
	{
		notice(chansvs.nick, origin, "\2%s\2 is not registered.", chan);
		return;
	}

	u = user_find_named(origin);

	if (!chanacs_user_has_flag(mc, u, CA_REMOVE))
	{
		notice(chansvs.nick, origin, "You are not authorized to perform this operation.");
		return;
	}

	/* figure out who we're going to kick */
	if (!(tu = user_find_named(nick)))
	{
		notice(chansvs.nick, origin, "\2%s\2 is not online.", nick);
		return;
	}

        /* if target is a service, bail. --nenolod */
	if (is_internal_client(tu))
		return;

	cu = chanuser_find(mc->chan, tu);
	if (!cu)
	{
		notice(chansvs.nick, origin, "\2%s\2 is not on \2%s\2.", tu->nick, mc->name);
		return;
	}

	snprintf(reasonbuf, BUFSIZE, "%s (%s)", reason ? reason : "No reason given", origin);
	ban(chansvs.nick, chan, tu);
	n = remove_ban_exceptions(chansvs.me->me, mc->chan, tu);
	if (n > 0)
		notice(chansvs.nick, origin, "To avoid rejoin, %d ban exception(s) matching \2%s\2 have been removed from \2%s\2.", n, tu->nick, mc->name);
	kick(chansvs.nick, chan, tu->nick, reasonbuf);
	logcommand(chansvs.me, user_find_named(origin), CMDLOG_SET, "%s KICKBAN %s!%s@%s", mc->name, tu->nick, tu->user, tu->vhost);
	if (u != tu && !chanuser_find(mc->chan, u))
		notice(chansvs.nick, origin, "\2%s\2 has been kickbanned from \2%s\2.", tu->nick, mc->name);
}

static void cs_fcmd_kick(char *origin, char *chan)
{
	char *nick = strtok(NULL, " ");
	char *reason = strtok(NULL, "");
	mychan_t *mc;
	user_t *u, *tu;
	chanuser_t *cu;
	char reasonbuf[BUFSIZE];

	if (!chan || !nick)
	{
		notice(chansvs.nick, origin, STR_INSUFFICIENT_PARAMS, "KICK");
		notice(chansvs.nick, origin, "Syntax: KICK <#channel> <nickname> [reason]");
		return;
	}

	mc = mychan_find(chan);
	if (!mc)
	{
		notice(chansvs.nick, origin, "\2%s\2 is not registered.", chan);
		return;
	}

	u = user_find_named(origin);

	if (!chanacs_user_has_flag(mc, u, CA_REMOVE))
	{
		notice(chansvs.nick, origin, "You are not authorized to perform this operation.");
		return;
	}

	/* figure out who we're going to kick */
	if (!(tu = user_find_named(nick)))
	{
		notice(chansvs.nick, origin, "\2%s\2 is not online.", nick);
		return;
	}

        /* if target is a service, bail. --nenolod */
	if (is_internal_client(tu))
                return;

	cu = chanuser_find(mc->chan, tu);
	if (!cu)
	{
		notice(chansvs.nick, origin, "\2%s\2 is not on \2%s\2.", tu->nick, mc->name);
		return;
	}

	snprintf(reasonbuf, BUFSIZE, "%s (%s)", reason ? reason : "No reason given", origin);
	kick(chansvs.nick, chan, tu->nick, reasonbuf);
	logcommand(chansvs.me, u, CMDLOG_SET, "%s KICK %s!%s@%s", mc->name, tu->nick, tu->user, tu->vhost);
}

static void cs_fcmd_kickban(char *origin, char *chan)
{
	char *nick = strtok(NULL, " ");
	char *reason = strtok(NULL, "");
	mychan_t *mc;
	user_t *u, *tu;
	chanuser_t *cu;
	char reasonbuf[BUFSIZE];
	int n;

	if (!chan || !nick)
	{
		notice(chansvs.nick, origin, STR_INSUFFICIENT_PARAMS, "KICKBAN");
		notice(chansvs.nick, origin, "Syntax: KICKBAN <#channel> <nickname> [reason]");
		return;
	}

	mc = mychan_find(chan);
	if (!mc)
	{
		notice(chansvs.nick, origin, "\2%s\2 is not registered.", chan);
		return;
	}

	u = user_find_named(origin);

	if (!chanacs_user_has_flag(mc, u, CA_REMOVE))
	{
		notice(chansvs.nick, origin, "You are not authorized to perform this operation.");
		return;
	}

	/* figure out who we're going to kick */
	if (!(tu = user_find_named(nick)))
	{
		notice(chansvs.nick, origin, "\2%s\2 is not online.", nick);
		return;
	}

        /* if target is a service, bail. --nenolod */
        if (is_internal_client(tu))
                return;

	cu = chanuser_find(mc->chan, tu);
	if (!cu)
	{
		notice(chansvs.nick, origin, "\2%s\2 is not on \2%s\2.", tu->nick, mc->name);
		return;
	}

	snprintf(reasonbuf, BUFSIZE, "%s (%s)", reason ? reason : "No reason given", origin);
	ban(chansvs.nick, chan, tu);
	n = remove_ban_exceptions(chansvs.me->me, mc->chan, tu);
	if (n > 0)
		notice(chansvs.nick, origin, "To avoid rejoin, %d ban exception(s) matching \2%s\2 have been removed from \2%s\2.", n, tu->nick, mc->name);
	kick(chansvs.nick, chan, tu->nick, reasonbuf);
	logcommand(chansvs.me, user_find_named(origin), CMDLOG_SET, "%s KICKBAN %s!%s@%s", mc->name, tu->nick, tu->user, tu->vhost);
}

