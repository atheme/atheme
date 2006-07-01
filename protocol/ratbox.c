/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains protocol support for ratbox-based ircd.
 *
 * $Id: ratbox.c 5628 2006-07-01 23:38:42Z jilles $
 */

#include "atheme.h"
#include "protocol/ratbox.h"

DECLARE_MODULE_V1("protocol/ratbox", TRUE, _modinit, NULL, "$Id: ratbox.c 5628 2006-07-01 23:38:42Z jilles $", "Atheme Development Group <http://www.atheme.org>");

/* *INDENT-OFF* */

ircd_t Ratbox = {
        "Ratbox (1.0 or later)",	/* IRCd name */
        "$$",                           /* TLD Prefix, used by Global. */
        TRUE,                           /* Whether or not we use IRCNet/TS6 UID */
        FALSE,                          /* Whether or not we use RCOMMAND */
        FALSE,                          /* Whether or not we support channel owners. */
        FALSE,                          /* Whether or not we support channel protection. */
        FALSE,                          /* Whether or not we support halfops. */
	FALSE,				/* Whether or not we use P10 */
	FALSE,				/* Whether or not we use vHosts. */
	0,				/* Oper-only cmodes */
        0,                              /* Integer flag for owner channel flag. */
        0,                              /* Integer flag for protect channel flag. */
        0,                              /* Integer flag for halfops. */
        "+",                            /* Mode we set for owner. */
        "+",                            /* Mode we set for protect. */
        "+",                            /* Mode we set for halfops. */
	PROTOCOL_RATBOX,		/* Protocol type */
	0,                              /* Permanent cmodes */
	"beI",                          /* Ban-like cmodes */
	'e',                            /* Except mchar */
	'I'                             /* Invex mchar */
};

struct cmode_ ratbox_mode_list[] = {
  { 'i', CMODE_INVITE },
  { 'm', CMODE_MOD    },
  { 'n', CMODE_NOEXT  },
  { 'p', CMODE_PRIV   },
  { 's', CMODE_SEC    },
  { 't', CMODE_TOPIC  },
  { '\0', 0 }
};

struct extmode ratbox_ignore_mode_list[] = {
  { '\0', 0 }
};

struct cmode_ ratbox_status_mode_list[] = {
  { 'o', CMODE_OP    },
  { 'v', CMODE_VOICE },
  { '\0', 0 }
};

struct cmode_ ratbox_prefix_mode_list[] = {
  { '@', CMODE_OP    },
  { '+', CMODE_VOICE },
  { '\0', 0 }
};

static boolean_t use_rserv_support = FALSE;
static boolean_t use_tb = FALSE;
static boolean_t use_rsfnc = FALSE;

static void server_eob(server_t *s);

static char ts6sid[3 + 1] = "";

/* *INDENT-ON* */

/* login to our uplink */
static uint8_t ratbox_server_login(void)
{
	int8_t ret = 1;

	if (!me.numeric)
	{
		ircd->uses_uid = FALSE;
		ret = sts("PASS %s :TS", curr_uplink->pass);
	}
	else if (strlen(me.numeric) == 3 && isdigit(*me.numeric))
	{
		ircd->uses_uid = TRUE;
		ret = sts("PASS %s TS 6 :%s", curr_uplink->pass, me.numeric);
	}
	else
	{
		slog(LG_ERROR, "Invalid numeric (SID) %s", me.numeric);
	}
	if (ret == 1)
		return 1;

	me.bursting = TRUE;

	sts("CAPAB :QS EX IE KLN UNKLN ENCAP TB SERVICES");
	sts("SERVER %s 1 :%s", me.name, me.desc);
	sts("SVINFO %d 3 0 :%ld", ircd->uses_uid ? 6 : 5, CURRTIME);

	return 0;
}

/* introduce a client */
static void ratbox_introduce_nick(char *nick, char *user, char *host, char *real, char *uid)
{
	if (ircd->uses_uid)
		sts(":%s UID %s 1 %ld +%s%s%s %s %s 0 %s :%s",
			me.numeric, nick, CURRTIME, "io",
			chansvs.fantasy ? "" : "D",
			use_rserv_support ? "S" : "", user, host, uid, real);
	else
		sts("NICK %s 1 %ld +%s%s%s %s %s %s :%s",
			nick, CURRTIME, "io", chansvs.fantasy ? "" : "D",
			use_rserv_support ? "S" : "",
			user, host, me.name, real);
}

/* invite a user to a channel */
static void ratbox_invite_sts(user_t *sender, user_t *target, channel_t *channel)
{
	/* some older TSora ircds require the sender to be
	 * on the channel, but hyb7/ratbox don't
	 * let's just assume it's not necessary -- jilles */
	sts(":%s INVITE %s %s", CLIENT_NAME(sender), CLIENT_NAME(target), channel->name);
}

