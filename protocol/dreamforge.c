/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains protocol support for bahamut-based ircd.
 *
 * $Id: dreamforge.c 5628 2006-07-01 23:38:42Z jilles $
 */

#include "atheme.h"
#include "protocol/dreamforge.h"

DECLARE_MODULE_V1("protocol/dreamforge", TRUE, _modinit, NULL, "$Id: dreamforge.c 5628 2006-07-01 23:38:42Z jilles $", "Atheme Development Group <http://www.atheme.org>");

/* *INDENT-OFF* */

ircd_t DreamForge = {
        "DreamForge 4.6.7 and later",   /* IRCd name */
        "$",                            /* TLD Prefix, used by Global. */
        FALSE,                          /* Whether or not we use IRCNet/TS6 UID */
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
	PROTOCOL_DREAMFORGE,		/* Protocol type */
	0,                              /* Permanent cmodes */
	"b",                            /* Ban-like cmodes */
	0,                              /* Except mchar */
	0                               /* Invex mchar */
};

struct cmode_ dreamforge_mode_list[] = {
  { 'i', CMODE_INVITE   },
  { 'm', CMODE_MOD      },
  { 'n', CMODE_NOEXT    },
  { 'p', CMODE_PRIV     },
  { 's', CMODE_SEC      },
  { 't', CMODE_TOPIC    },
  { 'c', CMODE_NOCOLOR  },
  { 'R', CMODE_REGONLY  },
  { '\0', 0 }
};

struct extmode dreamforge_ignore_mode_list[] = {
  { '\0', 0 }
};

struct cmode_ dreamforge_status_mode_list[] = {
  { 'o', CMODE_OP      },
  { 'v', CMODE_VOICE   },
  { '\0', 0 }
};

struct cmode_ dreamforge_prefix_mode_list[] = {
  { '@', CMODE_OP      },
  { '+', CMODE_VOICE   },
  { '\0', 0 }
};

/* *INDENT-ON* */

/* login to our uplink */
static uint8_t dreamforge_server_login(void)
{
	int8_t ret;

	ret = sts("PASS %s", curr_uplink->pass);
	if (ret == 1)
		return 1;

	me.bursting = TRUE;

	sts("PROTOCTL NOQUIT WATCH=128 SAFELIST");
	sts("SERVER %s 1 :%s", me.name, me.desc);

	services_init();

	return 0;
}

/* introduce a client */
static void dreamforge_introduce_nick(char *nick, char *user, char *host, char *real, char *uid)
{
	sts("NICK %s 1 %ld %s %s %s 0 :%s", nick, CURRTIME, user, host, me.name, real);
	sts(":%s MODE %s +%s", nick, nick, "io");
}

/* invite a user to a channel */
static void dreamforge_invite_sts(user_t *sender, user_t *target, channel_t *channel)
{
	sts(":%s INVITE %s %s", sender->nick, target->nick, channel->name);
}

static void dreamforge_quit_sts(user_t *u, char *reason)
{
	if (!me.connected)
		return;

	sts(":%s QUIT :%s", u->nick, reason);
}

/* WALLOPS wrapper */
static void dreamforge_wallops(char *fmt, ...)
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
static void dreamforge_join_sts(channel_t *c, user_t *u, boolean_t isnew, char *modes)
{
	if (isnew)
	{
		sts(":%s JOIN %s", u->nick, c->name);
		if (modes[0] && modes[1])
			sts(":%s MODE %s %s", u->nick, c->name, modes);
	}
	else
	{
		sts(":%s JOIN %s", u->nick, c->name);
	}
	sts(":%s MODE %s +o %s %ld", u->nick, c->name, u->nick, c->ts);
}

/* kicks a user from a channel */
static void dreamforge_kick(char *from, char *channel, char *to, char *reason)
{
	channel_t *chan = channel_find(channel);
	user_t *user = user_find(to);

	if (!chan || !user)
		return;

	sts(":%s KICK %s %s :%s", from, channel, to, reason);

	chanuser_delete(chan, user);
}

/* PRIVMSG wrapper */
static void dreamforge_msg(char *from, char *target, char *fmt, ...)
{
	va_list ap;
	char buf[BUFSIZE];

	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZE, fmt, ap);
	va_end(ap);

	sts(":%s PRIVMSG %s :%s", from, target, buf);
}

/* NOTICE wrapper */
static void dreamforge_notice(char *from, char *target, char *fmt, ...)
{
	va_list ap;
	char buf[BUFSIZE];

	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZE, fmt, ap);
	va_end(ap);

	sts(":%s NOTICE %s :%s", from, target, buf);
}

static void dreamforge_numeric_sts(char *from, int numeric, char *target, char *fmt, ...)
{
	va_list ap;
	char buf[BUFSIZE];

	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZE, fmt, ap);
	va_end(ap);

	sts(":%s %d %s %s", from, numeric, target, buf);
}

/* KILL wrapper */
static void dreamforge_skill(char *from, char *nick, char *fmt, ...)
{
	va_list ap;
	char buf[BUFSIZE];

	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZE, fmt, ap);
	va_end(ap);

	sts(":%s KILL %s :%s!%s!%s (%s)", from, nick, from, from, from, buf);
}

/* PART wrapper */
static void dreamforge_part(char *chan, char *nick)
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
static void dreamforge_kline_sts(char *server, char *user, char *host, long duration, char *reason)
{
	if (!me.connected)
		return;

	sts(":%s AKILL %s %s %ld %s %ld :%s", me.name, host, user, duration, opersvs.nick, time(NULL), reason);
}

