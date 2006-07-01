/*
 * Copyright (c) 2006 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains protocol support for solidircd.
 *
 * $Id: solidircd.c 3841 2005-11-11 11:49:44Z jilles $
 */

#include "atheme.h"
#include "protocol/solidircd.h"

DECLARE_MODULE_V1("protocol/solidircd", TRUE, _modinit, NULL, "$Id: solidircd.c 3841 2005-11-11 11:49:44Z jilles $", "Atheme Development Group <http://www.atheme.org>");

/* *INDENT-OFF* */

ircd_t Solidircd = {
        "solid-ircd 3.4.x",             /* IRCd name */
        "$",                            /* TLD Prefix, used by Global. */
        FALSE,                          /* Whether or not we use IRCNet/TS6 UID */
        FALSE,                          /* Whether or not we use RCOMMAND */
        FALSE,                          /* Whether or not we support channel owners. */
        FALSE,                          /* Whether or not we support channel protection. */
        TRUE,                           /* Whether or not we support halfops. */
	FALSE,				/* Whether or not we use P10 */
	FALSE,				/* Whether or not we use vHosts. */
	CMODE_OPERONLY,			/* Oper-only cmodes */
        0,                              /* Integer flag for owner channel flag. */
        0,                              /* Integer flag for protect channel flag. */
	CMODE_HALFOP,                   /* Integer flag for halfops. */
        "+",                            /* Mode we set for owner. */
        "+",                            /* Mode we set for protect. */
        "+h",                           /* Mode we set for halfops. */
	PROTOCOL_SOLIDIRCD,		/* Protocol type */
	0,                              /* Permanent cmodes */
	"beI",                          /* Ban-like cmodes */
	'e',                            /* Except mchar */
	'I'                             /* Invex mchar */
};

struct cmode_ solidircd_mode_list[] = {
  { 'i', CMODE_INVITE   },
  { 'm', CMODE_MOD      },
  { 'n', CMODE_NOEXT    },
  { 'p', CMODE_PRIV     },
  { 's', CMODE_SEC      },
  { 't', CMODE_TOPIC    },
  { 'c', CMODE_NOCTRL  },
  { 'M', CMODE_MODREG   },
  { 'R', CMODE_REGONLY  },
  { 'O', CMODE_OPERONLY },
  { 'S', CMODE_SSL }, /* SSL users only */
  { 'N', CMODE_NONICK }, /* No Nick Changes */
  { 'D', CMODE_RSL },  /*  Only Resolved Clients */
  { '\0', 0 }
};

struct extmode solidircd_ignore_mode_list[] = {
  { '\0', 0 }
};

struct cmode_ solidircd_status_mode_list[] = {
  { 'o', CMODE_OP    },
  { 'h', CMODE_HALFOP },
  { 'v', CMODE_VOICE },
  { '\0', 0 }
};

struct cmode_ solidircd_prefix_mode_list[] = {
  { '@', CMODE_OP    },
  { '%', CMODE_HALFOP  },
  { '+', CMODE_VOICE },
  { '\0', 0 }
};

/* *INDENT-ON* */

/* login to our uplink */
static uint8_t solidircd_server_login(void)
{
	int8_t ret;

	ret = sts("PASS %s :TS", curr_uplink->pass);
	if (ret == 1)
		return 1;

	me.bursting = TRUE;

	sts("CAPAB SSJOIN NOQUIT BURST ZIP NICKIP TSMODE");
	sts("SERVER %s 1 :%s", me.name, me.desc);
	sts("SVINFO 5 3 0 :%ld", CURRTIME);

	services_init();

	return 0;
}

/* introduce a client */
static void solidircd_introduce_nick(char *nick, char *user, char *host, char *real, char *uid)
{
	sts("NICK %s 1 %ld +%s %s %s %s 0 0 :%s", nick, CURRTIME, "io", user, host, me.name, real);
}

/* invite a user to a channel */
static void solidircd_invite_sts(user_t *sender, user_t *target, channel_t *channel)
{
	sts(":%s INVITE %s %s", sender->nick, target->nick, channel->name);
}

