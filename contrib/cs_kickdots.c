/*
 * Copyright (c) 2006 William Pitcock
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Kicks people saying "..." on channels with "kickdots" metadata set.
 *
 * $Id: cs_kickdots.c 5790 2006-07-08 23:03:39Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/kickdots", FALSE, _modinit, _moddeinit,
	"$Id: cs_kickdots.c 5790 2006-07-08 23:03:39Z jilles $",
	"William Pitcock <nenolod -at- nenolod.net>"
);

static void
on_channel_message(void *p)
{
	hook_cmessage_data_t *data = p;

	if (data != NULL && data->msg != NULL && !strncmp(data->msg, "...", 3))
	{
		mychan_t *mc = mychan_find(data->c->name);

		if (metadata_find(mc, METADATA_CHANNEL, "kickdots"))
		{
			kick(chansvs.nick, data->c->name, data->u->nick, data->msg);
		}
	}
}

void _modinit(module_t *m)
{
	hook_add_event("channel_message");
	hook_add_hook("channel_message", on_channel_message);
}

void _moddeinit(void)
{
	hook_del_hook("channel_message", on_channel_message);
}
