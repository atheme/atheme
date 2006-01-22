/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains the main() routine.
 *
 * $Id: main.c 4655 2006-01-22 15:22:29Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/main", FALSE, _modinit, _moddeinit,
	"$Id: main.c 4655 2006-01-22 15:22:29Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_join(chanuser_t *cu);
static void cs_part(chanuser_t *cu);
static void cs_register(mychan_t *mc);
static void cs_keeptopic_newchan(channel_t *c);
static void cs_keeptopic_topicset(channel_t *c);
static void cs_leave_empty(void *unused);

list_t cs_cmdtree;
list_t cs_fcmdtree;
list_t cs_helptree;

E list_t mychan;

static void join_registered(boolean_t all)
{
	node_t *n;	uint32_t i;

	for (i = 0; i < HASHSIZE; i++)
	{
		LIST_FOREACH(n, mclist[i].head)
		{
			mychan_t *mc = n->data;

			if (all == TRUE)
			{
				join(mc->name, chansvs.nick);
				continue;
			}
			else if (mc->chan != NULL && mc->chan->members.count != 0)
			{
				join(mc->name, chansvs.nick);
				continue;
			}
		}
	}
}

/* main services client routine */
static void chanserv(char *origin, uint8_t parc, char *parv[])
{
	mychan_t *mc;
	char *cmd, *s;
	char orig[BUFSIZE];
	boolean_t is_fcommand = FALSE;
	hook_cmessage_data_t cdata;

        if (!origin)
        {
                slog(LG_DEBUG, "services(): recieved a request with no origin!");
                return;
        }

	/* this should never happen */
	if (parv[parc - 2][0] == '&')
	{
		slog(LG_ERROR, "services(): got parv with local channel: %s", parv[0]);
		return;
	}

	/* is this a fantasy command? */
	if (parv[parc - 2][0] == '#')
	{
		metadata_t *md;
		user_t *u = user_find_named(origin);

		if (chansvs.fantasy == FALSE)
		{
			/* *all* fantasy disabled */
			return;
		}

		mc = mychan_find(parv[parc - 2]);
		if (!mc)
		{
			/* unregistered, NFI how we got this message, but let's leave it alone! */
			return;
		}

		md = metadata_find(mc, METADATA_CHANNEL, "disable_fantasy");
		if (md)
		{
			/* fantasy disabled on this channel. don't message them, just bail. */
			return;
		}

		/* we're ok to go */
		is_fcommand = TRUE;
	}

	/* make a copy of the original for debugging */
	strlcpy(orig, parv[parc - 1], BUFSIZE);

	/* lets go through this to get the command */
	cmd = strtok(parv[parc - 1], " ");

	if (!cmd)
		return;

	/* ctcp? case-sensitive as per rfc */
	if (!strcmp(cmd, "\001PING"))
	{
		if (!(s = strtok(NULL, " ")))
			s = " 0 ";

		strip(s);
		notice(chansvs.nick, origin, "\001PING %s\001", s);
		return;
	}
	else if (!strcmp(cmd, "\001VERSION\001"))
	{
		notice(chansvs.nick, origin,
		       "\001VERSION atheme-%s. %s %s %s%s%s%s%s%s%s%s%s TS5ow\001",
		       version, revision, me.name,
		       (match_mapping) ? "A" : "",
		       (me.loglevel & LG_DEBUG) ? "d" : "",
		       (me.auth) ? "e" : "",
		       (config_options.flood_msgs) ? "F" : "",
		       (config_options.leave_chans) ? "l" : "",
		       (config_options.join_chans) ? "j" : "", (!match_mapping) ? "R" : "", (config_options.raw) ? "r" : "", (runflags & RF_LIVE) ? "n" : "");

		return;
	}
	else if (!strcmp(cmd, "\001CLIENTINFO\001"))
	{
		/* easter egg :X */
		notice(chansvs.nick, origin, "\001CLIENTINFO 114 97 107 97 117 114\001");
		return;
	}

	/* ctcps we don't care about are ignored */
	else if (*cmd == '\001')
		return;

	/* take the command through the hash table */
	if (!is_fcommand)
		command_exec(chansvs.me, origin, cmd, &cs_cmdtree);
	else
	{
		fcommand_exec_floodcheck(chansvs.me, parv[parc - 2], origin, cmd, &cs_fcmdtree);

		cdata.c = channel_find(parv[parc - 2]);
		cdata.msg = parv[parc - 1];
		hook_call_event("channel_message", &cdata);
	}
}