static void solidircd_quit_sts(user_t *u, char *reason)
{
	if (!me.connected)
		return;

	sts(":%s QUIT :%s", u->nick, reason);
}

/* WALLOPS wrapper */
static void solidircd_wallops(char *fmt, ...)
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
static void solidircd_join_sts(channel_t *c, user_t *u, boolean_t isnew, char *modes)
{
	if (isnew)
		sts(":%s SJOIN %ld %s %s :@%s", me.name, c->ts, c->name,
				modes, u->nick);
	else
		sts(":%s SJOIN %ld %s + :@%s", me.name, c->ts, c->name,
				u->nick);
}

static void solidircd_chan_lowerts(channel_t *c, user_t *u)
{
	slog(LG_DEBUG, "solidircd_chan_lowerts(): lowering TS for %s to %ld",
			c->name, (long)c->ts);
	sts(":%s SJOIN %ld %s %s :@%s", me.name, c->ts, c->name,
				channel_modes(c, TRUE), u->nick);
	chanban_clear(c);
	/*handle_topic(c, "", 0, "");*/
	/* Don't destroy keeptopic info, I'll admit this is ugly -- jilles */
	if (c->topic != NULL)
		free(c->topic);
	if (c->topic_setter != NULL)
		free(c->topic_setter);
	c->topic = c->topic_setter = NULL;
	c->topicts = 0;
}

/* kicks a user from a channel */
static void solidircd_kick(char *from, char *channel, char *to, char *reason)
{
	channel_t *chan = channel_find(channel);
	user_t *user = user_find(to);

	if (!chan || !user)
		return;

	sts(":%s KICK %s %s :%s", from, channel, to, reason);

	chanuser_delete(chan, user);
}

/* PRIVMSG wrapper */
static void solidircd_msg(char *from, char *target, char *fmt, ...)
{
	va_list ap;
	char buf[BUFSIZE];

	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZE, fmt, ap);
	va_end(ap);

	sts(":%s PRIVMSG %s :%s", from, target, buf);
}

/* NOTICE wrapper */
static void solidircd_notice(char *from, char *target, char *fmt, ...)
{
	va_list ap;
	char buf[BUFSIZE];

	/* solidircd appears to not like it when it recieves
	 * a message to and from the same person.
	 */
	if (!strcasecmp(from, target))
		return;

	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZE, fmt, ap);
	va_end(ap);

	sts(":%s NOTICE %s :%s", from, target, buf);
}

static void solidircd_wallchops(user_t *sender, channel_t *channel, char *message)
{
	sts(":%s NOTICE @%s :%s", sender->nick, channel->name, message);
}

static void solidircd_numeric_sts(char *from, int numeric, char *target, char *fmt, ...)
{
	va_list ap;
	char buf[BUFSIZE];

	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZE, fmt, ap);
	va_end(ap);

	sts(":%s %d %s %s", from, numeric, target, buf);
}

/* KILL wrapper */
static void solidircd_skill(char *from, char *nick, char *fmt, ...)
{
	va_list ap;
	char buf[BUFSIZE];

	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZE, fmt, ap);
	va_end(ap);

	sts(":%s KILL %s :%s!%s!%s (%s)", from, nick, from, from, from, buf);
}

/* PART wrapper */
static void solidircd_part(char *chan, char *nick)
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
static void solidircd_kline_sts(char *server, char *user, char *host, long duration, char *reason)
{
	if (!me.connected)
		return;

	sts(":%s AKILL %s %s %ld %s %ld :%s", me.name, host, user, duration, opersvs.nick, time(NULL), reason);
}

/* server-to-server UNKLINE wrapper */
static void solidircd_unkline_sts(char *server, char *user, char *host)
{
	if (!me.connected)
		return;

	sts(":%s RAKILL %s %s", me.name, host, user);
}

/* topic wrapper */
static void solidircd_topic_sts(char *channel, char *setter, time_t ts, char *topic)
{
	if (!me.connected)
		return;

	sts(":%s TOPIC %s %s %ld :%s", chansvs.nick, channel, setter, ts, topic);
}

