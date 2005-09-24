/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains protocol support for ratbox-based ircd.
 *
 * $Id: scylla.c 2337 2005-09-24 02:01:26Z jilles $
 */

#include "atheme.h"
#include "protocol/ratbox.h"

DECLARE_MODULE_V1
(
	"protocol/scylla", FALSE, _modinit, NULL,
	"$Id: scylla.c 2337 2005-09-24 02:01:26Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

/* *INDENT-OFF* */

ircd_t Scylla = {
	"Scylla (svn 14 or later)",	/* IRCd name */
	"$$",				/* TLD Prefix, used by Global. */
	FALSE,				/* Whether or not we use IRCNet/TS6 UID */
	TRUE,				/* Whether or not we use RCOMMAND */
	FALSE,				/* Whether or not we support channel owners. */
	FALSE,				/* Whether or not we support channel protection. */
	FALSE,				/* Whether or not we support halfops. */
	FALSE,				/* Whether or not we use P10 */
	FALSE,				/* Whether or not we use vHosts. */
	0,				/* Oper-only cmodes */
	0,				/* Integer flag for owner channel flag. */
	0,				/* Integer flag for protect channel flag. */
	0,				/* Integer flag for halfops. */
	"+",				/* Mode we set for owner. */
	"+",				/* Mode we set for protect. */
	"+"				/* Mode we set for halfops. */
};

struct cmode_ scylla_mode_list[] = {
  { 'i', CMODE_INVITE },
  { 'm', CMODE_MOD    },
  { 'n', CMODE_NOEXT  },
  { 'p', CMODE_PRIV   },
  { 's', CMODE_SEC    },
  { 't', CMODE_TOPIC  },
  { 'k', CMODE_KEY    },
  { 'l', CMODE_LIMIT  },
  { '\0', 0 }
};

struct cmode_ scylla_ignore_mode_list[] = {
  { 'e', CMODE_EXEMPT },
  { 'I', CMODE_INVEX  },
  { '\0', 0 }
};

struct cmode_ scylla_status_mode_list[] = {
  { 'o', CMODE_OP    },
  { 'v', CMODE_VOICE },
  { '\0', 0 }
};

struct cmode_ scylla_prefix_mode_list[] = {
  { '@', CMODE_OP    },
  { '+', CMODE_VOICE },
  { '\0', 0 }
};

/* *INDENT-ON* */

/* login to our uplink */
static uint8_t scylla_server_login(void)
{
        int8_t ret;

        ret = sts("PASS %s :TS", curr_uplink->pass);
        if (ret == 1)
                return 1;

        me.bursting = TRUE;

        sts("CAPAB :QS KLN UNKLN");
        sts("SERVER %s 1 :%s", me.name, me.desc);
        sts("SVINFO 5 3 0 :%ld", CURRTIME);

	services_init();

	pcommand_add(nicksvs.nick, nickserv);
	pcommand_add(chansvs.nick, cservice);
	pcommand_add(opersvs.nick, oservice);

        return 0;
}

/* introduce a client */
static user_t *scylla_introduce_nick(char *nick, char *user, char *host, char *real, char *modes)
{
	sts("RCOMMAND %s %s", nick, me.name);
	return NULL;
}

/* WALLOPS wrapper */
static void scylla_wallops(char *fmt, ...)
{
        va_list ap;
        char buf[BUFSIZE];

        if (config_options.silent)
                return;

        va_start(ap, fmt);
        vsnprintf(buf, BUFSIZE, fmt, ap);
	va_end(ap);

        sts(":%s WALLOPS :%s", me.name, buf);
}

/* kicks a user from a channel */
static void scylla_kick(char *from, char *channel, char *to, char *reason)
{
        channel_t *chan = channel_find(channel);
        user_t *user = user_find(to);

        if (!chan || !user)
                return;

        sts(":%s KICK %s %s :%s", me.name, channel, to, reason);

        chanuser_delete(chan, user);
}

/* PRIVMSG wrapper */
static void scylla_msg(char *from, char *target, char *fmt, ...)
{
        va_list ap;
        char buf[BUFSIZE];

        va_start(ap, fmt);
        vsnprintf(buf, BUFSIZE, fmt, ap);
        va_end(ap);

        sts(":%s PRIVMSG %s :%s", from, target, buf);
}

/* NOTICE wrapper */
static void scylla_notice(char *from, char *target, char *fmt, ...)
{
        va_list ap;
        char buf[BUFSIZE];

        va_start(ap, fmt);
        vsnprintf(buf, BUFSIZE, fmt, ap);
        va_end(ap);

        sts(":%s 304 %s :%s", me.name, target, buf);
}

/* numeric wrapper */
static void scylla_numeric_sts(char *from, int numeric, char *target, char *fmt, ...)
{
	va_list ap;
	char buf[BUFSIZE];

	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZE, fmt, ap);
	va_end(ap);

	sts(":%s %d %s %s", from, numeric, target, buf);
}