static void chanserv_config_ready(void *unused)
{
	if (chansvs.me)
		del_service(chansvs.me);

	chansvs.me = add_service(chansvs.nick, chansvs.user,
				 chansvs.host, chansvs.real, chanserv);
	chansvs.disp = chansvs.me->disp;

	hook_del_hook("config_ready", chanserv_config_ready);
}

void _modinit(module_t *m)
{
	hook_add_event("config_ready");
	hook_add_hook("config_ready", chanserv_config_ready);

	if (!cold_start)
	{
		chansvs.me = add_service(chansvs.nick, chansvs.user, chansvs.host, chansvs.real, chanserv);
		chansvs.disp = chansvs.me->disp;

		if (config_options.join_chans)
		{
			if (config_options.leave_chans)
				join_registered(FALSE);
			else
				join_registered(TRUE);
		}
	}

	hook_add_event("channel_join");
	hook_add_event("channel_part");
	hook_add_event("channel_register");
	hook_add_event("channel_add");
	hook_add_event("channel_topic");
	hook_add_hook("channel_join", (void (*)(void *)) cs_join);
	hook_add_hook("channel_part", (void (*)(void *)) cs_part);
	hook_add_hook("channel_register", (void (*)(void *)) cs_register);
	hook_add_hook("channel_add", (void (*)(void *)) cs_keeptopic_newchan);
	hook_add_hook("channel_topic", (void (*)(void *)) cs_keeptopic_topicset);
	event_add("cs_leave_empty", cs_leave_empty, NULL, 300);
}

void _moddeinit(void)
{
	if (chansvs.me)
	{
		del_service(chansvs.me);
		chansvs.me = NULL;
	}

	hook_del_hook("channel_join", (void (*)(void *)) cs_join);
	hook_del_hook("channel_part", (void (*)(void *)) cs_part);
	hook_del_hook("channel_register", (void (*)(void *)) cs_register);
	hook_del_hook("channel_add", (void (*)(void *)) cs_keeptopic_newchan);
	hook_del_hook("channel_topic", (void (*)(void *)) cs_keeptopic_topicset);
	event_delete(cs_leave_empty, NULL);
}