static void ratbox_quit_sts(user_t *u, char *reason)
{
	if (!me.connected)
		return;

	sts(":%s QUIT :%s", CLIENT_NAME(u), reason);
}

/* WALLOPS wrapper */
static void ratbox_wallops(char *fmt, ...)
{
	va_list ap;
	char buf[BUFSIZE];

	if (config_options.silent)
		return;

	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZE, fmt, ap);
	va_end(ap);

	sts(":%s WALLOPS :%s", ME, buf);
}

/* join a channel */
static void ratbox_join_sts(channel_t *c, user_t *u, boolean_t isnew, char *modes)
{
	if (isnew)
		sts(":%s SJOIN %ld %s %s :@%s", ME, c->ts, c->name,
				modes, CLIENT_NAME(u));
	else
		sts(":%s SJOIN %ld %s + :@%s", ME, c->ts, c->name,
				CLIENT_NAME(u));
}

static void ratbox_chan_lowerts(channel_t *c, user_t *u)
{
	slog(LG_DEBUG, "ratbox_chan_lowerts(): lowering TS for %s to %ld",
			c->name, (long)c->ts);
	sts(":%s SJOIN %ld %s %s :@%s", ME, c->ts, c->name,
				channel_modes(c, TRUE), CLIENT_NAME(u));
	if (ircd->uses_uid)
		chanban_clear(c);
}

/* kicks a user from a channel */
static void ratbox_kick(char *from, char *channel, char *to, char *reason)
{
	channel_t *chan = channel_find(channel);
	user_t *user = user_find(to);
	user_t *from_p = user_find(from);

	if (!chan || !user)
		return;

	sts(":%s KICK %s %s :%s", CLIENT_NAME(from_p), channel, CLIENT_NAME(user), reason);

	chanuser_delete(chan, user);
}

/* PRIVMSG wrapper */
static void ratbox_msg(char *from, char *target, char *fmt, ...)
{
	va_list ap;
	char buf[BUFSIZE];
	user_t *u = user_find(from);
	user_t *t = user_find(target);

	if (!u)
		return;

	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZE, fmt, ap);
	va_end(ap);

	/* If this is to a channel, it's the snoop channel so chanserv
	 * is on it -- jilles
	 *
	 * Well, now it's operserv, but yes it's safe to assume that
	 * the source would be able to send to whatever target it is 
	 * sending to. --nenolod
	 */
	sts(":%s PRIVMSG %s :%s", CLIENT_NAME(u), t ? CLIENT_NAME(t) : target, buf);
}

/* NOTICE wrapper */
static void ratbox_notice(char *from, char *target, char *fmt, ...)
{
	va_list ap;
	char buf[BUFSIZE];
	user_t *u = user_find(from);
	user_t *t = user_find(target);

	if (u == NULL && (from == NULL || (irccasecmp(from, me.name) && irccasecmp(from, ME))))
	{
		slog(LG_DEBUG, "ratbox_notice(): unknown source %s for notice to %s", from, target);
		return;
	}

	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZE, fmt, ap);
	va_end(ap);

	if (u == NULL || target[0] != '#' || chanuser_find(channel_find(target), u))
		sts(":%s NOTICE %s :%s", u ? CLIENT_NAME(u) : ME, t ? CLIENT_NAME(t) : target, buf);
	else
		/* not on channel, let's send it from the server
		 * hyb6 won't accept this, oh well, they'll have to
		 * enable join_chans -- jilles */
		sts(":%s NOTICE %s :%s: %s", ME, target, u->nick, buf);
}

static void ratbox_wallchops(user_t *sender, channel_t *channel, char *message)
{
	if (chanuser_find(channel, sender))
		sts(":%s NOTICE @%s :%s", CLIENT_NAME(sender), channel->name,
				message);
	else /* do not join for this, everyone would see -- jilles */
		generic_wallchops(sender, channel, message);
}

/* numeric wrapper */
static void ratbox_numeric_sts(char *from, int numeric, char *target, char *fmt, ...)
{
	va_list ap;
	char buf[BUFSIZE];
	user_t *t = user_find(target);

	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZE, fmt, ap);
	va_end(ap);

	sts(":%s %d %s %s", ME, numeric, CLIENT_NAME(t), buf);
}

/* KILL wrapper */
static void ratbox_skill(char *from, char *nick, char *fmt, ...)
{
	va_list ap;
	char buf[BUFSIZE];
	user_t *killer = user_find(from);
	user_t *victim = user_find(nick);

	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZE, fmt, ap);
	va_end(ap);

	sts(":%s KILL %s :%s!%s!%s (%s)", killer ? CLIENT_NAME(killer) : ME, victim ? CLIENT_NAME(victim) : nick, from, from, from, buf);
}