/* KILL wrapper */
static void scylla_skill(char *from, char *nick, char *fmt, ...)
{
	va_list ap;
	char buf[BUFSIZE];

	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZE, fmt, ap);
	va_end(ap);

        sts(":%s KILL %s :%s!%s!%s (%s)", me.name, nick, from, from, from, buf);
}

/* server-to-server KLINE wrapper */
static void scylla_kline_sts(char *server, char *user, char *host, long duration, char *reason)
{
	if (!me.connected)
		return;

	sts(":%s KLINE %s %ld %s %s :%s", me.name, server, duration, user, host, reason);
}

/* server-to-server UNKLINE wrapper */
static void scylla_unkline_sts(char *server, char *user, char *host)
{
	if (!me.connected)
		return;

	sts(":%s UNKLINE %s %s %s", opersvs.nick, server, user, host);
}

/* topic wrapper */
static void scylla_topic_sts(char *channel, char *setter, char *topic)
{
	if (!me.connected)
		return;

	sts(":%s TOPIC %s :%s (%s)", chansvs.nick, channel, topic, setter);
}

/* mode wrapper */
static void scylla_mode_sts(char *sender, char *target, char *modes)
{
	if (!me.connected)
		return;

	sts(":%s MODE %s %s", me.name, target, modes);
}

/* ping wrapper */
static void scylla_ping_sts(void)
{
	if (!me.connected)
		return;

	sts("PING :%s", me.name);
}

/* protocol-specific stuff to do on login */
static void scylla_on_login(char *origin, char *user, char *wantedhost)
{
	/* nothing to do on ratbox */
	return;
}

/* protocol-specific stuff to do on login */
static void scylla_on_logout(char *origin, char *user, char *wantedhost)
{
	/* nothing to do on ratbox */
	return;
}

static void scylla_jupe(char *server, char *reason)
{
        if (!me.connected)
                return;

	sts(":%s SQUIT %s :%s", opersvs.nick, server, reason);
        sts(":%s SERVER %s 2 :%s", me.name, server, reason);
}