/* mode wrapper */
static void solidircd_mode_sts(char *sender, char *target, char *modes)
{
	channel_t *c = channel_find(target);

	if (!me.connected || !c)
		return;

	sts(":%s MODE %s %ld %s", sender, target, c->ts, modes);
}

/* ping wrapper */
static void solidircd_ping_sts(void)
{
	if (!me.connected)
		return;

	sts("PING :%s", me.name);
}

/* protocol-specific stuff to do on login */
static void solidircd_on_login(char *origin, char *user, char *wantedhost)
{
	if (!me.connected)
		return;

	/* Can only do this for nickserv, and can only record identified
	 * state if logged in to correct nick, sorry -- jilles
	 */
	if (nicksvs.me == NULL || irccasecmp(origin, user))
		return;

	sts(":%s SVSMODE %s +rd %ld", nicksvs.nick, origin, time(NULL));
}

/* protocol-specific stuff to do on login */
static boolean_t solidircd_on_logout(char *origin, char *user, char *wantedhost)
{
	if (!me.connected)
		return FALSE;

	if (nicksvs.me == NULL || irccasecmp(origin, user))
		return FALSE;

	sts(":%s SVSMODE %s -r+d %ld", nicksvs.nick, origin, time(NULL));
	return FALSE;
}

static void solidircd_jupe(char *server, char *reason)
{
	if (!me.connected)
		return;

	sts(":%s SQUIT %s :%s", opersvs.nick, server, reason);
	sts(":%s SERVER %s 2 :%s", me.name, server, reason);
}


static void solidircd_sethost_sts(char *source, char *target, char *host)
{
	if (!me.connected)
		return;

	sts(":%s SVSMODE %s +v", source, target);
	sts(":%s SVHOST %s :%s", me.name, target, host);
}

