/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Closing for channels.
 *
 * $Id: close.c 2553 2005-10-04 06:33:01Z nenolod $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/close", FALSE, _modinit, _moddeinit,
	"$Id: close.c 2553 2005-10-04 06:33:01Z nenolod $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_close(char *origin);

/* CLOSE ON|OFF -- don't pollute the root with REOPEN */
command_t cs_close = { "CLOSE", "Closes a channel.",
			AC_IRCOP, cs_cmd_close };

static void close_can_leave(void *name);
static void close_check_join(chanuser_t *cu);

list_t *cs_cmdtree;
list_t *cs_helptree;

void _modinit(module_t *m)
{
	cs_cmdtree = module_locate_symbol("chanserv/main", "cs_cmdtree");
	cs_helptree = module_locate_symbol("chanserv/main", "cs_helptree");

	command_add(&cs_close, cs_cmdtree);
	hook_add_event("channel_join");
	hook_add_hook("channel_join", (void (*)(void *)) close_check_join);

	help_addentry(cs_helptree, "CLOSE", "help/cservice/close", NULL);
}

void _moddeinit()
{
	command_delete(&cs_close, cs_cmdtree);
	hook_del_hook("channel_join", (void (*)(void *)) close_check_join);
	help_delentry(cs_helptree, "CLOSE");
}

static void close_check_join(chanuser_t *cu)
{
	mychan_t *mc;

	if (!(mc = mychan_find(cu->chan->name)))
		return;

	if (metadata_find(mc, METADATA_CHANNEL, "private:close:closer"))
	{
	        char buf[BUFSIZE];

		/* clear the channel */

		/* don't join if we're already in there */
		if (!chanuser_find(cu->chan, user_find(chansvs.nick)))
			join(cu->chan->name, chansvs.nick);
		if (!chanuser_find(cu->chan, user_find(opersvs.nick)))
			join(cu->chan->name, opersvs.nick);

		/* lock it down */
		cmode(chansvs.nick, cu->chan->name, "+isbl", "*!*@*", "1");
		cu->chan->modes |= CMODE_INVITE;
		cu->chan->modes |= CMODE_SEC;
		cu->chan->modes |= CMODE_LIMIT;
		cu->chan->limit = 1;
		if (!chanban_find(cu->chan, "*!*@*"))
			chanban_add(cu->chan, "*!*@*");

		kick(chansvs.nick, cu->chan->name, cu->user->nick, "This channel has been closed");

		/* the agents should stay for a bit to stop rejoin floods */
		snprintf(buf, BUFSIZE, "close_can_leave_%s", cu->chan->name);
		event_add_once(buf, close_can_leave, cu->chan->name, 90);
	}
}

static void close_can_leave(void *name)
{
	channel_t *c;

	if ((c = channel_find((char *) name)))
	{
		if (chanuser_find(c, user_find(chansvs.nick)))
			part(name, chansvs.nick);
		if (chanuser_find(c, user_find(opersvs.nick)))
			part(name, opersvs.nick);

		event_delete(close_can_leave, name);
	}
}

static void cs_cmd_close(char *origin)
{
	char *target = strtok(NULL, " ");
	char *action = strtok(NULL, " ");
	char *reason = strtok(NULL, "");
	mychan_t *mc;
	channel_t *c;
	chanuser_t *cu;
	node_t *n;

	if (!target || !action)
	{
		notice(chansvs.nick, origin, "Insufficient parameters for \2CLOSE\2.");
		notice(chansvs.nick, origin, "Usage: CLOSE <#channel> <ON|OFF> [reason]");
		return;
	}

	if (!(mc = mychan_find(target)))
	{
		notice(chansvs.nick, origin, "\2%s\2 is not registered.", target);
		return;
	}

	if (!strcasecmp(action, "ON"))
	{
		if (!reason)
		{
			notice(chansvs.nick, origin, "Insufficient parameters for \2CLOSE\2.");
			notice(chansvs.nick, origin, "Usage: CLOSE <#channel> ON <reason>");
			return;
		}

		if (config_options.chan && !irccasecmp(config_options.chan, target))
		{
			notice(chansvs.nick, origin, "\2%s\2 cannot be closed.", target);
			return;
		}

		if (metadata_find(mc, METADATA_CHANNEL, "private:close:closer"))
		{
			notice(chansvs.nick, origin, "\2%s\2 is already closed.", target);
			return;
		}

		metadata_add(mc, METADATA_CHANNEL, "private:close:closer", origin);
		metadata_add(mc, METADATA_CHANNEL, "private:close:reason", reason);
		metadata_add(mc, METADATA_CHANNEL, "private:close:timestamp", itoa(CURRTIME));

		if ((c = channel_find(target)))
		{
		        char buf[BUFSIZE];

			/* clear the channel */
			if (!chanuser_find(c, user_find(chansvs.nick)))
				join(target, chansvs.nick);
			if (!chanuser_find(c, user_find(opersvs.nick)))
				join(target, opersvs.nick);

			/* lock it down */
			cmode(chansvs.nick, target, "+isbl", "*!*@*", "1");
			c->modes |= CMODE_INVITE;
			c->modes |= CMODE_SEC;
			c->modes |= CMODE_LIMIT;
			c->limit = 1;
			if (!chanban_find(c, "*!*@*"))
				chanban_add(c, "*!*@*");

			LIST_FOREACH(n, c->members.head)
			{
				cu = (chanuser_t *)n->data;

				kick(chansvs.nick, target, cu->user->nick, "This channel has been closed");
			}

			/* the agents should stay for a bit to stop rejoin floods */
			snprintf(buf, BUFSIZE, "close_can_leave_%s", target);
			event_add_once(buf, close_can_leave, target, 90);
		}

		wallops("%s closed the channel \2%s\2 (%s).", origin, target, reason);
		notice(chansvs.nick, origin, "\2%s\2 is now closed.", target);
	}
	else if (!strcasecmp(action, "OFF"))
	{
		if (!metadata_find(mc, METADATA_CHANNEL, "private:close:closer"))
		{
			notice(chansvs.nick, origin, "\2%s\2 is not closed.", target);
			return;
		}

		metadata_delete(mc, METADATA_CHANNEL, "private:close:closer");
		metadata_delete(mc, METADATA_CHANNEL, "private:close:reason");
		metadata_delete(mc, METADATA_CHANNEL, "private:close:timestamp");

		/* we should be the only ones in the channel, so parting will reset */
		close_can_leave(target);

		/* for paranoia --nenolod */
		event_delete(close_can_leave, mc->name);

		wallops("%s reopened the channel \2%s\2.", origin, target);
		notice(chansvs.nick, origin, "\2%s\2 has been reopened.", target);
	}
	else
	{
		notice(chansvs.nick, origin, "Insufficient parameters for \2CLOSE\2.");
		notice(chansvs.nick, origin, "Usage: CLOSE <#channel> <ON|OFF> [note]");
	}
}
