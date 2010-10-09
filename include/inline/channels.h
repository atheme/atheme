#ifndef INLINE_CHANNELS_H
#define INLINE_CHANNELS_H

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
static inline channel_t *channel_find(const char *name)
{
	return name ? mowgli_patricia_retrieve(chanlist, name) : NULL;
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
 *     - no protocol messages are sent
 */
static inline void chanban_clear(channel_t *chan)
{
	mowgli_node_t *n, *tn;

	MOWGLI_ITER_FOREACH_SAFE(n, tn, chan->bans.head)
	{
		/* inefficient but avoids code duplication -- jilles */
		chanban_delete(n->data);
	}
}

#endif