/* PART wrapper */
static void ratbox_part(char *chan, char *nick)
{
	user_t *u = user_find(nick);
	channel_t *c = channel_find(chan);
	chanuser_t *cu;

	if (!u || !c)
		return;

	if (!(cu = chanuser_find(c, u)))
		return;

	sts(":%s PART %s", CLIENT_NAME(u), c->name);

	chanuser_delete(c, u);
}

/* server-to-server KLINE wrapper */
static void ratbox_kline_sts(char *server, char *user, char *host, long duration, char *reason)
{
	if (!me.connected)
		return;

	sts(":%s KLINE %s %ld %s %s :%s", CLIENT_NAME(opersvs.me->me), server, duration, user, host, reason);
}

/* server-to-server UNKLINE wrapper */
static void ratbox_unkline_sts(char *server, char *user, char *host)
{
	if (!me.connected)
		return;

	sts(":%s UNKLINE %s %s %s", CLIENT_NAME(opersvs.me->me), server, user, host);
}

/* topic wrapper */
static void ratbox_topic_sts(char *channel, char *setter, time_t ts, char *topic)
{
	channel_t *c;
	int joined = 0;

	c = channel_find(channel);
	if (!me.connected || !c)
		return;

	/* If restoring an older topic, try to use TB -- jilles */
	if (use_tb && ts < CURRTIME && *topic != '\0')
	{
		sts(":%s TB %s %ld %s :%s", ME, channel, ts, setter, topic);
		return;
	}
	/* We have to be on channel to change topic.
	 * We cannot nicely change topic from the server:
	 * :server.name TOPIC doesn't propagate and TB requires
	 * us to specify an older topicts.
	 * -- jilles
	 */
	if (!chanuser_find(c, chansvs.me->me))
	{
		sts(":%s SJOIN %ld %s + :@%s", ME, c->ts, channel, CLIENT_NAME(chansvs.me->me));
		joined = 1;
	}
	sts(":%s TOPIC %s :%s", CLIENT_NAME(chansvs.me->me), channel, topic);
	if (joined)
		sts(":%s PART %s :Topic set", CLIENT_NAME(chansvs.me->me), channel);
}

/* mode wrapper */
static void ratbox_mode_sts(char *sender, char *target, char *modes)
{
	user_t *u = user_find(sender);
	if (!me.connected)
		return;

	sts(":%s MODE %s %s", CLIENT_NAME(u), target, modes);
}

/* ping wrapper */
static void ratbox_ping_sts(void)
{
	if (!me.connected)
		return;

	sts("PING :%s", me.name);
}

/* protocol-specific stuff to do on login */
static void ratbox_on_login(char *origin, char *user, char *wantedhost)
{
	user_t *u = user_find(origin);

	if (!me.connected || !use_rserv_support || !u)
		return;

	sts(":%s ENCAP * SU %s %s", ME, CLIENT_NAME(u), user);
}

/* protocol-specific stuff to do on login */
static boolean_t ratbox_on_logout(char *origin, char *user, char *wantedhost)
{
	user_t *u = user_find(origin);

	if (!me.connected || !use_rserv_support || !u)
		return FALSE;

	sts(":%s ENCAP * SU %s", ME, CLIENT_NAME(u));
	return FALSE;
}

static void ratbox_jupe(char *server, char *reason)
{
	if (!me.connected)
		return;

	sts(":%s SQUIT %s :%s", CLIENT_NAME(opersvs.me->me), server, reason);
	sts(":%s SERVER %s 2 :%s", me.name, server, reason);
}

static void ratbox_fnc_sts(user_t *source, user_t *u, char *newnick, int type)
{
	if (use_rsfnc)
		/* XXX assumes whole net has it -- jilles */
		sts(":%s ENCAP %s RSFNC %s %s %lu %lu", ME,
			u->server->name,
			CLIENT_NAME(u), newnick,
			(unsigned long)(CURRTIME - 60),
			(unsigned long)u->ts);
	else
		generic_fnc_sts(source, u, newnick, type);
}

static void m_topic(char *origin, uint8_t parc, char *parv[])
{
	channel_t *c = channel_find(parv[0]);
	user_t *u = user_find(origin);

	if (!c || !u)
		return;

	handle_topic(c, u->nick, CURRTIME, parv[1]);
}

