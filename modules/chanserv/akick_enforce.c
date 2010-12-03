/*
 * Copyright (c) 2010 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Enforce AKICKs immediately when added.
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/akick_enforce", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void enforce_akicks_on_channel(chanacs_t *ca)
{
	char akickreason[120] = "User is banned from this channel", *p;
	channel_t *chan;
	unsigned int flags;
	metadata_t *md;
	mowgli_node_t *n, *tn;
	mychan_t *mc;

	mc = ca->mychan;
	return_if_fail(mc != NULL);

	chan = mc->chan;
	if (chan == NULL)
		return;

	MOWGLI_ITER_FOREACH_SAFE(n, tn, chan->members.head)
	{
		chanacs_t *ca2;
		chanuser_t *cu = n->data;
		user_t *u = cu->user;

		flags = chanacs_user_flags(mc, u);

		if (flags & CA_AKICK && !(flags & CA_REMOVE))
		{
			/* Stay on channel if this would empty it -- jilles */
			if (chan->nummembers <= (mc->flags & MC_GUARD ? 2 : 1))
			{
				mc->flags |= MC_INHABIT;
				if (!(mc->flags & MC_GUARD))
					join(chan->name, chansvs.nick);
			}

			/* use a user-given ban mask if possible -- jilles */
			ca2 = chanacs_find_host_by_user(mc, u, CA_AKICK);
			if (ca2 != NULL)
			{
				if (chanban_find(chan, ca2->host, 'b') == NULL)
				{
					char str[512];

					chanban_add(chan, ca2->host, 'b');
					snprintf(str, sizeof str, "+b %s", ca2->host);
					/* ban immediately */
					mode_sts(chansvs.nick, chan, str);
				}
			}
			else
			{
				/* XXX this could be done more efficiently */
				ca2 = chanacs_find(mc, entity(u->myuser), CA_AKICK);
				ban(chansvs.me->me, chan, u);
			}

			remove_ban_exceptions(chansvs.me->me, chan, u);
			if (ca2 != NULL)
			{
				md = metadata_find(ca2, "reason");
				if (md != NULL && *md->value != '|')
				{
					snprintf(akickreason, sizeof akickreason,
							"Banned: %s", md->value);
					p = strchr(akickreason, '|');
					if (p != NULL)
						*p = '\0';
					else
						p = akickreason + strlen(akickreason);

					/* strip trailing spaces, so as not to
					 * disclose the existence of an oper reason */
					p--;
					while (p > akickreason && *p == ' ')
						p--;
					p[1] = '\0';
				}
			}

			try_kick(chansvs.me->me, chan, u, akickreason);
		}
	}
}

void _modinit(module_t *m)
{
	hook_add_event("channel_acl_change");
	hook_add_channel_acl_change(enforce_akicks_on_channel);
}

void _moddeinit()
{
	hook_del_channel_acl_change(enforce_akicks_on_channel);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
