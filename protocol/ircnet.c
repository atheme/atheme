/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains protocol support for IRCnet ircd's.
 * Derived mainly from the documentation (or lack thereof)
 * in my protocol bridge.
 *
 * $Id: ircnet.c 5628 2006-07-01 23:38:42Z jilles $
 */

#include "atheme.h"
#include "protocol/ircnet.h"

DECLARE_MODULE_V1("protocol/ircnet", TRUE, _modinit, NULL, "$Id: ircnet.c 5628 2006-07-01 23:38:42Z jilles $", "Atheme Development Group <http://www.atheme.org>");

/* *INDENT-OFF* */

ircd_t IRCNet = {
        "ircd 2.11.1p1 or later",       /* IRCd name */
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
	PROTOCOL_IRCNET,		/* Protocol type */
	0,                              /* Permanent cmodes */
	"beIR",                         /* Ban-like cmodes */
	'e',                            /* Except mchar */
	'I'                             /* Invex mchar */
};

struct cmode_ ircnet_mode_list[] = {
  { 'i', CMODE_INVITE },
  { 'm', CMODE_MOD    },
  { 'n', CMODE_NOEXT  },
  { 'p', CMODE_PRIV   },
  { 's', CMODE_SEC    },
  { 't', CMODE_TOPIC  },
  { '\0', 0 }
};

struct extmode ircnet_ignore_mode_list[] = {
  { '\0', 0 }
};

struct cmode_ ircnet_status_mode_list[] = {
  { 'o', CMODE_OP    },
  { 'v', CMODE_VOICE },
  { '\0', 0 }
};

struct cmode_ ircnet_prefix_mode_list[] = {
  { '@', CMODE_OP    },
  { '+', CMODE_VOICE },
  { '\0', 0 }
};

/* *INDENT-ON* */

/* login to our uplink */
static uint8_t ircnet_server_login(void)
{
	int8_t ret;

	ret = sts("PASS %s 0211010000 IRC|aDEFiIJMuw P", curr_uplink->pass);
	if (ret == 1)
		return 1;

	me.bursting = TRUE;

	sts("SERVER %s 1 %s :%s", me.name, me.numeric, me.desc);

	services_init();

	sts(":%s EOB", me.numeric);

	return 0;
}

/* introduce a client */
static void ircnet_introduce_nick(char *nick, char *user, char *host, char *real, char *uid)
{
	sts(":%s UNICK %s %s %s %s 0.0.0.0 +%s :%s", me.numeric, nick, uid, user, host, "io", real);
}

/* invite a user to a channel */
static void ircnet_invite_sts(user_t *sender, user_t *target, channel_t *channel)
{
	int joined = 0;

	/* Need to join to invite -- jilles */
	if (!chanuser_find(channel, sender))
	{
		sts(":%s NJOIN %s :@%s", ME, channel->name, CLIENT_NAME(sender));
		joined = 1;
	}
	/* ircnet's UID implementation is incomplete, in many places,
	 * like this one, it does not accept UIDs -- jilles */
	sts(":%s INVITE %s %s", CLIENT_NAME(sender), target->nick, channel->name);
	if (joined)
		sts(":%s PART %s :Invited %s", CLIENT_NAME(sender), channel->name, target->nick);
}

static void ircnet_quit_sts(user_t *u, char *reason)
{
	if (!me.connected)
		return;

	sts(":%s QUIT :%s", u->nick, reason);
}

/* WALLOPS wrapper */
static void ircnet_wallops(char *fmt, ...)
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

/* join a channel */
static void ircnet_join_sts(channel_t *c, user_t *u, boolean_t isnew, char *modes)
{
	sts(":%s NJOIN %s :@%s", me.numeric, c->name, u->uid);
	if (isnew && modes[0] && modes[1])
		sts(":%s MODE %s %s", me.numeric, c->name, modes);
}

/* kicks a user from a channel */
static void ircnet_kick(char *from, char *channel, char *to, char *reason)
{
	channel_t *chan = channel_find(channel);
	user_t *user = user_find(to);
	user_t *from_p = user_find(from);

	if (!chan || !user)
		return;

	/* sigh server kicks will generate snotes
	 * but let's avoid joining N times for N kicks */
	sts(":%s KICK %s %s :%s", from_p != NULL && chanuser_find(chan, from_p) ? CLIENT_NAME(from_p) : ME, channel, CLIENT_NAME(user), reason);

	chanuser_delete(chan, user);
}

/* PRIVMSG wrapper */
static void ircnet_msg(char *from, char *target, char *fmt, ...)
{
	va_list ap;
	char buf[BUFSIZE];

	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZE, fmt, ap);
	va_end(ap);

	sts(":%s PRIVMSG %s :%s", from, target, buf);
}

