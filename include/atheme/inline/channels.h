/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2007 Pippijn van Steenhoven <pip88nl@gmail.com>
 */

#ifndef ATHEME_INC_INLINE_CHANNELS_H
#define ATHEME_INC_INLINE_CHANNELS_H 1

#include <atheme/channels.h>
#include <atheme/stdheaders.h>

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
static inline struct channel *channel_find(const char *name)
{
	return name ? mowgli_patricia_retrieve(chanlist, name) : NULL;
}

/*
 * chanban_clear(struct channel *chan)
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
static inline void chanban_clear(struct channel *chan)
{
	mowgli_node_t *n, *tn;

	MOWGLI_ITER_FOREACH_SAFE(n, tn, chan->bans.head)
	{
		/* inefficient but avoids code duplication -- jilles */
		chanban_delete(n->data);
	}
}

#endif /* !ATHEME_INC_INLINE_CHANNELS_H */
