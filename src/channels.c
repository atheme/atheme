/*
 * atheme-services: A collection of minimalist IRC services   
 * channels.c: Channel event and state tracking
 *
 * Copyright (c) 2005-2007 Atheme Project (http://www.atheme.org)           
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "atheme.h"

mowgli_patricia_t *chanlist;

mowgli_heap_t *chan_heap;
mowgli_heap_t *chanuser_heap;
mowgli_heap_t *chanban_heap;

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
	chan_heap = mowgli_heap_create(sizeof(channel_t), HEAP_CHANNEL, BH_NOW);
	chanuser_heap = mowgli_heap_create(sizeof(chanuser_t), HEAP_CHANUSER, BH_NOW);
	chanban_heap = mowgli_heap_create(sizeof(chanban_t), HEAP_CHANUSER, BH_NOW);

	if (chan_heap == NULL || chanuser_heap == NULL || chanban_heap == NULL)
	{
		slog(LG_INFO, "init_channels(): block allocator failure.");
		exit(EXIT_FAILURE);
	}

	chanlist = mowgli_patricia_create(irccasecanon);
}

/*
 * channel_add(const char *name, time_t ts, server_t *creator)
 *
 * Channel object factory.
 *
 * Inputs:
 *     - channel name
 *     - timestamp of channel creation
 *     - server that is creating the channel
 *
 * Outputs:
 *     - on success, a channel object
 *     - on failure, NULL
 *
 * Side Effects:
 *     - the channel is automatically inserted into the channel DTree
 *     - if the creator is not me.me:
 *       - channel_add hook is called
 *       - all services are joined if this is the snoop channel
 *     - if the creator is me.me these actions must be performed by the
 *       caller (i.e. join()) after joining the service
 */
channel_t *channel_add(const char *name, time_t ts, server_t *creator)
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

	slog(LG_DEBUG, "channel_add(): %s by %s", name, creator->name);

	c = mowgli_heap_alloc(chan_heap);

	c->name = sstrdup(name);
	c->ts = ts;

	c->topic = NULL;
	c->topic_setter = NULL;

	if (ignore_mode_list_size != 0)
		c->extmodes = scalloc(sizeof(char *), ignore_mode_list_size);

	c->bans.head = NULL;
	c->bans.tail = NULL;
	c->bans.count = 0;

	if ((mc = mychan_find(c->name)))
		mc->chan = c;

	mowgli_patricia_add(chanlist, c->name, c);

	cnt.chan++;

	if (creator != me.me)
	{
		hook_call_channel_add(c);

		if (config_options.chan != NULL && !irccasecmp(config_options.chan, name))
			joinall(config_options.chan);
	}

	return c;
}

/*
 * channel_delete(channel_t *c)
 *
 * Destroys a channel object and its children member objects.
 *
 * Inputs:
 *     - channel object to destroy
 *
 * Outputs:
 *     - nothing
 *
 * Side Effects:
 *     - channel_delete hook is called
 *     - a channel and all attached structures are destroyed
 *     - no protocol messages are sent for any remaining members
 */
void channel_delete(channel_t *c)
{
	mychan_t *mc;
	mowgli_node_t *n, *tn;
	chanuser_t *cu;

	return_if_fail(c != NULL);

	slog(LG_DEBUG, "channel_delete(): %s", c->name);
	
	modestack_finalize_channel(c);

	/* If this is called from uplink_close(), there may still be services
	 * in the channel. Remove them. Calling chanuser_delete() could lead
	 * to a recursive call, so don't do that.
	 * -- jilles */
	MOWGLI_ITER_FOREACH_SAFE(n, tn, c->members.head)
	{
		cu = n->data;
		soft_assert(is_internal_client(cu->user) && !me.connected);
		mowgli_node_delete(&cu->cnode, &c->members);
		mowgli_node_delete(&cu->unode, &cu->user->channels);
		mowgli_heap_free(chanuser_heap, cu);
		cnt.chanuser--;
	}
	c->nummembers = 0;

	hook_call_channel_delete(c);

	mowgli_patricia_delete(chanlist, c->name);

	if ((mc = mychan_find(c->name)))
		mc->chan = NULL;

	clear_simple_modes(c);
	chanban_clear(c);

	if (c->extmodes != NULL)
		free(c->extmodes);

	free(c->name);
	if (c->topic != NULL)
		free(c->topic);
	if (c->topic_setter != NULL)
		free(c->topic_setter);

	mowgli_heap_free(chan_heap, c);

	cnt.chan--;
}