static void m_tb(char *origin, uint8_t parc, char *parv[])
{
	channel_t *c = channel_find(parv[0]);
	time_t ts = atol(parv[1]);
	server_t *source = server_find(origin);

	if (source == NULL)
		source = server_find(me.actual);
	if (source == NULL)
		source = me.me;

	handle_topic(c, parc > 3 ? parv[2] : source->name, ts, parv[parc - 1]);
}

static void m_ping(char *origin, uint8_t parc, char *parv[])
{
	/* reply to PING's */
	sts(":%s PONG %s %s", ME, me.name, parv[0]);
}

static void m_pong(char *origin, uint8_t parc, char *parv[])
{
	server_t *s;

	/* someone replied to our PING */
	if (!parv[0])
		return;
	s = server_find(parv[0]);
	if (s == NULL)
		return;
	handle_eob(s);

	if (irccasecmp(me.actual, parv[0]))
		return;

	me.uplinkpong = CURRTIME;

	/* -> :test.projectxero.net PONG test.projectxero.net :shrike.malkier.net */
	if (me.bursting)
	{
#ifdef HAVE_GETTIMEOFDAY
		e_time(burstime, &burstime);

		slog(LG_INFO, "m_pong(): finished synching with uplink (%d %s)", (tv2ms(&burstime) > 1000) ? (tv2ms(&burstime) / 1000) : tv2ms(&burstime), (tv2ms(&burstime) > 1000) ? "s" : "ms");

		wallops("Finished synching to network in %d %s.", (tv2ms(&burstime) > 1000) ? (tv2ms(&burstime) / 1000) : tv2ms(&burstime), (tv2ms(&burstime) > 1000) ? "s" : "ms");
#else
		slog(LG_INFO, "m_pong(): finished synching with uplink");
		wallops("Finished synching to network.");
#endif

		me.bursting = FALSE;
	}
}

static void m_privmsg(char *origin, uint8_t parc, char *parv[])
{
	if (parc != 2)
		return;

	handle_message(origin, parv[0], FALSE, parv[1]);
}

static void m_notice(char *origin, uint8_t parc, char *parv[])
{
	if (parc != 2)
		return;

	handle_message(origin, parv[0], TRUE, parv[1]);
}

static void m_sjoin(char *origin, uint8_t parc, char *parv[])
{
	/* -> :proteus.malkier.net SJOIN 1073516550 #shrike +tn :@sycobuny @+rakaur */

	channel_t *c;
	boolean_t keep_new_modes = TRUE;
	uint8_t userc;
	char *userv[256];
	uint8_t i;
	time_t ts;
	server_t *source_server;
	char *p;

	if (origin)
	{
		/* :origin SJOIN ts chan modestr [key or limits] :users */
		source_server = server_find(origin);
		if (source_server == NULL)
			return;

		c = channel_find(parv[1]);
		ts = atol(parv[0]);

		if (!c)
		{
			slog(LG_DEBUG, "m_sjoin(): new channel: %s", parv[1]);
			c = channel_add(parv[1], ts);
		}

		if (ts == 0 || c->ts == 0)
		{
			if (c->ts != 0)
				slog(LG_INFO, "m_sjoin(): server %s changing TS on %s from %ld to 0", source_server->name, c->name, (long)c->ts);
			c->ts = 0;
			hook_call_event("channel_tschange", c);
		}
		else if (ts < c->ts)
		{
			chanuser_t *cu;
			node_t *n;

			/* the TS changed.  a TS change requires the following things
			 * to be done to the channel:  reset all modes to nothing, remove
			 * all status modes on known users on the channel (including ours),
			 * and set the new TS.
			 *
			 * if the source does TS6, also remove all bans
			 * note that JOIN does not do this
			 */

			clear_simple_modes(c);
			if (source_server->sid != NULL)
				chanban_clear(c);

			LIST_FOREACH(n, c->members.head)
			{
				cu = (chanuser_t *)n->data;
				if (cu->user->server == me.me)
				{
					/* it's a service, reop */
					sts(":%s PART %s :Reop", CLIENT_NAME(cu->user), c->name);
					sts(":%s SJOIN %ld %s + :@%s", ME, ts, c->name, CLIENT_NAME(cu->user));
					cu->modes = CMODE_OP;
				}
				else
					cu->modes = 0;
			}

			slog(LG_INFO, "m_sjoin(): TS changed for %s (%ld -> %ld)", c->name, c->ts, ts);

			c->ts = ts;
			hook_call_event("channel_tschange", c);
		}
		else if (ts > c->ts)
			keep_new_modes = FALSE;

		if (keep_new_modes)
			channel_mode(NULL, c, parc - 3, parv + 2);

		userc = sjtoken(parv[parc - 1], ' ', userv);

		if (keep_new_modes)
			for (i = 0; i < userc; i++)
				chanuser_add(c, userv[i]);
		else
			for (i = 0; i < userc; i++)
			{
				p = userv[i];
				while (*p == '@' || *p == '%' || *p == '+')
					p++;
				/* XXX for TS5 we should mark them deopped
				 * if they were opped and drop modes from them
				 * -- jilles */
				chanuser_add(c, p);
			}
	}
}