/* NOTICE wrapper */
static void ircnet_notice(char *from, char *target, char *fmt, ...)
{
	va_list ap;
	char buf[BUFSIZE];
	user_t *u = user_find(from);
	user_t *t = user_find(target);

	if (u == NULL && (from == NULL || (irccasecmp(from, me.name) && irccasecmp(from, ME))))
	{
		slog(LG_DEBUG, "ircnet_notice(): unknown source %s for notice to %s", from, target);
		return;
	}

	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZE, fmt, ap);
	va_end(ap);

	if (u == NULL || target[0] != '#' || chanuser_find(channel_find(target), user_find(from)))
		sts(":%s NOTICE %s :%s", u ? CLIENT_NAME(u) : ME, t ? CLIENT_NAME(t) : target, buf);
	else
		sts(":%s NOTICE %s :%s: %s", ME, target, u->nick, buf);
}

/* numeric wrapper */
static void ircnet_numeric_sts(char *from, int numeric, char *target, char *fmt, ...)
{
	va_list ap;
	char buf[BUFSIZE];

	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZE, fmt, ap);
	va_end(ap);

	sts(":%s %d %s %s", from, numeric, target, buf);
}

/* KILL wrapper */
static void ircnet_skill(char *from, char *nick, char *fmt, ...)
{
	va_list ap;
	char buf[BUFSIZE];

	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZE, fmt, ap);
	va_end(ap);

	sts(":%s KILL %s :%s!%s!%s (%s)", from, nick, from, from, from, buf);
}

/* PART wrapper */
static void ircnet_part(char *chan, char *nick)
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
static void ircnet_kline_sts(char *server, char *user, char *host, long duration, char *reason)
{
	if (!me.connected)
		return;

	/* this won't propagate!
	 * you'll need some bot/service on each server to do that */
	if (irccasecmp(server, me.actual) && cnt.server > 2)
		wallops("Missed a tkline");
	sts(":%s TKLINE %lds %s@%s :%s", opersvs.nick, duration, user, host, reason);
}

/* server-to-server UNKLINE wrapper */
static void ircnet_unkline_sts(char *server, char *user, char *host)
{
	if (!me.connected)
		return;

	if (irccasecmp(server, me.actual) && cnt.server > 2)
		wallops("Missed an untkline");
	sts(":%s UNTKLINE %s@%s", opersvs.nick, user, host);
}

/* topic wrapper */
static void ircnet_topic_sts(char *channel, char *setter, time_t ts, char *topic)
{
	channel_t *c;
	int joined = 0;

	c = channel_find(channel);
	if (!me.connected || !c)
		return;

	/* Need to join to set topic -- jilles */
	if (!chanuser_find(c, chansvs.me->me))
	{
		sts(":%s NJOIN %s :@%s", ME, channel, CLIENT_NAME(chansvs.me->me));
		joined = 1;
	}
	sts(":%s TOPIC %s :%s", CLIENT_NAME(chansvs.me->me), channel, topic);
	if (joined)
		sts(":%s PART %s :Topic set", CLIENT_NAME(chansvs.me->me), channel);
}

/* mode wrapper */
static void ircnet_mode_sts(char *sender, char *target, char *modes)
{
	user_t *u;

	if (!me.connected)
		return;

	u = user_find(sender);

	/* send it from the server if that service isn't on channel
	 * -- jilles */
	sts(":%s MODE %s %s", chanuser_find(channel_find(target), u) ? CLIENT_NAME(u) : ME, target, modes);
}

/* ping wrapper */
static void ircnet_ping_sts(void)
{
	if (!me.connected)
		return;

	sts("PING :%s", me.name);
}

/* protocol-specific stuff to do on login */
static void ircnet_on_login(char *origin, char *user, char *wantedhost)
{
	/* nothing to do on ratbox */
	return;
}

/* protocol-specific stuff to do on login */
static boolean_t ircnet_on_logout(char *origin, char *user, char *wantedhost)
{
	/* nothing to do on ratbox */
	return FALSE;
}

static void ircnet_jupe(char *server, char *reason)
{
	static char sid[4+1];
	int i;

	if (!me.connected)
		return;

	sts(":%s SQUIT %s :%s", opersvs.nick, server, reason);

	/* dirty dirty make up some sid */
	if (sid[0] == '\0')
		strlcpy(sid, me.numeric, sizeof sid);
	do
	{
		i = 3;
		for (;;)
		{
			if (sid[i] == 'Z')
			{
				sid[i] = '0';
				i--;
				/* eek, no more sids */
				if (i < 0)
					return;
				continue;
			}
			else if (sid[i] == '9')
				sid[i] = 'A';
			else sid[i]++;
			break;
		}
	} while (server_find(sid));

	sts(":%s SERVER %s 2 %s 0211010000 :%s", me.name, server, sid, reason);
}

static void m_topic(char *origin, uint8_t parc, char *parv[])
{
	channel_t *c = channel_find(parv[0]);
	user_t *u = user_find(origin);

	if (!c || !u)
		return;

	handle_topic(c, u->nick, CURRTIME, parv[1]);
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
}