/*
 * chanban_add(channel_t *chan, const char *mask, int type)
 *
 * Channel ban factory.
 *
 * Inputs:
 *     - channel that the ban belongs to
 *     - banmask
 *     - type of ban, e.g. 'b' or 'e'
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

	if (mask == NULL)
	{
		slog(LG_ERROR, "chanban_add(): NULL +%c mask", type);
		return NULL;
	}
	/* this would break protocol and/or cause crashes */
	if (*mask == '\0' || *mask == ':' || strchr(mask, ' '))
	{
		slog(LG_ERROR, "chanban_add(): trying to add invalid +%c %s to channel %s", type, mask, chan->name);
		return NULL;
	}

	c = chanban_find(chan, mask, type);

	if (c)
	{
		slog(LG_DEBUG, "chanban_add(): channel ban %s:%s already exists", chan->name, c->mask);
		return NULL;
	}

	slog(LG_DEBUG, "chanban_add(): %s +%c %s", chan->name, type, mask);

	c = mowgli_heap_alloc(chanban_heap);

	c->chan = chan;
	c->mask = sstrdup(mask);
	c->type = type;

	mowgli_node_add(c, &c->node, &chan->bans);

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
	if (!c)
	{
		slog(LG_DEBUG, "chanban_delete(): called for nonexistant ban");
		return;
	}

	mowgli_node_delete(&c->node, &c->chan->bans);

	free(c->mask);
	mowgli_heap_free(chanban_heap, c);
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
	mowgli_node_t *n;

	MOWGLI_ITER_FOREACH(n, chan->bans.head)
	{
		c = n->data;

		if (c->type == type && !irccasecmp(c->mask, mask))
			return c;
	}

	return NULL;
}

/*
 * chanuser_add(channel_t *chan, const char *nick)
 *
 * Channel user factory.
 *
 * Inputs:
 *     - channel that the user should belong to
 *     - nick/UID with any appropriate prefixes (e.g. ~, &, @, %, +)
 *       (if the user has a UID it must be used)
 *
 * Outputs:
 *     - on success, a new channel user object
 *     - on failure (NULL channel, or user kicked by channel_join hook), NULL
 *
 * Side Effects:
 *     - the channel user object is automatically associated to its parents
 *     - channel_join hook is called
 */

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
chanuser_t *chanuser_add(channel_t *chan, const char *nick)
{
	user_t *u;
	chanuser_t *cu, *tcu;
	unsigned int flags = 0;
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

	cu = mowgli_heap_alloc(chanuser_heap);

	cu->chan = chan;
	cu->user = u;
	cu->modes |= flags;

	chan->nummembers++;

	mowgli_node_add(cu, &cu->cnode, &chan->members);
	mowgli_node_add(cu, &cu->unode, &u->channels);

	cnt.chanuser++;

	hdata.cu = cu;
	hook_call_channel_join(&hdata);

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
 *     if the user is on the channel:
 *     - a channel user object is removed from the
 *       channel's userlist and the user's channellist.
 *     - channel_part hook is called
 *     - if this empties the channel and the channel is not set permanent
 *       (ircd->perm_mode), channel_delete() is called (q.v.)
 */
void chanuser_delete(channel_t *chan, user_t *user)
{
	chanuser_t *cu;
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

	cu = chanuser_find(chan, user);
	if (cu == NULL)
		return;

	/* this is called BEFORE we remove the user */
	hdata.cu = cu;
	hook_call_channel_part(&hdata);

	slog(LG_DEBUG, "chanuser_delete(): %s -> %s (%d)", cu->chan->name, cu->user->nick, cu->chan->nummembers - 1);

	mowgli_node_delete(&cu->cnode, &chan->members);
	mowgli_node_delete(&cu->unode, &user->channels);

	mowgli_heap_free(chanuser_heap, cu);

	chan->nummembers--;
	cnt.chanuser--;

	if (chan->nummembers == 0 && !(chan->modes & ircd->perm_mode))
	{
		/* empty channels die */
		slog(LG_DEBUG, "chanuser_delete(): `%s' is empty, removing", chan->name);

		channel_delete(chan);
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
	mowgli_node_t *n;
	chanuser_t *cu;

	if ((!chan) || (!user))
		return NULL;

	/* choose shortest list to search -- jilles */
	if (MOWGLI_LIST_LENGTH(&user->channels) < MOWGLI_LIST_LENGTH(&chan->members))
	{
		MOWGLI_ITER_FOREACH(n, user->channels.head)
		{
			cu = (chanuser_t *)n->data;

			if (cu->chan == chan)
				return cu;
		}
	}
	else
	{
		MOWGLI_ITER_FOREACH(n, chan->members.head)
		{
			cu = (chanuser_t *)n->data;

			if (cu->user == user)
				return cu;
		}
	}

	return NULL;
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
