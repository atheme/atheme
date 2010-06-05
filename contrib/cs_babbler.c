/*
 * Copyright (c) 2008 William Pitcock
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Because sometimes premadonna assholes with large ignore lists
 * piss entire channels the hell off...
 *
 * So what does this do?
 * =====================
 *
 * It repeats everything someone says and to be extra annoying, highlights
 * the person who has public ignore notification spam.
 *
 * It was written for the purpose of mockery of someone on #atheme-project
 * who makes claims like "I have the whole channel on ignore", etc.
 *
 * Pro tip: we don't care about your ignore list.
 *
 * How do I use it? I have an asshole on my channel too!
 * =====================================================
 *
 * Load the module, set these options:
 *
 *  - babbler:enable to actually enable babbler
 *  - babbler:nicks, the actual ignore list of the asshole
 *  - babbler:target, the nick of the person who needs to be pwnt
 *  - babbler:source, the nick of a psuedoclient to send the message
 *    from.
 *
 * Will you make it PM them instead?
 * =================================
 *
 * Absolutely not. Then it could be used for spambots, etc. That's a really
 * bad idea.
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/babbler", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"William Pitcock <nenolod -at- nenolod.net>"
);

static void
on_channel_message(void *p)
{
	hook_cmessage_data_t *data = p;

	if (data != NULL && data->msg != NULL)
	{
		mychan_t *mc = mychan_find(data->c->name);
		metadata_t *md;

		if (!mc)
			return;

		if (!metadata_find(mc, "babbler:enable"))
			return;

		if (!(md = metadata_find(mc, "babbler:nicks")))
			return;

		if (strstr(md->value, data->u->nick))
		{
			char *source = NULL;
			char *target;

			if (!(md = metadata_find(mc, "babbler:target")))
				return;

			target = md->value;

			if (!(md = metadata_find(mc, "babbler:source")))
				source = chansvs.nick;
			else
				source = md->value;

			msg(source, data->c->name, "%s: <%s> %s", target, data->u->nick, data->msg);
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
