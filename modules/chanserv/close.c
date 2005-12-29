/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Closing for channels.
 *
 * $Id: close.c 4341 2005-12-29 22:20:40Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/close", FALSE, _modinit, _moddeinit,
	"$Id: close.c 4341 2005-12-29 22:20:40Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_close(char *origin);

/* CLOSE ON|OFF -- don't pollute the root with REOPEN */
command_t cs_close = { "CLOSE", "Closes a channel.",
			PRIV_CHAN_ADMIN, cs_cmd_close };

static void close_can_leave(void *unused);
static void close_check_join(void *vcu);

list_t *cs_cmdtree;
list_t *cs_helptree;

list_t can_leave_list;

void _modinit(module_t *m)
{
	cs_cmdtree = module_locate_symbol("chanserv/main", "cs_cmdtree");
	cs_helptree = module_locate_symbol("chanserv/main", "cs_helptree");

	command_add(&cs_close, cs_cmdtree);
	hook_add_event("channel_join");
	hook_add_hook("channel_join", close_check_join);
	event_add("close_can_leave", close_can_leave, NULL, 180);
	help_addentry(cs_helptree, "CLOSE", "help/cservice/close", NULL);
}

void _moddeinit()
{
	command_delete(&cs_close, cs_cmdtree);
	hook_del_hook("channel_join", close_check_join);
	help_delentry(cs_helptree, "CLOSE");
	event_delete(close_can_leave, NULL);
}

static void close_check_join(void *vcu)
{
	mychan_t *mc;
	chanuser_t *cu;
	node_t *n;

	cu = vcu;

	if (is_internal_client(cu->user))
		return;

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
		LIST_FOREACH(n, can_leave_list.head)
		{
			if (!irccasecmp(n->data, mc->name))
				return;
		}
		node_add(sstrdup(mc->name), node_create(), &can_leave_list);
	}
}

#if 0
static void close_check_drop(void *vmc)
{
	mychan_t *mc;
	node_t *n;
	channel_t *c;

	mc = vmc;
	n = node_find(mc, &can_leave_list);
	if (n != NULL)
	{
		node_delete(n, &can_leave_list);
		if (chanuser_find(c, user_find(chansvs.nick)))
			part(c->name, chansvs.nick);
		if (chanuser_find(c, user_find(opersvs.nick)))
			part(c->name, opersvs.nick);
	}
}
#endif

static void close_can_leave(void *unused)
{
	char *name;
	node_t *n, *tn;
	mychan_t *mc;
	channel_t *c;

	(void)unused;
	LIST_FOREACH_SAFE(n, tn, can_leave_list.head)
	{
		name = n->data;
		node_del(n, &can_leave_list);
		node_free(n);
		mc = mychan_find(name);
		free(name);
		if (mc == NULL)
			continue;
		c = mc->chan;
		if (c == NULL)
			continue;
		if (!metadata_find(mc, METADATA_CHANNEL, "private:close:closer"))
		{
			slog(LG_DEBUG, "close_can_leave(): channel %s is not closed but was on can_leave_list", c->name);
			continue;
		}
		slog(LG_DEBUG, "close_can_leave(): leaving %s", c->name);
		if (chanuser_find(c, user_find(chansvs.nick)))
			part(c->name, chansvs.nick);
		if (chanuser_find(c, user_find(opersvs.nick)))
			part(c->name, opersvs.nick);
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
	node_t *n, *tn;
	boolean_t found;

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

				if (!is_internal_client(cu->user))
					kick(chansvs.nick, target, cu->user->nick, "This channel has been closed");
			}

			/* the agents should stay for a bit to stop rejoin floods */
			found = FALSE;
			LIST_FOREACH(n, can_leave_list.head)
			{
				if (!irccasecmp(n->data, mc->name))
				{
					found = TRUE;
					break;
				}
			}
			if (!found)
				node_add(sstrdup(mc->name), node_create(), &can_leave_list);
		}

		wallops("%s closed the channel \2%s\2 (%s).", origin, target, reason);
		logcommand(chansvs.me, user_find(origin), CMDLOG_ADMIN, "%s CLOSE ON %s", target, reason);
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
		c = channel_find(target);
		if (c != NULL)
			if (chanuser_find(c, user_find(chansvs.nick)))
				part(c->name, chansvs.nick);
		c = channel_find(target);
		if (c != NULL)
			if (chanuser_find(c, user_find(opersvs.nick)))
				part(c->name, opersvs.nick);
		c = channel_find(target);
		if (c != NULL)
		{
			chanban_t *cb;

			/* hmm, channel still exists, probably permanent? */
			cb = chanban_find(c, "*!*@*");
			if (cb != NULL)
			{
				chanban_delete(cb);
				cmode(chansvs.nick, target, "-b", "*!*@*");
			}
			cmode(chansvs.nick, target, "-isl");
			c->modes &= ~(CMODE_INVITE | CMODE_SEC | CMODE_LIMIT);
			c->limit = 0;
			check_modes(mc, TRUE);
		}

		LIST_FOREACH_SAFE(n, tn, can_leave_list.head)
		{
			if (!irccasecmp(n->data, mc->name))
			{
				node_del(n, &can_leave_list);
				node_free(n);
			}
		}

		wallops("%s reopened the channel \2%s\2.", origin, target);
		logcommand(chansvs.me, user_find(origin), CMDLOG_ADMIN, "%s CLOSE OFF", target);
		notice(chansvs.nick, origin, "\2%s\2 has been reopened.", target);
	}
	else
	{
		notice(chansvs.nick, origin, "Insufficient parameters for \2CLOSE\2.");
		notice(chansvs.nick, origin, "Usage: CLOSE <#channel> <ON|OFF> [reason]");
	}
}