static void m_eob(char *origin, uint8_t parc, char *parv[])
{
	server_t *source = server_find(origin), *serv;
	char sidbuf[4+1], *p;

	if (source == NULL)
	{
		slog(LG_DEBUG, "m_eob(): Got EOB from unknown server %s", origin);
	}
	handle_eob(source);
	if (parc >= 1)
	{
		sidbuf[4] = '\0';
		p = parv[0];
		while (p[0] && p[1] && p[2] && p[3])
		{
			memcpy(sidbuf, p, 4);
			serv = server_find(sidbuf);
			handle_eob(serv);
			if (p[4] != ',')
				break;
			p += 5;
		}
	}

	if (me.bursting)
	{
		sts(":%s EOBACK", me.numeric);
#ifdef HAVE_GETTIMEOFDAY
		e_time(burstime, &burstime);

		slog(LG_INFO, "m_eob(): finished synching with uplink (%d %s)", (tv2ms(&burstime) > 1000) ? (tv2ms(&burstime) / 1000) : tv2ms(&burstime), (tv2ms(&burstime) > 1000) ? "s" : "ms");

		wallops("Finished synching to network in %d %s.", (tv2ms(&burstime) > 1000) ? (tv2ms(&burstime) / 1000) : tv2ms(&burstime), (tv2ms(&burstime) > 1000) ? "s" : "ms");
#else
		slog(LG_INFO, "m_eob(): finished synching with uplink");
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

static void m_njoin(char *origin, uint8_t parc, char *parv[])
{
	channel_t *c;
	uint8_t userc;
	char *userv[256];
	uint8_t i;
	server_t *source;

	source = server_find(origin);
	if (source != NULL)
	{
		c = channel_find(parv[0]);

		if (!c)
		{
			slog(LG_DEBUG, "m_njoin(): new channel: %s", parv[0]);
			/* Give channels created during burst an older "TS"
			 * so they won't be deopped -- jilles */
			c = channel_add(parv[0], source->flags & SF_EOB ? CURRTIME : CURRTIME - 601);
			/* Check mode locks */
			channel_mode_va(NULL, c, 1, "+");
		}

		userc = sjtoken(parv[parc - 1], ',', userv);

		for (i = 0; i < userc; i++)
			chanuser_add(c, userv[i]);
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

	/* got the right number of args for an introduction? */
	if (parc == 7)
	{
		s = server_find(origin);
		if (!s)
		{
			slog(LG_DEBUG, "m_nick(): new user on nonexistant server: %s", origin);
			return;
		}

		slog(LG_DEBUG, "m_nick(): new user on `%s': %s", s->name, parv[0]);

		u = user_add(parv[0], parv[2], parv[3], NULL, parv[4], parv[1], parv[6], s, 0);

		user_mode(u, parv[5]);

		handle_nickchange(u);
	}

	/* if it's only 1 then it's a nickname change */
	else if (parc == 1)
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
	server_add(parv[0], atoi(parv[1]), origin ? origin : me.name, parv[2], parv[parc - 1]);

	if (cnt.server == 2)
		me.actual = sstrdup(parv[0]);
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

static void m_motd(char *origin, uint8_t parc, char *parv[])
{
	handle_motd(user_find(origin));
}

void _modinit(module_t * m)
{
	/* Symbol relocation voodoo. */
	server_login = &ircnet_server_login;
	introduce_nick = &ircnet_introduce_nick;
	quit_sts = &ircnet_quit_sts;
	wallops = &ircnet_wallops;
	join_sts = &ircnet_join_sts;
	kick = &ircnet_kick;
	msg = &ircnet_msg;
	notice_sts = &ircnet_notice;
	/* no wallchops, ircnet ircd does not support this */
	numeric_sts = &ircnet_numeric_sts;
	skill = &ircnet_skill;
	part = &ircnet_part;
	kline_sts = &ircnet_kline_sts;
	unkline_sts = &ircnet_unkline_sts;
	topic_sts = &ircnet_topic_sts;
	mode_sts = &ircnet_mode_sts;
	ping_sts = &ircnet_ping_sts;
	ircd_on_login = &ircnet_on_login;
	ircd_on_logout = &ircnet_on_logout;
	jupe = &ircnet_jupe;
	invite_sts = &ircnet_invite_sts;

	mode_list = ircnet_mode_list;
	ignore_mode_list = ircnet_ignore_mode_list;
	status_mode_list = ircnet_status_mode_list;
	prefix_mode_list = ircnet_prefix_mode_list;

	ircd = &IRCNet;

	pcommand_add("PING", m_ping);
	pcommand_add("PONG", m_pong);
	pcommand_add("EOB", m_eob);
	pcommand_add("PRIVMSG", m_privmsg);
	pcommand_add("NOTICE", m_notice);
	pcommand_add("NJOIN", m_njoin);
	pcommand_add("PART", m_part);
	pcommand_add("NICK", m_nick);
	pcommand_add("UNICK", m_nick);
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
	pcommand_add("MOTD", m_motd);

	m->mflags = MODTYPE_CORE;

	pmodule_loaded = TRUE;
}
