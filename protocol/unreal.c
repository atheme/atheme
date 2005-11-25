/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains protocol support for bahamut-based ircd.
 *
 * $Id: unreal.c 3975 2005-11-25 20:12:20Z nenolod $
 */

#include "atheme.h"
#include "protocol/unreal.h"

DECLARE_MODULE_V1("protocol/unreal", TRUE, _modinit, NULL, "$Id: unreal.c 3975 2005-11-25 20:12:20Z nenolod $", "Atheme Development Group <http://www.atheme.org>");

/* *INDENT-OFF* */

ircd_t Unreal = {
        "UnrealIRCd 3.1 or later",      /* IRCd name */
        "$",                            /* TLD Prefix, used by Global. */
        FALSE,                          /* Whether or not we use IRCNet/TS6 UID */
        FALSE,                          /* Whether or not we use RCOMMAND */
        TRUE,                           /* Whether or not we support channel owners. */
        TRUE,                           /* Whether or not we support channel protection. */
        TRUE,                           /* Whether or not we support halfops. */
	FALSE,				/* Whether or not we use P10 */
	TRUE,				/* Whether or not we use vHosts. */
	CMODE_OPERONLY | CMODE_ADMONLY, /* Oper-only cmodes */
        CMODE_OWNER,                    /* Integer flag for owner channel flag. */
        CMODE_PROTECT,                  /* Integer flag for protect channel flag. */
        CMODE_HALFOP,                   /* Integer flag for halfops. */
        "+q",                           /* Mode we set for owner. */
        "+a",                           /* Mode we set for protect. */
        "+h",                           /* Mode we set for halfops. */
	PROTOCOL_UNREAL			/* Protocol type */
};

struct cmode_ unreal_mode_list[] = {
  { 'i', CMODE_INVITE   },
  { 'm', CMODE_MOD      },
  { 'n', CMODE_NOEXT    },
  { 'p', CMODE_PRIV     },
  { 's', CMODE_SEC      },
  { 't', CMODE_TOPIC    },
  { 'c', CMODE_NOCOLOR  },
  { 'M', CMODE_MODREG   },
  { 'R', CMODE_REGONLY  },
  { 'O', CMODE_OPERONLY },
  { 'A', CMODE_ADMONLY  },
  { 'Q', CMODE_PEACE    },
  { 'S', CMODE_STRIP    },
  { 'K', CMODE_NOKNOCK  },
  { 'V', CMODE_NOINVITE },
  { 'C', CMODE_NOCTCP   },
  { 'u', CMODE_HIDING   },
  { 'z', CMODE_SSLONLY  },
  { 'N', CMODE_STICKY   },
  { '\0', 0 }
};

struct cmode_ unreal_ignore_mode_list[] = {
  { 'e', CMODE_EXEMPT },
  { 'I', CMODE_INVEX  },
  { 'j', CMODE_JTHROT },
  { '\0', 0 }
};

struct cmode_ unreal_status_mode_list[] = {
  { 'q', CMODE_OWNER   },
  { 'a', CMODE_PROTECT },
  { 'o', CMODE_OP      },
  { 'h', CMODE_HALFOP  },
  { 'v', CMODE_VOICE   },
  { '\0', 0 }
};

struct cmode_ unreal_prefix_mode_list[] = {
  { '~', CMODE_OWNER   },
  { '*', CMODE_PROTECT },
  { '&', CMODE_PROTECT },
  { '@', CMODE_OP      },
  { '%', CMODE_HALFOP  },
  { '+', CMODE_VOICE   },
  { '\0', 0 }
};

/* *INDENT-ON* */

/* login to our uplink */
static uint8_t unreal_server_login(void)
{
	int8_t ret;

	ret = sts("PASS %s", curr_uplink->pass);
	if (ret == 1)
		return 1;

	me.bursting = TRUE;

	sts("PROTOCTL TOKEN NICKv2 VHP UMODE2 SJOIN SJOIN2 SJ3 NOQUIT TKLEXT");
	sts("SERVER %s 1 :%s", me.name, me.desc);

	services_init();

	return 0;
}

/* introduce a client */
static void unreal_introduce_nick(char *nick, char *user, char *host, char *real, char *uid)
{
	sts("NICK %s 1 %ld %s %s %s 0 +%s * :%s", nick, CURRTIME, user, host, me.name, "io", real);
}

static void unreal_quit_sts(user_t *u, char *reason)
{
	if (!me.connected)
		return;

	sts(":%s QUIT :%s", u->nick, reason);
}