static void m_join(char *origin, uint8_t parc, char *parv[])
{
	/* -> :1JJAAAAAB JOIN 1127474195 #test +tn */
	user_t *u = user_find(origin);
	boolean_t keep_new_modes = TRUE;
	node_t *n, *tn;
	channel_t *c;
	chanuser_t *cu;
	uint8_t i;
	time_t ts;

	if (!u)
		return;

	/* JOIN 0 is really a part from all channels */
	/* be sure to allow joins to TS 0 channels -- jilles */
	if (parv[0][0] == '0' && parc <= 2)
	{
		LIST_FOREACH_SAFE(n, tn, u->channels.head)
		{
			cu = (chanuser_t *)n->data;
			chanuser_delete(cu->chan, u);
		}
		return;
	}

	/* :user JOIN ts chan modestr [key or limits] */
	c = channel_find(parv[1]);
	ts = atol(parv[0]);

	if (!c)
	{
		slog(LG_DEBUG, "m_join(): new channel: %s", parv[1]);
		c = channel_add(parv[1], ts);
	}

	if (ts == 0 || c->ts == 0)
	{
		if (c->ts != 0)
			slog(LG_INFO, "m_join(): server %s changing TS on %s from %ld to 0", u->server->name, c->name, (long)c->ts);
		c->ts = 0;
		hook_call_event("channel_tschange", c);
	}
	else if (ts < c->ts)
	{
		/* the TS changed.  a TS change requires the following things
		 * to be done to the channel:  reset all modes to nothing, remove
		 * all status modes on known users on the channel (including ours),
		 * and set the new TS.
		 */
		clear_simple_modes(c);

		LIST_FOREACH(n, c->members.head)
		{
			cu = (chanuser_t *)n->data;
			if (cu->user->server == me.me)
			{
				/* it's a service, reop */
				sts(":%s PART %s :Reop", CLIENT_NAME(cu->user), c->name);
				sts(":%s SJOIN %ld %s + :@%s", ME, ts, c->name, CLIENT_NAME(cu->user));
				cu->modes = CMODE_OP;
			}
			else
				cu->modes = 0;
		}
		slog(LG_INFO, "m_join(): TS changed for %s (%ld -> %ld)", c->name, c->ts, ts);
		c->ts = ts;
		hook_call_event("channel_tschange", c);
	}
	else if (ts > c->ts)
		keep_new_modes = FALSE;

	if (keep_new_modes)
		channel_mode(NULL, c, parc - 2, parv + 2);

	chanuser_add(c, origin);
}

static void m_bmask(char *origin, uint8_t parc, char *parv[])
{
	uint8_t ac, i;
	char *av[256];
	channel_t *c = channel_find(parv[1]);
	int type;

	/* :1JJ BMASK 1127474361 #services b :*!*@*evil* *!*eviluser1@* */
	if (!c)
	{
		slog(LG_DEBUG, "m_bmask(): got bmask for unknown channel");
		return;
	}

	if (atol(parv[0]) > c->ts)
		return;
	
	type = *parv[2];
	if (!strchr(ircd->ban_like_modes, type))
	{
		slog(LG_DEBUG, "m_bmask(): got unknown type '%c'", type);
		return;
	}

	ac = sjtoken(parv[parc - 1], ' ', av);

	for (i = 0; i < ac; i++)
		chanban_add(c, av[i], type);
}

static void m_part(char *origin, uint8_t parc, char *parv[])
{
	uint8_t chanc;
	char *chanv[256];
	int i;

	if (parc < 1)
		return;
	chanc = sjtoken(parv[0], ',', chanv);
	for (i = 0; i < chanc; i++)
	{
		slog(LG_DEBUG, "m_part(): user left channel: %s -> %s", origin, chanv[i]);

		chanuser_delete(channel_find(chanv[i]), user_find(origin));
	}
}

