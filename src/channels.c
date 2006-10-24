/*
 * Copyright (c) 2005-2006 Atheme developers.
 * Rights to this code are documented in doc/LICENSE.
 *
 * Channel stuff.
 *
 * $Id: channels.c 6931 2006-10-24 16:53:07Z jilles $
 */

#include "atheme.h"

dictionary_tree_t *chanlist;

static BlockHeap *chan_heap;
static BlockHeap *chanuser_heap;
static BlockHeap *chanban_heap;

/*
 * init_channels()
 *
 * Initializes the channel-related heaps and DTree structures.
 *
 * Inputs:
 *     - nothing
 *
 * Outputs:
 *     - nothing
 *
 * Side Effects:
 *     - if the heaps or DTrees fail to initialize, the program will abort.
 */
void init_channels(void)
{
	chan_heap = BlockHeapCreate(sizeof(channel_t), HEAP_CHANNEL);
	chanuser_heap = BlockHeapCreate(sizeof(chanuser_t), HEAP_CHANUSER);
	chanban_heap = BlockHeapCreate(sizeof(chanban_t), HEAP_CHANUSER);

	if (chan_heap == NULL || chanuser_heap == NULL || chanban_heap == NULL)
	{
		slog(LG_INFO, "init_channels(): block allocator failure.");
		exit(EXIT_FAILURE);
	}

	chanlist = dictionary_create("channel", HASH_CHANNEL, irccasecmp);
}

/*
 * channel_add(const char *name, uint32_t ts)
 *
 * Channel object factory.
 *
 * Inputs:
 *     - channel name
 *     - timestamp of channel creation
 *
 * Outputs:
 *     - on success, a channel object
 *     - on failure, NULL
 *
 * Side Effects:
 *     - the channel is automatically inserted into the channel DTree.
 */
channel_t *channel_add(const char *name, uint32_t ts)
{
	channel_t *c;
	mychan_t *mc;

	if (*name != '#')
	{
		slog(LG_DEBUG, "channel_add(): got non #channel: %s", name);
		return NULL;
	}

	c = channel_find(name);

	if (c)
	{
		slog(LG_DEBUG, "channel_add(): channel already exists: %s", name);
		return c;
	}

	slog(LG_DEBUG, "channel_add(): %s", name);

	c = BlockHeapAlloc(chan_heap);

	c->name = sstrdup(name);
	c->ts = ts;

	c->topic = NULL;
	c->topic_setter = NULL;

	c->bans.head = NULL;
	c->bans.tail = NULL;
	c->bans.count = 0;

	if ((mc = mychan_find(c->name)))
		mc->chan = c;

	dictionary_add(chanlist, c->name, c);

	cnt.chan++;

	hook_call_event("channel_add", c);

	if (config_options.chan != NULL && !irccmp(config_options.chan, name))
		joinall(config_options.chan);

	return c;
}

/*
 * channel_delete(const char *name)
 *
 * Destroys a channel object and it's children member objects.
 *
 * Inputs:
 *     - name of channel object to find and destroy
 *
 * Outputs:
 *     - nothing
 *
 * Side Effects:
 *     - a channel and it's members are recursively destroyed
 */
void channel_delete(const char *name)
{
	channel_t *c = channel_find(name);
	mychan_t *mc;
	node_t *n, *tn, *n2;
	user_t *u;
	chanuser_t *cu;

	if (!c)
	{
		slog(LG_DEBUG, "channel_delete(): called for nonexistant channel: %s", name);
		return;
	}

	slog(LG_DEBUG, "channel_delete(): %s", c->name);

	/* channels with services may not be empty, kick them out -- jilles */
	LIST_FOREACH_SAFE(n, tn, c->members.head)
	{
		cu = n->data;
		u = cu->user;
		node_del(n, &c->members);
		node_free(n);
		n2 = node_find(cu, &u->channels);
		node_del(n2, &u->channels);
		node_free(n2);
		BlockHeapFree(chanuser_heap, cu);
		cnt.chanuser--;
	}
	c->nummembers = 0;

	hook_call_event("channel_delete", c);

	/* we assume all lists should be null */

	dictionary_delete(chanlist, c->name);

	if ((mc = mychan_find(c->name)))
		mc->chan = NULL;

	clear_simple_modes(c);

	free(c->name);
	BlockHeapFree(chan_heap, c);

	cnt.chan--;
}