/* WALLOPS wrapper */
static void unreal_wallops(char *fmt, ...)
{
	va_list ap;
	char buf[BUFSIZE];

	if (config_options.silent)
		return;

	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZE, fmt, ap);
	va_end(ap);

	sts(":%s GLOBOPS :%s", me.name, buf);
}

/* join a channel */
static void unreal_join_sts(channel_t *c, user_t *u, boolean_t isnew, char *modes)
{
	if (isnew)
		sts(":%s SJOIN %ld %s %s :@%s", me.name, c->ts, c->name,
				modes, u->nick);
	else
		sts(":%s SJOIN %ld %s + :@%s", me.name, c->ts, c->name,
				u->nick);
}

/* kicks a user from a channel */
static void unreal_kick(char *from, char *channel, char *to, char *reason)
{
	channel_t *chan = channel_find(channel);
	user_t *user = user_find(to);

	if (!chan || !user)
		return;

	sts(":%s KICK %s %s :%s", from, channel, to, reason);

	chanuser_delete(chan, user);
}

/* PRIVMSG wrapper */
static void unreal_msg(char *from, char *target, char *fmt, ...)
{
	va_list ap;
	char buf[BUFSIZE];

	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZE, fmt, ap);
	va_end(ap);

	sts(":%s PRIVMSG %s :%s", from, target, buf);
}

/* NOTICE wrapper */
static void unreal_notice(char *from, char *target, char *fmt, ...)
{
	va_list ap;
	char buf[BUFSIZE];

	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZE, fmt, ap);
	va_end(ap);

	sts(":%s NOTICE %s :%s", from, target, buf);
}

static void unreal_numeric_sts(char *from, int numeric, char *target, char *fmt, ...)
{
	va_list ap;
	char buf[BUFSIZE];

	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZE, fmt, ap);
	va_end(ap);

	sts(":%s %d %s %s", from, numeric, target, buf);
}

/* KILL wrapper */
static void unreal_skill(char *from, char *nick, char *fmt, ...)
{
	va_list ap;
	char buf[BUFSIZE];

	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZE, fmt, ap);
	va_end(ap);

	sts(":%s KILL %s :%s!%s!%s (%s)", from, nick, from, from, from, buf);
}

/* PART wrapper */
static void unreal_part(char *chan, char *nick)
{
	user_t *u = user_find(nick);
	channel_t *c = channel_find(chan);
	chanuser_t *cu;

	if (!u || !c)
		return;

	if (!(cu = chanuser_find(c, u)))
		return;

	sts(":%s PART %s", u->nick, c->name);

	chanuser_delete(c, u);
}

/* server-to-server KLINE wrapper */
static void unreal_kline_sts(char *server, char *user, char *host, long duration, char *reason)
{
	if (!me.connected)
		return;

	sts(":%s TKL + G %s %s %s 0 %ld :%s", me.name, user, host, opersvs.nick, time(NULL), reason);
}

/* server-to-server UNKLINE wrapper */
static void unreal_unkline_sts(char *server, char *user, char *host)
{
	if (!me.connected)
		return;

	sts(":%s TKL - G %s %s %s", me.name, user, host, opersvs.nick);
}

/* topic wrapper */
static void unreal_topic_sts(char *channel, char *setter, time_t ts, char *topic)
{
	if (!me.connected)
		return;

	sts(":%s TOPIC %s %s %ld :%s", chansvs.nick, channel, setter, ts, topic);
}

/* mode wrapper */
static void unreal_mode_sts(char *sender, char *target, char *modes)
{
	if (!me.connected)
		return;

	sts(":%s MODE %s %s", sender, target, modes);
}

/* ping wrapper */
static void unreal_ping_sts(void)
{
	if (!me.connected)
		return;

	sts("PING :%s", me.name);
}

/* protocol-specific stuff to do on login */
static void unreal_on_login(char *origin, char *user, char *wantedhost)
{
	if (!me.connected)
		return;

	/* Can only record identified state if logged in to correct nick,
	 * sorry -- jilles
	 */
	if (irccasecmp(origin, user))
		return;

	/* imo, we should be using SVS2MODE to show the modechange here and on logout --w00t */
	sts(":%s SVS2MODE %s +rd %ld", nicksvs.nick, origin, time(NULL));
}

/* protocol-specific stuff to do on logout */
static void unreal_on_logout(char *origin, char *user, char *wantedhost)
{
	if (!me.connected)
		return;

	if (irccasecmp(origin, user))
		return;

	sts(":%s SVS2MODE %s -r+d %ld", nicksvs.nick, origin, time(NULL));
}