static void m_topic(char *origin, uint8_t parc, char *parv[])
{
	channel_t *c = channel_find(parv[0]);

	if (!origin)
		return;

	c->topic = sstrdup(parv[1]);
	c->topic_setter = sstrdup(origin);
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

static void m_sjoin(char *origin, uint8_t parc, char *parv[])
{
	/* -> :proteus.malkier.net SJOIN 1073516550 #shrike +tn :@sycobuny @+rakaur */

	channel_t *c;
	uint8_t modec = 0;
	char *modev[16];
	uint8_t userc;
	char *userv[256];
	uint8_t i;
	time_t ts;

	if (origin)
	{
		/* :origin SJOIN ts chan modestr [key or limits] :users */
		modev[0] = parv[2];

		if (parc > 4)
			modev[++modec] = parv[3];
		if (parc > 5)
			modev[++modec] = parv[4];

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

		channel_mode(c, modec, modev);

		userc = sjtoken(parv[parc - 1], ' ', userv);

		for (i = 0; i < userc; i++)
			chanuser_add(c, userv[i]);
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

		if ((k = kline_find(parv[4], parv[5])))
		{
			/* the new user matches a kline.
			 * the server introducing the user probably wasn't around when
			 * we added the kline or isn't accepting klines from us.
			 * either way, we'll KILL the user and send the server
			 * a new KLINE.
			 */

			skill(opersvs.nick, parv[0], k->reason);
			kline_sts(parv[6], k->user, k->host, (k->expires - CURRTIME), k->reason);

			return;
		}

		u = user_add(parv[0], parv[4], parv[5], NULL, NULL, NULL, parv[7], s);

		user_mode(u, parv[3]);

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

		/* remove the current one from the list */
		n = node_find(u, &userlist[u->hash]);
		node_del(n, &userlist[u->hash]);
		node_free(n);

		/* change the nick */
		strlcpy(u->nick, parv[0], NICKLEN);

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
		channel_mode(channel_find(parv[0]), parc - 1, &parv[1]);
	else
		user_mode(user_find(parv[0]), parv[1]);
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
	mychan_t *mc;
	node_t *n;
	int i;

	slog(LG_DEBUG, "m_kill(): killed user: %s", parv[0]);
	user_delete(parv[0]);

	if (!irccasecmp(chansvs.nick, parv[0]))
	{
		services_init();

		if (config_options.chan)
			join(config_options.chan, chansvs.nick);

		for (i = 0; i < HASHSIZE; i++)
		{
			LIST_FOREACH(n, mclist[i].head)
			{
				mc = (mychan_t *)n->data;

				if ((config_options.join_chans) && (mc->chan) && (mc->chan->nummembers >= 1))
					join(mc->name, chansvs.nick);

				if ((config_options.join_chans) && (!config_options.leave_chans) && (mc->chan) && (mc->chan->nummembers == 0))
					join(mc->name, chansvs.nick);
			}
		}
	}
}

static void m_squit(char *origin, uint8_t parc, char *parv[])
{
	slog(LG_DEBUG, "m_squit(): server leaving: %s from %s", parv[0], parv[1]);
	server_delete(parv[0]);
}

static void m_server(char *origin, uint8_t parc, char *parv[])
{
	slog(LG_DEBUG, "m_server(): new server: %s", parv[0]);
	server_add(parv[0], atoi(parv[1]), origin ? origin : me.name,
		NULL, parv[2]);

	if (cnt.server == 2)
		me.actual = sstrdup(parv[0]);
}

static void m_stats(char *origin, uint8_t parc, char *parv[])
{
	if (irccasecmp(me.name, parv[1]))
		return;

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
	handle_trace(origin, parc >= 1 ? parv[0] : "*",
			parc >= 2 ? parv[1] : NULL);
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

void _modinit(module_t *m)
{
	/* Symbol relocation voodoo. */
	server_login = &scylla_server_login;
	introduce_nick = &scylla_introduce_nick;
	wallops = &scylla_wallops;
	kick = &scylla_kick;
	msg = &scylla_msg;
	notice = &scylla_notice;
	numeric_sts = &scylla_numeric_sts;
	skill = &scylla_skill;
	kline_sts = &scylla_kline_sts;
	unkline_sts = &scylla_unkline_sts;
	topic_sts = &scylla_topic_sts;
	mode_sts = &scylla_mode_sts;
	ping_sts = &scylla_ping_sts;
	ircd_on_login = &scylla_on_login;
	ircd_on_logout = &scylla_on_logout;
	jupe = &scylla_jupe;

	mode_list = scylla_mode_list;
	ignore_mode_list = scylla_ignore_mode_list;
	status_mode_list = scylla_status_mode_list;
	prefix_mode_list = scylla_prefix_mode_list;

	ircd = &Scylla;

	pcommand_add("PING", m_ping);
	pcommand_add("PONG", m_pong);
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

	m->mflags = MODTYPE_CORE;

	pmodule_loaded = TRUE;
}