static void solidircd_fnc_sts(user_t *source, user_t *u, char *newnick, int type)
{
	sts(":%s SVSNICK %s %s %lu", source->nick, u->nick, newnick,
			(unsigned long)(CURRTIME - 60));
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
	/*
	 *  -> :proteus.malkier.net SJOIN 1073516550 #shrike +tn :@sycobuny @+rakaur
	 *      also:
	 *  -> :nenolod_ SJOIN 1117334567 #chat
	 */

	channel_t *c;
	boolean_t keep_new_modes = TRUE;
	uint8_t userc;
	char *userv[256];
	uint8_t i;
	time_t ts;
	char *p;

	if (origin && parc >= 4)
	{
		/* :origin SJOIN ts chan modestr [key or limits] :users */
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
				slog(LG_INFO, "m_sjoin(): server %s changing TS on %s from %ld to 0", origin, c->name, (long)c->ts);
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
			 * also clear all bans and the topic
			 */

			clear_simple_modes(c);
			chanban_clear(c);
			handle_topic(c, "", 0, "");

			LIST_FOREACH(n, c->members.head)
			{
				cu = (chanuser_t *)n->data;
				if (cu->user->server == me.me)
				{
					/* it's a service, reop */
					sts(":%s PART %s :Reop", cu->user->nick, c->name);
					sts(":%s SJOIN %ld %s + :@%s", me.name, ts, c->name, cu->user->nick);
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
				chanuser_add(c, p);
			}
	}
	else
	{
		c = channel_find(parv[1]);
		ts = atol(parv[0]);

		if (c == NULL || ts < c->ts)
		{
			/* just request a resynch, this will include
			 * the user joining -- jilles */
			slog(LG_DEBUG, "m_sjoin(): requesting resynch for %s",
					parv[1]);
			sts("RESYNCH %s", parv[1]);
			return;
		}

		chanuser_add(c, origin);
	}
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
	struct in_addr ip;
	char ipstring[64];

	/* -> NICK jilles 1 1136143909 +oi ~jilles 192.168.1.5 jaguar.test 0 3232235781 :Jilles Tjoelker */
	if (parc == 10)
	{
		s = server_find(parv[6]);
		if (!s)
		{
			slog(LG_DEBUG, "m_nick(): new user on nonexistant server: %s", parv[6]);
			return;
		}

		slog(LG_DEBUG, "m_nick(): new user on `%s': %s", s->name, parv[0]);

		ip.s_addr = ntohl(strtoul(parv[8], NULL, 10));
		ipstring[0] = '\0';
		if (!inet_ntop(AF_INET, &ip, ipstring, sizeof ipstring))
			ipstring[0] = '\0';
		u = user_add(parv[0], parv[4], parv[5], NULL, ipstring, NULL, parv[9], s, atoi(parv[2]));

		user_mode(u, parv[3]);

		/* Ok, we have the user ready to go.
		 * Here's the deal -- if the user's SVID is before
		 * the start time, and not 0, then check to see
		 * if it's a registered account or not.
		 *
		 * If it IS registered, deal with that accordingly,
		 * via handle_burstlogin(). --nenolod
		 */
		/* Changed to just check umode +r for now -- jilles */
		/* This is ok because this ircd clears +r on nick changes
		 * -- jilles */
		if (strchr(parv[3], 'r'))
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
		if (nicksvs.me != NULL && u->myuser != NULL && !(u->myuser->flags & MU_WAITAUTH) && irccasecmp(u->nick, parv[0]) && !irccasecmp(parv[0], u->myuser->name))
			/* changed nick to registered one, reset +r */
			sts(":%s SVSMODE %s +rd %ld", nicksvs.nick, parv[0], time(NULL));

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
	user_delete(user_find(origin));
}

static void m_mode(char *origin, uint8_t parc, char *parv[])
{
	channel_t *c;

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
	{
		c = channel_find(parv[0]);
		if (c == NULL)
		{
			slog(LG_DEBUG, "m_mode(): unknown channel %s", parv[0]);
			return;
		}
		if (atol(parv[1]) > c->ts)
			return;
		channel_mode(NULL, channel_find(parv[0]), parc - 2, &parv[2]);
	}
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
	if (is_internal_client(u))
	{
		slog(LG_DEBUG, "m_kick(): i got kicked from `%s'; rejoining", parv[0]);
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
	server_add(parv[0], atoi(parv[1]), origin ? origin : me.name, NULL, parv[2]);

	if (cnt.server == 2)
		me.actual = sstrdup(parv[0]);
	else
	{
		/* elicit PONG for EOB detection; pinging uplink is
		 * already done elsewhere -- jilles
		 */
		sts(":%s PING %s %s", me.name, me.name, parv[0]);
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

static void m_svhost(char *origin, uint8_t parc, char *parv[])
{
	user_t *u = user_find(parv[0]);

	if (!u)
		return;

	strlcpy(u->vhost, parv[1], HOSTLEN);
}

void _modinit(module_t * m)
{
	/* Symbol relocation voodoo. */
	server_login = &solidircd_server_login;
	introduce_nick = &solidircd_introduce_nick;
	quit_sts = &solidircd_quit_sts;
	wallops = &solidircd_wallops;
	join_sts = &solidircd_join_sts;
	chan_lowerts = &solidircd_chan_lowerts;
	kick = &solidircd_kick;
	msg = &solidircd_msg;
	notice_sts = &solidircd_notice;
	wallchops = &solidircd_wallchops;
	numeric_sts = &solidircd_numeric_sts;
	skill = &solidircd_skill;
	part = &solidircd_part;
	kline_sts = &solidircd_kline_sts;
	unkline_sts = &solidircd_unkline_sts;
	topic_sts = &solidircd_topic_sts;
	mode_sts = &solidircd_mode_sts;
	ping_sts = &solidircd_ping_sts;
	ircd_on_login = &solidircd_on_login;
	ircd_on_logout = &solidircd_on_logout;
	jupe = &solidircd_jupe;
	sethost_sts = &solidircd_sethost_sts;
	fnc_sts = &solidircd_fnc_sts;
	invite_sts = &solidircd_invite_sts;

	mode_list = solidircd_mode_list;
	ignore_mode_list = solidircd_ignore_mode_list;
	status_mode_list = solidircd_status_mode_list;
	prefix_mode_list = solidircd_prefix_mode_list;

	ircd = &Solidircd;

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
	pcommand_add("SVHOST", m_svhost);

	m->mflags = MODTYPE_CORE;

	pmodule_loaded = TRUE;
}