static void unreal_jupe(char *server, char *reason)
{
	if (!me.connected)
		return;

	sts(":%s SQUIT %s :%s", opersvs.nick, server, reason);
	sts(":%s SERVER %s 2 :%s", me.name, server, reason);
}

static void unreal_sethost_sts(char *source, char *target, char *host)
{
	if (!me.connected)
		return;

	sts(":%s SVS2MODE %s +x", source, target, host);
	sts(":%s CHGHOST %s :%s", source, target, host);
}

static void m_topic(char *origin, uint8_t parc, char *parv[])
{
	channel_t *c = channel_find(parv[0]);

	if (!c)
		return;

	handle_topic(c, parv[1], atol(parv[2]), parv[3]);
}

static void m_ping(char *origin, uint8_t parc, char *parv[])
{
	/* reply to PING's */
	sts(":%s PONG %s %s", me.name, me.name, parv[0]);
}

static void m_pong(char *origin, uint8_t parc, char *parv[])
{
	/* someone replied to our PING */
	if ((!parv[0]) || (strcasecmp(me.actual, parv[0])))
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
	/*
	 *  -> :proteus.malkier.net SJOIN 1073516550 #shrike +tn :@sycobuny @+rakaur
	 *      also:
	 *  -> :nenolod_ SJOIN 1117334567 #chat
	 *      also:
	 *  -> SJOIN 1117334567 #chat :@nenolod
	 */

	channel_t *c;
	uint8_t modec = 0;
	char *modev[16];
	uint8_t userc;
	char *userv[256];
	uint8_t i;
	time_t ts;

	if (parc >= 4)
	{
		/* :origin SJOIN ts chan modestr [key or limits] :users */
		modev[modec++] = parv[2];

		if (parc > 4)
			modev[modec++] = parv[3];
		if (parc > 5)
			modev[modec++] = parv[4];

		c = channel_find(parv[1]);
		ts = atol(parv[0]);

		if (!c)
		{
			slog(LG_DEBUG, "m_sjoin(): new channel: %s", parv[1]);
			c = channel_add(parv[1], ts);
		}

		if (ts < c->ts)
		{
			chanuser_t *cu;
			node_t *n;

			/* the TS changed.  a TS change requires the following things
			 * to be done to the channel:  reset all modes to nothing, remove
			 * all status modes on known users on the channel (including ours),
			 * and set the new TS.
			 */

			c->modes = 0;
			c->limit = 0;
			if (c->key)
				free(c->key);
			c->key = NULL;

			LIST_FOREACH(n, c->members.head)
			{
				cu = (chanuser_t *)n->data;
				cu->modes = 0;
			}

			slog(LG_INFO, "m_sjoin(): TS changed for %s (%ld -> %ld)", c->name, c->ts, ts);

			c->ts = ts;
		}

		channel_mode(NULL, c, modec, modev);

		userc = sjtoken(parv[parc - 1], ' ', userv);

		for (i = 0; i < userc; i++)
			if ((*userv[i] == '\'') || (*userv[i] == '"'))	/* ignore cmodes +I, +e */
				;
			else if (*userv[i] == '&')	/* channel ban */
				chanban_add(c, userv[i] + 1);
			else
				chanuser_add(c, userv[i]);
	}
	else if (parc == 3)
	{
		/* :origin SJOIN ts chan :users */
		c = channel_find(parv[1]);
		ts = atol(parv[0]);

		if (!c)
		{
			slog(LG_DEBUG, "m_sjoin(): new channel: %s", parv[1]);
			c = channel_add(parv[1], ts);
		}

		if (ts < c->ts)
		{
			chanuser_t *cu;
			node_t *n;

			/* the TS changed.  a TS change requires the following things
			 * to be done to the channel:  reset all modes to nothing, remove
			 * all status modes on known users on the channel (including ours),
			 * and set the new TS.
			 */

			c->modes = 0;
			c->limit = 0;
			if (c->key)
				free(c->key);
			c->key = NULL;

			LIST_FOREACH(n, c->members.head)
			{
				cu = (chanuser_t *)n->data;
				cu->modes = 0;
			}

			slog(LG_INFO, "m_sjoin(): TS changed for %s (%ld -> %ld)", c->name, c->ts, ts);

			c->ts = ts;
		}

		userc = sjtoken(parv[parc - 1], ' ', userv);

		for (i = 0; i < userc; i++)
			if ((*userv[i] == '\'') || (*userv[i] == '"'))	/* ignore cmodes +I, +e */
				;
			else if (*userv[i] == '&')	/* channel ban */
				chanban_add(c, userv[i] + 1);
			else
				chanuser_add(c, userv[i]);
	}
	else if (parc == 2)
	{
		c = channel_find(parv[1]);
		/* XXX what if the channel doesn't exist? */
		ts = atol(parv[0]);

		if (ts < c->ts)
		{
			chanuser_t *cu;
			node_t *n;

			/* the TS changed.  a TS change requires the following things
			 * to be done to the channel:  reset all modes to nothing, remove
			 * all status modes on known users on the channel (including ours),
			 * and set the new TS.
			 */

			c->modes = 0;
			c->limit = 0;
			if (c->key)
				free(c->key);
			c->key = NULL;

			LIST_FOREACH(n, c->members.head)
			{
				cu = (chanuser_t *)n->data;
				cu->modes = 0;
			}

			slog(LG_INFO, "m_sjoin(): TS changed for %s (%ld -> %ld)", c->name, c->ts, ts);

			c->ts = ts;
			/* XXX lost modes! */
		}

		chanuser_add(c, origin);
	}
}