static void m_nick(char *origin, uint8_t parc, char *parv[])
{
	server_t *s;
	user_t *u;

	/* got the right number of args for an introduction? */
	if (parc == 8)
	{
		s = server_find(parv[6]);
		if (!s)
		{
			slog(LG_DEBUG, "m_nick(): new user on nonexistant server: %s", parv[6]);
			return;
		}

		slog(LG_DEBUG, "m_nick(): new user on `%s': %s", s->name, parv[0]);

		u = user_add(parv[0], parv[4], parv[5], NULL, NULL, NULL, parv[7], s, atoi(parv[2]));

		user_mode(u, parv[3]);

		/* If server is not yet EOB we will do this later.
		 * This avoids useless "please identify" -- jilles */
		if (s->flags & SF_EOB)
			handle_nickchange(user_find(parv[0]));
	}

	/* if it's only 2 then it's a nickname change */
	else if (parc == 2)
	{
		node_t *n;

		u = user_find(origin);
		if (!u)
		{
			slog(LG_DEBUG, "m_nick(): nickname change from nonexistant user: %s", origin);
			return;
		}

		slog(LG_DEBUG, "m_nick(): nickname change from `%s': %s", u->nick, parv[0]);

		/* remove the current one from the list */
		n = node_find(u, &userlist[u->hash]);
		node_del(n, &userlist[u->hash]);
		node_free(n);

		/* change the nick */
		strlcpy(u->nick, parv[0], NICKLEN);
		u->ts = atoi(parv[1]);

		/* readd with new nick (so the hash works) */
		n = node_create();
		u->hash = UHASH((unsigned char *)u->nick);
		node_add(u, n, &userlist[u->hash]);

		/* It could happen that our PING arrived late and the
		 * server didn't acknowledge EOB yet even though it is
		 * EOB; don't send double notices in that case -- jilles */
		if (u->server->flags & SF_EOB)
			handle_nickchange(u);
	}
	else
	{
		int i;
		slog(LG_DEBUG, "m_nick(): got NICK with wrong number of params");

		for (i = 0; i < parc; i++)
			slog(LG_DEBUG, "m_nick():   parv[%d] = %s", i, parv[i]);
	}
}

static void m_uid(char *origin, uint8_t parc, char *parv[])
{
	server_t *s;
	user_t *u;

	/* got the right number of args for an introduction? */
	if (parc == 9)
	{
		s = server_find(origin);
		if (!s)
		{
			slog(LG_DEBUG, "m_uid(): new user on nonexistant server: %s", origin);
			return;
		}

		slog(LG_DEBUG, "m_uid(): new user on `%s': %s", s->name, parv[0]);

		u = user_add(parv[0], parv[4], parv[5], NULL, parv[6], parv[7], parv[8], s, atoi(parv[2]));

		user_mode(u, parv[3]);

		/* If server is not yet EOB we will do this later.
		 * This avoids useless "please identify" -- jilles
		 */
		if (s->flags & SF_EOB)
			handle_nickchange(user_find(parv[0]));
	}
	else
	{
		int i;
		slog(LG_DEBUG, "m_uid(): got UID with wrong number of params");

		for (i = 0; i < parc; i++)
			slog(LG_DEBUG, "m_uid():   parv[%d] = %s", i, parv[i]);
	}
}

static void m_quit(char *origin, uint8_t parc, char *parv[])
{
	slog(LG_DEBUG, "m_quit(): user leaving: %s", origin);

	/* user_delete() takes care of removing channels and so forth */
	user_delete(user_find(origin));
}

static void m_mode(char *origin, uint8_t parc, char *parv[])
{
	if (!origin)
	{
		slog(LG_DEBUG, "m_mode(): received MODE without origin");
		return;
	}

	if (parc < 2)
	{
		slog(LG_DEBUG, "m_mode(): missing parameters in MODE");
		return;
	}

	if (*parv[0] == '#')
		channel_mode(NULL, channel_find(parv[0]), parc - 1, &parv[1]);
	else
		user_mode(user_find(parv[0]), parv[1]);
}

static void m_tmode(char *origin, uint8_t parc, char *parv[])
{
	channel_t *c;

	/* -> :1JJAAAAAB TMODE 1127511579 #new +o 2JJAAAAAB */
	if (!origin)
	{
		slog(LG_DEBUG, "m_tmode(): received TMODE without origin");
		return;
	}

	if (parc < 3)
	{
		slog(LG_DEBUG, "m_tmode(): missing parameters in TMODE");
		return;
	}

	c = channel_find(parv[1]);
	if (c == NULL)
	{
		slog(LG_DEBUG, "m_tmode(): nonexistent channel %s", parv[1]);
		return;
	}

	if (atol(parv[0]) > c->ts)
		return;

	channel_mode(NULL, c, parc - 2, &parv[2]);
}