static void cs_join(chanuser_t *cu)
{
	user_t *u = cu->user;
	channel_t *chan = cu->chan;
	mychan_t *mc;
	char hostbuf[BUFSIZE];
	uint32_t flags;
	metadata_t *md;
	boolean_t noop;
	chanacs_t *ca2;

	if (is_internal_client(cu->user))
		return;

	/* first check if this is a registered channel at all */
	mc = mychan_find(chan->name);
	if (mc == NULL)
		return;

	if (chan->nummembers == 1 && config_options.join_chans)
		join(chan->name, chansvs.nick);

	flags = chanacs_user_flags(mc, u);
	noop = mc->flags & MC_NOOP || (u->myuser != NULL &&
			u->myuser->flags & MU_NOOP);

	/* auto stuff */
	if ((mc->flags & MC_STAFFONLY) && !has_priv(u, PRIV_JOIN_STAFFONLY))
	{
		/* Stay on channel if this would empty it -- jilles */
		if (chan->nummembers <= (config_options.join_chans ? 2 : 1))
		{
			mc->flags |= MC_INHABIT;
			if (!config_options.join_chans)
				join(chan->name, chansvs.nick);
		}
		ban(chansvs.nick, chan->name, u);
		remove_ban_exceptions(chansvs.me->me, chan, u);
		kick(chansvs.nick, chan->name, u->nick, "You are not authorized to be on this channel");
		return;
	}

	if (flags & CA_AKICK && !(flags & CA_REMOVE))
	{
		/* Stay on channel if this would empty it -- jilles */
		if (chan->nummembers <= (config_options.join_chans ? 2 : 1))
		{
			mc->flags |= MC_INHABIT;
			if (!config_options.join_chans)
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
				mode_sts(chansvs.nick, chan->name, str);
			}
		}
		else
			ban(chansvs.nick, chan->name, u);
		remove_ban_exceptions(chansvs.me->me, chan, u);
		kick(chansvs.nick, chan->name, u->nick, "User is banned from this channel");
		return;
	}

	/* A user joined and was not kicked; we do not need
	 * to stay on the channel artificially. */
	if (mc->flags & MC_INHABIT)
	{
		mc->flags &= ~MC_INHABIT;
		if (!config_options.join_chans && chanuser_find(chan, chansvs.me->me))
			part(chan->name, chansvs.nick);
	}

	if (ircd->uses_owner)
	{
		if (u->myuser != NULL && is_founder(mc, u->myuser))
		{
			if (!(noop || cu->modes & ircd->owner_mode))
			{
				cmode(chansvs.nick, chan->name, ircd->owner_mchar, CLIENT_NAME(u));
				cu->modes |= ircd->owner_mode;
			}
		}
		else if ((mc->flags & MC_SECURE) && (cu->modes & ircd->owner_mode))
		{
			char *mbuf = sstrdup(ircd->owner_mchar);
			*mbuf = '-';

			cmode(chansvs.nick, chan->name, mbuf, CLIENT_NAME(u));
			cu->modes &= ~ircd->owner_mode;

			free(mbuf);
		}
	}

	/* XXX still uses should_protect() */
	if (ircd->uses_protect && u->myuser != NULL)
	{
		if (should_protect(mc, u->myuser))
		{
			if (!(noop || cu->modes & ircd->protect_mode))
			{
				cmode(chansvs.nick, chan->name, ircd->protect_mchar, CLIENT_NAME(u));
				cu->modes |= ircd->protect_mode;
			}
		}
		else if ((mc->flags & MC_SECURE) && (cu->modes & ircd->protect_mode))
		{
			char *mbuf = sstrdup(ircd->protect_mchar);
			*mbuf = '-';

			cmode(chansvs.nick, chan->name, mbuf, CLIENT_NAME(u));
			cu->modes &= ~ircd->protect_mode;

			free(mbuf);
		}
	}

	if (flags & CA_AUTOOP)
	{
		if (!(noop || cu->modes & CMODE_OP))
		{
			cmode(chansvs.nick, chan->name, "+o", CLIENT_NAME(u));
			cu->modes |= CMODE_OP;
		}
	}
	else if ((mc->flags & MC_SECURE) && (cu->modes & CMODE_OP) && !(flags & CA_OP))
	{
		cmode(chansvs.nick, chan->name, "-o", CLIENT_NAME(u));
		cu->modes &= ~CMODE_OP;
	}

	if (ircd->uses_halfops)
	{
		if (flags & CA_AUTOHALFOP)
		{
			if (!(noop || cu->modes & (CMODE_OP | ircd->halfops_mode)))
			{
				cmode(chansvs.nick, chan->name, "+h", CLIENT_NAME(u));
				cu->modes |= ircd->halfops_mode;
			}
		}
		else if ((mc->flags & MC_SECURE) && (cu->modes & ircd->halfops_mode) && !(flags & CA_HALFOP))
		{
			cmode(chansvs.nick, chan->name, "-h", CLIENT_NAME(u));
			cu->modes &= ~ircd->halfops_mode;
		}
	}

	if (flags & CA_AUTOVOICE)
	{
		if (!(noop || cu->modes & (CMODE_OP | ircd->halfops_mode | CMODE_VOICE)))
		{
			cmode(chansvs.nick, chan->name, "+v", CLIENT_NAME(u));
			cu->modes |= CMODE_VOICE;
		}
	}

	if ((md = metadata_find(mc, METADATA_CHANNEL, "private:entrymsg")))
		notice(chansvs.nick, cu->user->nick, "[%s] %s", mc->name, md->value);

	if ((md = metadata_find(mc, METADATA_CHANNEL, "url")))
		numeric_sts(me.name, 328, cu->user->nick, "%s :%s", mc->name, md->value);

	check_modes(mc, TRUE);
	
	if (flags & CA_USEDUPDATE)
		mc->used = CURRTIME;
}

static void cs_part(chanuser_t *cu)
{
	mychan_t *mc;

	mc = mychan_find(cu->chan->name);
	if (mc == NULL)
		return;
	/*
	 * When channel_part is fired, we haven't yet removed the
	 * user from the room. So, the channel will have two members
	 * if ChanServ is joining channels: the triggering user and
	 * itself.
	 *
	 * Do not part if we're enforcing an akick/close in an otherwise
	 * empty channel (MC_INHABIT). -- jilles
	 */
	if (config_options.join_chans
		&& config_options.leave_chans
		&& !(mc->flags & MC_INHABIT)
		&& (cu->chan->nummembers <= 2)
		&& !is_internal_client(cu->user))
		part(cu->chan->name, chansvs.nick);
}