/* server-to-server UNKLINE wrapper */
static void dreamforge_unkline_sts(char *server, char *user, char *host)
{
	if (!me.connected)
		return;

	sts(":%s RAKILL %s %s", me.name, host, user);
}

/* topic wrapper */
static void dreamforge_topic_sts(char *channel, char *setter, time_t ts, char *topic)
{
	if (!me.connected)
		return;

	sts(":%s TOPIC %s %s %ld :%s", chansvs.nick, channel, setter, ts, topic);
}

/* mode wrapper */
static void dreamforge_mode_sts(char *sender, char *target, char *modes)
{
	if (!me.connected)
		return;

	sts(":%s MODE %s %s", sender, target, modes);
}

/* ping wrapper */
static void dreamforge_ping_sts(void)
{
	if (!me.connected)
		return;

	sts("PING :%s", me.name);
}

/* protocol-specific stuff to do on login */
static void dreamforge_on_login(char *origin, char *user, char *wantedhost)
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
static boolean_t dreamforge_on_logout(char *origin, char *user, char *wantedhost)
{
	if (!me.connected)
		return FALSE;

	if (nicksvs.me == NULL || irccasecmp(origin, user))
		return FALSE;

	sts(":%s SVSMODE %s -r+d %ld", nicksvs.nick, origin, time(NULL));
	return FALSE;
}

static void dreamforge_jupe(char *server, char *reason)
{
	if (!me.connected)
		return;

	sts(":%s SQUIT %s :%s", opersvs.nick, server, reason);
	sts(":%s SERVER %s 2 :%s", me.name, server, reason);
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

	if (parc == 8)
	{
		s = server_find(parv[5]);
		if (!s)
		{
			slog(LG_DEBUG, "m_nick(): new user on nonexistant server: %s", parv[6]);
			return;
		}

		slog(LG_DEBUG, "m_nick(): new user on `%s': %s", s->name, parv[0]);

		user_add(parv[0], parv[3], parv[4], NULL, NULL, NULL, parv[7], s, atoi(parv[2]));

		/* Note: cannot rely on umode +r to see if they're identified
		 * -- jilles */

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

		/* fix up +r if necessary -- jilles */
		if (nicksvs.me != NULL && u->myuser != NULL && !(u->myuser->flags & MU_WAITAUTH) && irccasecmp(u->nick, parv[0]))
		{
			if (!irccasecmp(parv[0], u->myuser->name))
				/* changed nick to registered one, reset +r */
				sts(":%s SVSMODE %s +rd %ld", nicksvs.nick, parv[0], CURRTIME);
			else if (!irccasecmp(u->nick, u->myuser->name))
				/* changed from registered nick, remove +r */
				sts(":%s SVSMODE %s -r+d %ld", nicksvs.nick, parv[0], CURRTIME);
		}

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
	user_t *u;

	u = user_find(origin);
	if (u == NULL)
		return;
	if (u->server == me.me)
	{
		slog(LG_DEBUG, "m_quit(): not destroying own user %s (fake direction)", origin);
		return;
	}
	slog(LG_DEBUG, "m_quit(): user leaving: %s", origin);

	/* user_delete() takes care of removing channels and so forth */
	user_delete(u);
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
	uint8_t chanc;
	char *chanv[256];
	int i;

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
	else
	{
		chanc = sjtoken(parv[0], ',', chanv);
		for (i = 0; i < chanc; i++)
		{
			channel_t *c = channel_find(chanv[i]);

			if (!c)
			{
				slog(LG_DEBUG, "m_join(): new channel: %s", parv[0]);
				c = channel_add(chanv[i], CURRTIME);
			}
			cu = chanuser_add(c, origin);
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
	server_login = &dreamforge_server_login;
	introduce_nick = &dreamforge_introduce_nick;
	quit_sts = &dreamforge_quit_sts;
	wallops = &dreamforge_wallops;
	join_sts = &dreamforge_join_sts;
	kick = &dreamforge_kick;
	msg = &dreamforge_msg;
	notice_sts = &dreamforge_notice;
	numeric_sts = &dreamforge_numeric_sts;
	skill = &dreamforge_skill;
	part = &dreamforge_part;
	kline_sts = &dreamforge_kline_sts;
	unkline_sts = &dreamforge_unkline_sts;
	topic_sts = &dreamforge_topic_sts;
	mode_sts = &dreamforge_mode_sts;
	ping_sts = &dreamforge_ping_sts;
	ircd_on_login = &dreamforge_on_login;
	ircd_on_logout = &dreamforge_on_logout;
	jupe = &dreamforge_jupe;
	invite_sts = &dreamforge_invite_sts;

	mode_list = dreamforge_mode_list;
	ignore_mode_list = dreamforge_ignore_mode_list;
	status_mode_list = dreamforge_status_mode_list;
	prefix_mode_list = dreamforge_prefix_mode_list;

	ircd = &DreamForge;

	pcommand_add("PING", m_ping);
	pcommand_add("PONG", m_pong);
	pcommand_add("PRIVMSG", m_privmsg);
	pcommand_add("NOTICE", m_notice);
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
	pcommand_add("MOTD", m_motd);

	m->mflags = MODTYPE_CORE;

	pmodule_loaded = TRUE;
}