static void m_kick(char *origin, uint8_t parc, char *parv[])
{
	user_t *u = user_find(parv[1]);
	channel_t *c = channel_find(parv[0]);

	/* -> :rakaur KICK #shrike rintaun :test */
	slog(LG_DEBUG, "m_kick(): user was kicked: %s -> %s", parv[1], parv[0]);

	if (!u)
	{
		slog(LG_DEBUG, "m_kick(): got kick for nonexistant user %s", parv[1]);
		return;
	}

	if (!c)
	{
		slog(LG_DEBUG, "m_kick(): got kick in nonexistant channel: %s", parv[0]);
		return;
	}

	if (!chanuser_find(c, u))
	{
		slog(LG_DEBUG, "m_kick(): got kick for %s not in %s", u->nick, c->name);
		return;
	}

	chanuser_delete(c, u);

	/* if they kicked us, let's rejoin */
	if (is_internal_client(u))
	{
		slog(LG_DEBUG, "m_kick(): %s got kicked from %s; rejoining", u->nick, parv[0]);
		join(parv[0], u->nick);
	}
}

static void m_kill(char *origin, uint8_t parc, char *parv[])
{
	if (parc < 1)
		return;
	handle_kill(origin, parv[0], parc > 1 ? parv[1] : "<No reason given>");
}

static void m_squit(char *origin, uint8_t parc, char *parv[])
{
	slog(LG_DEBUG, "m_squit(): server leaving: %s from %s", parv[0], parv[1]);
	server_delete(parv[0]);
}

static void m_server(char *origin, uint8_t parc, char *parv[])
{
	slog(LG_DEBUG, "m_server(): new server: %s", parv[0]);
	server_add(parv[0], atoi(parv[1]), origin ? origin : me.name, origin != NULL || !ircd->uses_uid ? NULL : ts6sid, parv[2]);

	if (cnt.server == 2)
		me.actual = sstrdup(parv[0]);
	else
	{
		/* elicit PONG for EOB detection; pinging uplink is
		 * already done elsewhere -- jilles
		 */
		sts(":%s PING %s %s", ME, me.name, parv[0]);
	}
}

static void m_sid(char *origin, uint8_t parc, char *parv[])
{
	/* -> :1JJ SID file. 2 00F :telnet server */
	slog(LG_DEBUG, "m_sid(): new server: %s", parv[0]);
	server_add(parv[0], atoi(parv[1]), origin ? origin : me.name, parv[2], parv[3]);

	if (cnt.server == 2)
		me.actual = sstrdup(parv[0]);
	else
	{
		/* elicit PONG for EOB detection; pinging uplink is
		 * already done elsewhere -- jilles
		 */
		sts(":%s PING %s %s", ME, me.name, parv[2]);
	}
}

static void m_stats(char *origin, uint8_t parc, char *parv[])
{
	handle_stats(user_find(origin), parv[0][0]);
}

static void m_admin(char *origin, uint8_t parc, char *parv[])
{
	handle_admin(user_find(origin));
}

static void m_version(char *origin, uint8_t parc, char *parv[])
{
	handle_version(user_find(origin));
}

static void m_info(char *origin, uint8_t parc, char *parv[])
{
	handle_info(user_find(origin));
}

static void m_whois(char *origin, uint8_t parc, char *parv[])
{
	handle_whois(user_find(origin), parc >= 2 ? parv[1] : "*");
}

static void m_trace(char *origin, uint8_t parc, char *parv[])
{
	handle_trace(user_find(origin), parc >= 1 ? parv[0] : "*", parc >= 2 ? parv[1] : NULL);
}

static void m_pass(char *origin, uint8_t parc, char *parv[])
{
	/* TS5: PASS mypassword :TS
	 * TS6: PASS mypassword TS 6 :sid */
	if (strcmp(curr_uplink->pass, parv[0]))
	{
		slog(LG_INFO, "m_pass(): password mismatch from uplink; aborting");
		runflags |= RF_SHUTDOWN;
	}

	if (ircd->uses_uid && parc > 3 && atoi(parv[2]) >= 6)
		strlcpy(ts6sid, parv[3], sizeof(ts6sid));
	else
	{
		if (ircd->uses_uid)
		{
			slog(LG_INFO, "m_pass(): uplink does not support TS6, falling back to TS5");
			ircd->uses_uid = FALSE;
		}
		ts6sid[0] = '\0';
	}
}

static void m_error(char *origin, uint8_t parc, char *parv[])
{
	slog(LG_INFO, "m_error(): error from server: %s", parv[0]);
}

static void m_encap(char *origin, uint8_t parc, char *parv[])
{
	user_t *u;

	if (parc < 2)
		return;
	if (match(parv[0], me.name))
		return;
	if (!irccasecmp(parv[1], "LOGIN"))
	{
		/* :jilles ENCAP * LOGIN jilles */
		if (!use_rserv_support)
			return;
		if (parc < 3)
			return;
		u = user_find(origin);
		if (u == NULL)
			return;
		if (u->server->flags & SF_EOB)
		{
			slog(LG_DEBUG, "m_encap(): %s sent ENCAP LOGIN for user %s outside burst", u->server->name, origin);
			return;
		}
		handle_burstlogin(u, parv[2]);
	}
}

