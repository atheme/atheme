/*
 * Copyright (c) 2008 William Pitcock
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Because sometimes premadonna assholes with large ignore lists
 * piss entire channels the hell off...
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/kickdots", FALSE, _modinit, _moddeinit,
	"$Id: cs_kickdots.c 7785 2007-03-03 15:54:32Z pippijn $",
	"William Pitcock <nenolod -at- nenolod.net>"
);

static void
on_channel_message(void *p)
{
	hook_cmessage_data_t *data = p;

	if (data != NULL && data->msg != NULL && !strncmp(data->msg, "...", 3))
	{
		mychan_t *mc = mychan_find(data->c->name);
		metadata_t *md;

		if (!mc)
			return;

		if (!metadata_find(mc, METADATA_CHANNEL, "babbler:enable"))
			return;

		if (!(md = metadata_find(mc, METADATA_CHANNEL, "babbler:nicks")))
			return;

		if (strstr(md->value, data->u->nick))
		{
			char *source = NULL;
			char *target;
			char buf[BUFSIZE];

			if (!(md = metadata_find(mc, METADATA_CHANNEL, "babbler:target")))
				return;

			target = md->value;

			if (!(md = metadata_find(mc, METADATA_CHANNEL, "babbler:source")))
				source = chansvs.nick;
			else
				source = md->value;

			snprintf(buf, BUFSIZE, "%s: <%s> %s", target, data->u->nick, data->msg);

			msg(source, data->c->name, data->u->nick, buf);
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

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