static void m_part(char *origin, uint8_t parc, char *parv[])
{
	slog(LG_DEBUG, "m_part(): user left channel: %s -> %s", origin, parv[0]);

	chanuser_delete(channel_find(parv[0]), user_find(origin));
}

static void m_nick(char *origin, uint8_t parc, char *parv[])
{
	server_t *s;
	user_t *u;
	kline_t *k;

	if (parc == 10)
	{
		s = server_find(parv[5]);
		if (!s)
		{
			slog(LG_DEBUG, "m_nick(): new user on nonexistant server: %s", parv[5]);
			return;
		}

		slog(LG_DEBUG, "m_nick(): new user on `%s': %s", s->name, parv[0]);

		if ((k = kline_find(parv[3], parv[4])))
		{
			/* the new user matches a kline.
			 * the server introducing the user probably wasn't around when
			 * we added the kline or isn't accepting klines from us.
			 * either way, we'll KILL the user and send the server
			 * a new KLINE.
			 */

			skill(opersvs.nick, parv[0], k->reason);
			kline_sts(parv[5], k->user, k->host, (k->expires - CURRTIME), k->reason);

			return;
		}

		u = user_add(parv[0], parv[3], parv[4], parv[8], NULL, NULL, parv[9], s, atoi(parv[2]));

		user_mode(u, parv[7]);

		/* Ok, we have the user ready to go.
		 * Here's the deal -- if the user's SVID is before
		 * the start time, and not 0, then check to see
		 * if it's a registered account or not.
		 *
		 * If it IS registered, deal with that accordingly,
		 * via handle_burstlogin(). --nenolod
		 */
		/* Changed to just check umode +r for now -- jilles */
		if (strchr(parv[7], 'r'))
			handle_burstlogin(u, parv[0]);

		handle_nickchange(u);
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

		/* fix up +r if necessary -- jilles */
		if (u->myuser != NULL && irccasecmp(u->nick, parv[0]) && !irccasecmp(parv[0], u->myuser->name))
			/* changed nick to registered one, reset +r */
			sts(":%s SVS2MODE %s +rd %ld", nicksvs.nick, parv[0], time(NULL));

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

static void m_quit(char *origin, uint8_t parc, char *parv[])
{
	slog(LG_DEBUG, "m_quit(): user leaving: %s", origin);

	/* user_delete() takes care of removing channels and so forth */
	user_delete(origin);
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
		channel_mode(NULL, channel_find(parv[0]), parc - 2, &parv[1]);
	else
		user_mode(user_find(parv[0]), parv[1]);
}

static void m_umode(char *origin, uint8_t parc, char *parv[])
{
	if (!origin)
	{
		slog(LG_DEBUG, "m_umode(): received MODE without origin");
		return;
	}

	user_mode(user_find(origin), parv[0]);
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
	if (!irccasecmp(chansvs.nick, parv[1]))
	{
		slog(LG_DEBUG, "m_kick(): i got kicked from `%s'; rejoining", parv[0]);
		join(parv[0], parv[1]);
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
	server_add(parv[0], atoi(parv[1]), origin ? origin : me.name, NULL, parv[2]);

	if (cnt.server == 2)
		me.actual = sstrdup(parv[0]);
}

static void m_stats(char *origin, uint8_t parc, char *parv[])
{
	handle_stats(origin, parv[0][0]);
}

static void m_admin(char *origin, uint8_t parc, char *parv[])
{
	handle_admin(origin);
}

static void m_version(char *origin, uint8_t parc, char *parv[])
{
	handle_version(origin);
}

static void m_info(char *origin, uint8_t parc, char *parv[])
{
	handle_info(origin);
}

static void m_whois(char *origin, uint8_t parc, char *parv[])
{
	handle_whois(origin, parc >= 2 ? parv[1] : "*");
}

static void m_trace(char *origin, uint8_t parc, char *parv[])
{
	handle_trace(origin, parc >= 1 ? parv[0] : "*", parc >= 2 ? parv[1] : NULL);
}

static void m_join(char *origin, uint8_t parc, char *parv[])
{
	user_t *u = user_find(origin);
	chanuser_t *cu;
	node_t *n, *tn;

	if (!u)
		return;

	/* JOIN 0 is really a part from all channels */
	if (parv[0][0] == '0')
	{
		LIST_FOREACH_SAFE(n, tn, u->channels.head)
		{
			cu = (chanuser_t *)n->data;
			chanuser_delete(cu->chan, u);
		}
	}
}

static void m_pass(char *origin, uint8_t parc, char *parv[])
{
	if (strcmp(curr_uplink->pass, parv[0]))
	{
		slog(LG_INFO, "m_pass(): password mismatch from uplink; aborting");
		runflags |= RF_SHUTDOWN;
	}
}

static void m_error(char *origin, uint8_t parc, char *parv[])
{
	slog(LG_INFO, "m_error(): error from server: %s", parv[0]);
}

static void m_chghost(char *origin, uint8_t parc, char *parv[])
{
	user_t *u = user_find(parv[0]);

	if (!u)
		return;

	strlcpy(u->vhost, parv[1], HOSTLEN);
}

void _modinit(module_t * m)
{
	/* Symbol relocation voodoo. */
	server_login = &unreal_server_login;
	introduce_nick = &unreal_introduce_nick;
	quit_sts = &unreal_quit_sts;
	wallops = &unreal_wallops;
	join_sts = &unreal_join_sts;
	kick = &unreal_kick;
	msg = &unreal_msg;
	notice = &unreal_notice;
	numeric_sts = &unreal_numeric_sts;
	skill = &unreal_skill;
	part = &unreal_part;
	kline_sts = &unreal_kline_sts;
	unkline_sts = &unreal_unkline_sts;
	topic_sts = &unreal_topic_sts;
	mode_sts = &unreal_mode_sts;
	ping_sts = &unreal_ping_sts;
	ircd_on_login = &unreal_on_login;
	ircd_on_logout = &unreal_on_logout;
	jupe = &unreal_jupe;
	sethost_sts = &unreal_sethost_sts;

	mode_list = unreal_mode_list;
	ignore_mode_list = unreal_ignore_mode_list;
	status_mode_list = unreal_status_mode_list;
	prefix_mode_list = unreal_prefix_mode_list;

	ircd = &Unreal;

	pcommand_add("PING", m_ping);
	pcommand_add("PONG", m_pong);
	pcommand_add("PRIVMSG", m_privmsg);
	pcommand_add("NOTICE", m_notice);
	pcommand_add("SJOIN", m_sjoin);
	pcommand_add("PART", m_part);
	pcommand_add("NICK", m_nick);
	pcommand_add("QUIT", m_quit);
	pcommand_add("UMODE2", m_umode);
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
	pcommand_add("CHGHOST", m_chghost);

	/* 
	 * for fun, and to give nenolod a heart attack
	 * whenever he reads unreal.c, let us do tokens... --w00t
	 * 
	 * oh, as a side warning: don't remove the above. not only
	 * are they for compatability, but unreal's server protocol
	 * can be odd in places (ie not sending JOIN token after synch
	 * with no SJOIN, not using KILL token... etc. --w00t.
	 */

	pcommand_add("8", m_ping);
	pcommand_add("9", m_pong);
	pcommand_add("!", m_privmsg);
	pcommand_add("B", m_notice);
	pcommand_add("~", m_sjoin);
	pcommand_add("D", m_part);
	pcommand_add("&", m_nick);
	pcommand_add(",", m_quit);
	pcommand_add("|", m_umode);
	pcommand_add("G", m_mode);
	pcommand_add("H", m_kick);
	pcommand_add(".", m_kill);
	pcommand_add("-", m_squit);
	pcommand_add("'", m_server);
	pcommand_add("2", m_stats);
	pcommand_add("@", m_admin);
	pcommand_add("+", m_version);
	pcommand_add("/", m_info);
	pcommand_add("C", m_join);
	pcommand_add("<", m_pass);
	pcommand_add("5", m_error);
	pcommand_add(")", m_topic);
	pcommand_add("AL", m_chghost);

	m->mflags = MODTYPE_CORE;

	pmodule_loaded = TRUE;
}