/*
 * channel_find(const char *name)
 *
 * Looks up a channel object.
 *
 * Inputs:
 *     - name of channel to look up
 *
 * Outputs:
 *     - on success, the channel object
 *     - on failure, NULL
 *
 * Side Effects:
 *     - none
 */
channel_t *channel_find(const char *name)
{
	return dictionary_retrieve(chanlist, name);
}

/*
 * chanban_add(channel_t *chan, const char *mask, int type)
 *
 * Channel ban factory.
 *
 * Inputs:
 *     - channel that the ban belongs to
 *     - banmask
 *     - type of ban
 *
 * Outputs:
 *     - on success, a new channel ban object
 *     - on failure, NULL
 *
 * Side Effects:
 *     - the created channel ban object is added to the channel automatically.
 */
chanban_t *chanban_add(channel_t *chan, const char *mask, int type)
{
	chanban_t *c;
	node_t *n;

	c = chanban_find(chan, mask, type);

	if (c)
	{
		slog(LG_DEBUG, "chanban_add(): channel ban %s:%s already exists", chan->name, c->mask);
		return NULL;
	}

	slog(LG_DEBUG, "chanban_add(): %s +%c %s", chan->name, type, mask);

	n = node_create();
	c = BlockHeapAlloc(chanban_heap);

	c->chan = chan;
	c->mask = sstrdup(mask);
	c->type = type;

	node_add(c, n, &chan->bans);

	return c;
}

/*
 * chanban_delete(chanban_t *c)
 *
 * Destroys a channel ban.
 *
 * Inputs:
 *     - channel ban object to destroy
 *
 * Outputs:
 *     - nothing
 *
 * Side Effects:
 *     - the channel ban is automatically removed from the channel that owned it
 */
void chanban_delete(chanban_t * c)
{
	node_t *n;

	if (!c)
	{
		slog(LG_DEBUG, "chanban_delete(): called for nonexistant ban");
		return;
	}

	n = node_find(c, &c->chan->bans);
	node_del(n, &c->chan->bans);
	node_free(n);

	free(c->mask);
	BlockHeapFree(chanban_heap, c);
}

/*
 * chanban_find(channel_t *chan, const char *mask, int type)
 *
 * Looks up a channel ban.
 *
 * Inputs:
 *     - channel that the ban is supposedly on
 *     - mask that is being looked for
 *     - type of ban that is being looked for
 *
 * Outputs:
 *     - on success, returns the channel ban object requested
 *     - on failure, returns NULL
 *
 * Side Effects:
 *     - none
 */
chanban_t *chanban_find(channel_t *chan, const char *mask, int type)
{
	chanban_t *c;
	node_t *n;

	LIST_FOREACH(n, chan->bans.head)
	{
		c = n->data;

		if (c->type == type && !irccasecmp(c->mask, mask))
			return c;
	}

	return NULL;
}

/*
 * chanban_clear(channel_t *chan)
 *
 * Destroys all channel bans attached to a channel.
 *
 * Inputs:
 *     - channel to clear banlist on
 *
 * Outputs:
 *     - nothing
 *
 * Side Effects:
 *     - the banlist on the channel is cleared
 */
void chanban_clear(channel_t *chan)
{
	node_t *n, *tn;

	LIST_FOREACH_SAFE(n, tn, chan->bans.head)
	{
		/* inefficient but avoids code duplication -- jilles */
		chanban_delete(n->data);
	}
}

/*
 * Rewritten 06/23/05 by nenolod:
 *
 * Iterate through the list of prefix characters we know about.
 * Continue to do so until all prefixes are covered. Then add the
 * nick to the channel, with the privs he has acquired thus far.
 *
 * Once, and only once we have done that do we start in on checking
 * privileges. Otherwise we have a very inefficient way of doing
 * things. It worked fine for shrike, but the old code was restricted
 * to handling only @, @+ and + as prefixes.
 */

/*
 * chanuser_add(channel_t *chan, const char *nick)
 *
 * Channel user factory.
 *
 * Inputs:
 *     - channel that the user should belong to
 *     - nick with any appropriate prefixes (e.g. ~, &, @, %, +)
 *
 * Outputs:
 *     - on success, a new channel user object
 *     - on failure, NULL
 *
 * Side Effects:
 *     - the channel user object is automatically associated to it's parent.
 */
