#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/kickdots", FALSE, _modinit, _moddeinit,
	"$Id$",
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
