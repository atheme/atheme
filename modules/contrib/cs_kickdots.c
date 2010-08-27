/*
 * Copyright (c) 2006 William Pitcock
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Kicks people saying "..." on channels with "kickdots" metadata set.
 *
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/kickdots", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"William Pitcock <nenolod -at- nenolod.net>"
);

static void
on_channel_message(hook_cmessage_data_t *data)
{
	if (data != NULL && data->msg != NULL && !strncmp(data->msg, "...", 3))
	{
		mychan_t *mc = mychan_find(data->c->name);

		if (metadata_find(mc, "kickdots"))
		{
			kick(chansvs.me->me, data->c, data->u, data->msg);
		}
	}
}

void _modinit(module_t *m)
{
	hook_add_event("channel_message");
	hook_add_channel_message(on_channel_message);
}

void _moddeinit(void)
{
	hook_del_channel_message(on_channel_message);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