chanuser_t *chanuser_add(channel_t *chan, const char *nick)
{
	user_t *u;
	node_t *n1;
	node_t *n2;
	chanuser_t *cu, *tcu;
	uint32_t flags = 0;
	int i = 0;
	hook_channel_joinpart_t hdata;

	if (chan == NULL)
		return NULL;

	if (*chan->name != '#')
	{
		slog(LG_DEBUG, "chanuser_add(): got non #channel: %s", chan->name);
		return NULL;
	}

	while (*nick != '\0')
	{
		for (i = 0; prefix_mode_list[i].mode; i++)
			if (*nick == prefix_mode_list[i].mode)
			{
				flags |= prefix_mode_list[i].value;
				break;
			}
		if (!prefix_mode_list[i].mode)
			break;
		nick++;
	}

	u = user_find(nick);
	if (u == NULL)
	{
		slog(LG_DEBUG, "chanuser_add(): nonexist user: %s", nick);
		return NULL;
	}

	tcu = chanuser_find(chan, u);
	if (tcu != NULL)
	{
		slog(LG_DEBUG, "chanuser_add(): user is already present: %s -> %s", chan->name, u->nick);

		/* could be an OPME or other desyncher... */
		tcu->modes |= flags;

		return tcu;
	}

	slog(LG_DEBUG, "chanuser_add(): %s -> %s", chan->name, u->nick);

	n1 = node_create();
	n2 = node_create();

	cu = BlockHeapAlloc(chanuser_heap);

	cu->chan = chan;
	cu->user = u;
	cu->modes |= flags;

	chan->nummembers++;

	node_add(cu, n1, &chan->members);
	node_add(cu, n2, &u->channels);

	cnt.chanuser++;

	hdata.cu = cu;
	hook_call_event("channel_join", &hdata);

	/* Return NULL if a hook function kicked the user out */
	return hdata.cu;
}

/*
 * chanuser_delete(channel_t *chan, user_t *user)
 *
 * Destroys a channel user object.
 *
 * Inputs:
 *     - channel the user is on
 *     - the user itself
 *
 * Outputs:
 *     - nothing
 *
 * Side Effects:
 *     - on success, a channel user object is removed from the
 *       channel's userlist and the user's channellist.
 */
void chanuser_delete(channel_t *chan, user_t *user)
{
	chanuser_t *cu;
	node_t *n, *tn, *n2;
	hook_channel_joinpart_t hdata;

	if (!chan)
	{
		slog(LG_DEBUG, "chanuser_delete(): called with NULL chan");
		return;
	}

	if (!user)
	{
		slog(LG_DEBUG, "chanuser_delete(): called with NULL user");
		return;
	}

	LIST_FOREACH_SAFE(n, tn, chan->members.head)
	{
		cu = (chanuser_t *)n->data;

		if (cu->user == user)
		{
			/* this is called BEFORE we remove the user */
			hdata.cu = cu;
			hook_call_event("channel_part", &hdata);

			slog(LG_DEBUG, "chanuser_delete(): %s -> %s (%d)", cu->chan->name, cu->user->nick, cu->chan->nummembers - 1);
			node_del(n, &chan->members);
			node_free(n);

			n2 = node_find(cu, &user->channels);
			node_del(n2, &user->channels);
			node_free(n2);

			BlockHeapFree(chanuser_heap, cu);

			chan->nummembers--;
			cnt.chanuser--;

			if (chan->nummembers == 0 && !(chan->modes & ircd->perm_mode))
			{
				/* empty channels die */
				slog(LG_DEBUG, "chanuser_delete(): `%s' is empty, removing", chan->name);

				channel_delete(chan->name);
			}

			return;
		}
	}
}

/*
 * chanuser_find(channel_t *chan, user_t *user)
 *
 * Looks up a channel user object.
 *
 * Inputs:
 *     - channel object that the user is on
 *     - target user object
 *
 * Outputs:
 *     - on success, a channel user object
 *     - on failure, NULL
 *
 * Side Effects:
 *     - none
 */
chanuser_t *chanuser_find(channel_t *chan, user_t *user)
{
	node_t *n;
	chanuser_t *cu;

	if ((!chan) || (!user))
		return NULL;

	LIST_FOREACH(n, chan->members.head)
	{
		cu = (chanuser_t *)n->data;

		if (cu->user == user)
			return cu;
	}

	return NULL;
}