static void cs_register(mychan_t *mc)
{
	if (mc->chan)
	{
		if (config_options.join_chans)
			join(mc->name, chansvs.nick);

		check_modes(mc, TRUE);
	}
}

/* Called on set of a topic */
static void cs_keeptopic_topicset(channel_t *c)
{
	mychan_t *mc;
	metadata_t *md;
	char *text;

	mc = mychan_find(c->name);

	if (mc == NULL)
		return;

	md = metadata_find(mc, METADATA_CHANNEL, "private:topic:text");
	if (md != NULL)
	{
		if (c->topic != NULL && !strcmp(md->value, c->topic))
			return;
		metadata_delete(mc, METADATA_CHANNEL, "private:topic:text");
	}

	if (metadata_find(mc, METADATA_CHANNEL, "private:topic:setter"))
		metadata_delete(mc, METADATA_CHANNEL, "private:topic:setter");

	if (metadata_find(mc, METADATA_CHANNEL, "private:topic:ts"))
		metadata_delete(mc, METADATA_CHANNEL, "private:topic:ts");

	if (c->topic && c->topic_setter)
	{
		slog(LG_DEBUG, "KeepTopic: topic set for %s by %s: %s", c->name,
			c->topic_setter, c->topic);
		metadata_add(mc, METADATA_CHANNEL, "private:topic:setter",
			c->topic_setter);
		metadata_add(mc, METADATA_CHANNEL, "private:topic:text",
			c->topic);
		metadata_add(mc, METADATA_CHANNEL, "private:topic:ts",
			itoa(c->topicts));
	}
	else
		slog(LG_DEBUG, "KeepTopic: topic cleared for %s", c->name);
}

/* Called on creation of a channel */
static void cs_keeptopic_newchan(channel_t *c)
{
	mychan_t *mc;
	metadata_t *md;
	char *setter;
	char *text;
	time_t channelts;
	time_t topicts;


	if (!(mc = mychan_find(c->name)))
		return;

	if (!(MC_KEEPTOPIC & mc->flags))
		return;

#if 0
	md = metadata_find(mc, METADATA_CHANNEL, "private:channelts");
	if (md == NULL)
		return;
	channelts = atol(md->value);
	if (channelts == c->ts)
	{
		/* Same channel, let's assume ircd has kept it.
		 * Probably not a good assumption if the ircd doesn't do
		 * topic bursting.
		 * -- jilles
		 */
		slog(LG_DEBUG, "Not doing keeptopic for %s because of equal channelTS", c->name);
		return;
	}
#endif

	md = metadata_find(mc, METADATA_CHANNEL, "private:topic:setter");
	if (md == NULL)
		return;
	setter = md->value;

	md = metadata_find(mc, METADATA_CHANNEL, "private:topic:text");
	if (md == NULL)
		return;
	text = md->value;

	md = metadata_find(mc, METADATA_CHANNEL, "private:topic:ts");
	if (md == NULL)
		return;
	topicts = atol(md->value);

	handle_topic(c, setter, topicts, text);
	topic_sts(c->name, setter, topicts, text);
}

static void cs_leave_empty(void *unused)
{
	int i;
	node_t *n;
	mychan_t *mc;

	(void)unused;
	for (i = 0; i < HASHSIZE; i++)
	{
		LIST_FOREACH(n, mclist[i].head)
		{
			mc = n->data;
			if (!(mc->flags & MC_INHABIT))
				continue;
			mc->flags &= ~MC_INHABIT;
			if (mc->chan != NULL &&
					(!config_options.join_chans ||
					 (config_options.leave_chans && mc->chan->nummembers == 1) ||
					 metadata_find(mc, METADATA_CHANNEL, "private:close:closer")) &&
					chanuser_find(mc->chan, chansvs.me->me))
			{
				slog(LG_DEBUG, "cs_leave_empty(): leaving %s", mc->chan->name);
				part(mc->chan->name, chansvs.nick);
			}
		}
	}
}