static void m_capab(char *origin, uint8_t parc, char *parv[])
{
	char *p;

	use_rserv_support = FALSE;
	use_tb = FALSE;
	use_rsfnc = FALSE;
	for (p = strtok(parv[0], " "); p != NULL; p = strtok(NULL, " "))
	{
		if (!irccasecmp(p, "SERVICES"))
		{
			slog(LG_DEBUG, "m_capab(): uplink has rserv extensions, enabling support.");
			use_rserv_support = TRUE;
		}
		if (!irccasecmp(p, "TB"))
		{
			slog(LG_DEBUG, "m_capab(): uplink does topic bursting, using if appropriate.");
			use_tb = TRUE;
		}
		if (!irccasecmp(p, "RSFNC"))
		{
			slog(LG_DEBUG, "m_capab(): uplink does RSFNC, assuming whole net has it.");
			use_rsfnc = TRUE;
		}
	}

	/* Now we know whether or not we should enable services support,
	 * so burst the clients.
	 *       --nenolod
	 */
	services_init();
}

static void m_motd(char *origin, uint8_t parc, char *parv[])
{
	handle_motd(user_find(origin));
}

/* Server ended their burst: warn all their users if necessary -- jilles */
static void server_eob(server_t *s)
{
	node_t *n;

	LIST_FOREACH(n, s->userlist.head)
	{
		handle_nickchange((user_t *)n->data);
	}
}

void _modinit(module_t * m)
{
	/* Symbol relocation voodoo. */
	server_login = &ratbox_server_login;
	introduce_nick = &ratbox_introduce_nick;
	quit_sts = &ratbox_quit_sts;
	wallops = &ratbox_wallops;
	join_sts = &ratbox_join_sts;
	chan_lowerts = &ratbox_chan_lowerts;
	kick = &ratbox_kick;
	msg = &ratbox_msg;
	notice_sts = &ratbox_notice;
	wallchops = &ratbox_wallchops;
	numeric_sts = &ratbox_numeric_sts;
	skill = &ratbox_skill;
	part = &ratbox_part;
	kline_sts = &ratbox_kline_sts;
	unkline_sts = &ratbox_unkline_sts;
	topic_sts = &ratbox_topic_sts;
	mode_sts = &ratbox_mode_sts;
	ping_sts = &ratbox_ping_sts;
	ircd_on_login = &ratbox_on_login;
	ircd_on_logout = &ratbox_on_logout;
	jupe = &ratbox_jupe;
	fnc_sts = &ratbox_fnc_sts;
	invite_sts = &ratbox_invite_sts;

	mode_list = ratbox_mode_list;
	ignore_mode_list = ratbox_ignore_mode_list;
	status_mode_list = ratbox_status_mode_list;
	prefix_mode_list = ratbox_prefix_mode_list;

	ircd = &Ratbox;

	pcommand_add("PING", m_ping);
	pcommand_add("PONG", m_pong);
	pcommand_add("PRIVMSG", m_privmsg);
	pcommand_add("NOTICE", m_notice);
	pcommand_add("SJOIN", m_sjoin);
	pcommand_add("PART", m_part);
	pcommand_add("NICK", m_nick);
	pcommand_add("QUIT", m_quit);
	pcommand_add("MODE", m_mode);
	pcommand_add("KICK", m_kick);
	pcommand_add("KILL", m_kill);
	pcommand_add("SQUIT", m_squit);
	pcommand_add("SERVER", m_server);
	pcommand_add("STATS", m_stats);
	pcommand_add("ADMIN", m_admin);
	pcommand_add("VERSION", m_version);
	pcommand_add("INFO", m_info);
	pcommand_add("WHOIS", m_whois);
	pcommand_add("TRACE", m_trace);
	pcommand_add("JOIN", m_join);
	pcommand_add("PASS", m_pass);
	pcommand_add("ERROR", m_error);
	pcommand_add("TOPIC", m_topic);
	pcommand_add("TB", m_tb);
	pcommand_add("ENCAP", m_encap);
	pcommand_add("CAPAB", m_capab);
	pcommand_add("UID", m_uid);
	pcommand_add("BMASK", m_bmask);
	pcommand_add("TMODE", m_tmode);
	pcommand_add("SID", m_sid);
	pcommand_add("MOTD", m_motd);

	hook_add_event("server_eob");
	hook_add_hook("server_eob", (void (*)(void *))server_eob);

	m->mflags = MODTYPE_CORE;

	pmodule_loaded = TRUE;
}
