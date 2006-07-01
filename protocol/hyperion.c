/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains protocol support for hyperion-based ircd.
 *
 * $Id: hyperion.c 5628 2006-07-01 23:38:42Z jilles $
 */

/* option: use SVSLOGIN/SIGNON to remember users even if they're
 * not logged in to their current nick, etc
 */
#define USE_SVSLOGIN

#include "atheme.h"
#include "protocol/hyperion.h"

DECLARE_MODULE_V1("protocol/hyperion", TRUE, _modinit, NULL, "$Id: hyperion.c 5628 2006-07-01 23:38:42Z jilles $", "Atheme Development Group <http://www.atheme.org>");

/* *INDENT-OFF* */

ircd_t Hyperion = {
        "Hyperion 1.0",                 /* IRCd name */
        "$",                            /* TLD Prefix, used by Global. */
        FALSE,                          /* Whether or not we use IRCNet/TS6 UID */
        FALSE,                          /* Whether or not we use RCOMMAND */
        FALSE,                          /* Whether or not we support channel owners. */
        FALSE,                          /* Whether or not we support channel protection. */
        FALSE,                          /* Whether or not we support halfops. */
	FALSE,				/* Whether or not we use P10 */
	TRUE,				/* Whether or not we use vHosts. */
	CMODE_EXLIMIT | CMODE_PERM | CMODE_JUPED,	/* Oper-only cmodes */
        0,                              /* Integer flag for owner channel flag. */
        0,                              /* Integer flag for protect channel flag. */
        0,                              /* Integer flag for halfops. */
        "+",                            /* Mode we set for owner. */
        "+",                            /* Mode we set for protect. */
        "+",                            /* Mode we set for halfops. */
	PROTOCOL_HYPERION,		/* Protocol type */
	CMODE_PERM | CMODE_JUPED,       /* Permanent cmodes */
	"beIqd",                        /* Ban-like cmodes */
	'e',                            /* Except mchar */
	'I'                             /* Invex mchar */
};

struct cmode_ hyperion_mode_list[] = {
  { 'i', CMODE_INVITE },
  { 'm', CMODE_MOD    },
  { 'n', CMODE_NOEXT  },
  { 's', CMODE_SEC    },
  { 't', CMODE_TOPIC  },
  { 'c', CMODE_NOCOLOR},
  { 'r', CMODE_REGONLY},
  { 'R', CMODE_MODREG },
  { 'z', CMODE_OPMOD  },
  { 'g', CMODE_FINVITE},
  { 'L', CMODE_EXLIMIT},
  { 'P', CMODE_PERM   },
  { 'j', CMODE_JUPED  },
  { 'Q', CMODE_DISFWD },
  { '\0', 0 }
};

static boolean_t check_forward(const char *, channel_t *, mychan_t *, user_t *, myuser_t *);
static boolean_t check_jointhrottle(const char *, channel_t *, mychan_t *, user_t *, myuser_t *);

struct extmode hyperion_ignore_mode_list[] = {
  /*{ 'D', 0 },*/
  { 'f', check_forward },
  { 'J', check_jointhrottle },
  { '\0', 0 }
};

struct cmode_ hyperion_status_mode_list[] = {
  { 'o', CMODE_OP    },
  { 'v', CMODE_VOICE },
  { '\0', 0 }
};

struct cmode_ hyperion_prefix_mode_list[] = {
  { '@', CMODE_OP    },
  { '+', CMODE_VOICE },
  { '\0', 0 }
};

static boolean_t use_svslogin = FALSE;

/* *INDENT-ON* */

static boolean_t check_forward(const char *value, channel_t *c, mychan_t *mc, user_t *u, myuser_t *mu)
{
	channel_t *target_c;
	mychan_t *target_mc;

	if (*value != '#' || strlen(value) > 50)
		return FALSE;
	if (u == NULL && mu == NULL)
		return TRUE;
	target_c = channel_find(value);
	target_mc = mychan_find(value);
	if (target_c == NULL && target_mc == NULL)
		return FALSE;
	return TRUE;
}

static boolean_t check_jointhrottle(const char *value, channel_t *c, mychan_t *mc, user_t *u, myuser_t *mu)
{
	const char *p, *arg2;

	p = value, arg2 = NULL;
	while (*p != '\0')
	{
		if (*p == ',')
		{
			if (arg2 != NULL)
				return FALSE;
			arg2 = p + 1;
		}
		else if (!isdigit(*p))
			return FALSE;
		p++;
	}
	if (arg2 == NULL)
		return FALSE;
	if (p - arg2 > 5 || arg2 - value - 1 > 5 || !atoi(value) || !atoi(arg2))
		return FALSE;
	return TRUE;
}

/* login to our uplink */
static uint8_t hyperion_server_login(void)
{
	int8_t ret;

	ret = sts("PASS %s :TS", curr_uplink->pass);
	if (ret == 1)
		return 1;

	me.bursting = TRUE;

	sts("CAPAB :QS EX DE CHW IE QU DNCR SRV SIGNON");
	sts("SERVER %s 1 :%s", me.name, me.desc);
	sts("SVINFO 5 3 0 :%ld", CURRTIME);

	services_init();

	return 0;
}

/* introduce a nick.
 * Hyperion sucks because it has to do things differently.
 * Thus we have to make all clients opered, because there is no real way to get
 * around the problem.
 *
 * Protocol information ripped from theia/dancer-services (Hybserv2 TS of some sort).
 *    -- nenolod
 */
static void hyperion_introduce_nick(char *nick, char *user, char *host, char *real, char *uid)
{
	const char *privs = "6@BFmMopPRUX";
	sts("NICK %s 1 %ld +ei%s %s %s %s 0.0.0.0 :%s", nick, CURRTIME, privs, user, host, me.name, real);
	sts(":%s OPER %s +%s", me.name, nick, privs);
}

/* invite a user to a channel */
static void hyperion_invite_sts(user_t *sender, user_t *target, channel_t *channel)
{
	sts(":%s INVITE %s %s", sender->nick, target->nick, channel->name);
}

static void hyperion_quit_sts(user_t *u, char *reason)
{
	if (!me.connected)
		return;

	sts(":%s QUIT :%s", u->nick, reason);
}

/* WALLOPS wrapper */
static void hyperion_wallops(char *fmt, ...)
{
	va_list ap;
	char buf[BUFSIZE];

	if (config_options.silent)
		return;

	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZE, fmt, ap);
	va_end(ap);

	/* Generate +s server notice -- jilles */
	sts(":%s WALLOPS 1-0 :%s", me.name, buf);
}

/* join a channel */
static void hyperion_join_sts(channel_t *c, user_t *u, boolean_t isnew, char *modes)
{
	if (isnew)
		sts(":%s SJOIN %ld %s %s :@%s", me.name, c->ts, c->name,
				modes, u->nick);
	else
		sts(":%s SJOIN %ld %s + :@%s", me.name, c->ts, c->name,
				u->nick);
}

/* kicks a user from a channel */
static void hyperion_kick(char *from, char *channel, char *to, char *reason)
{
	channel_t *chan = channel_find(channel);
	user_t *user = user_find(to);

	if (!chan || !user)
		return;

	sts(":%s KICK %s %s :%s", from, channel, to, reason);

	chanuser_delete(chan, user);
}

/* PRIVMSG wrapper */
static void hyperion_msg(char *from, char *target, char *fmt, ...)
{
	va_list ap;
	char buf[BUFSIZE];

	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZE, fmt, ap);
	va_end(ap);

	sts(":%s PRIVMSG %s :%s", from, target, buf);
}

/* NOTICE wrapper */
static void hyperion_notice(char *from, char *target, char *fmt, ...)
{
	va_list ap;
	char buf[BUFSIZE];

	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZE, fmt, ap);
	va_end(ap);

	sts(":%s NOTICE %s :%s", from, target, buf);
}

static void hyperion_wallchops(user_t *sender, channel_t *channel, char *message)
{
	/* +p does not grant the ability to send to @#channel (!) */
	if (chanuser_find(channel, sender))
		sts(":%s NOTICE @%s :%s", CLIENT_NAME(sender), channel->name,
				message);
	else /* do not join for this, everyone would see -- jilles */
		generic_wallchops(sender, channel, message);
}

/* numeric wrapper */
static void hyperion_numeric_sts(char *from, int numeric, char *target, char *fmt, ...)
{
	va_list ap;
	char buf[BUFSIZE];

	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZE, fmt, ap);
	va_end(ap);

	sts(":%s %d %s %s", from, numeric, target, buf);
}

/* KILL wrapper */
static void hyperion_skill(char *from, char *nick, char *fmt, ...)
{
	va_list ap;
	char buf[BUFSIZE];

	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZE, fmt, ap);
	va_end(ap);

	sts(":%s KILL %s :%s!%s!%s (%s)", from, nick, from, from, from, buf);
}

/* PART wrapper */
static void hyperion_part(char *chan, char *nick)
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
static void hyperion_kline_sts(char *server, char *user, char *host, long duration, char *reason)
{
	if (!me.connected)
		return;

	if (duration)
		sts(":%s KLINE %s %ld %s@%s :%s", me.name, me.name, duration > 60 ? (duration / 60) : 1, user, host, reason);
	else
		sts(":%s KLINE %s %s@%s :%s", me.name, me.name, user, host, reason);
}

/* server-to-server UNKLINE wrapper */
static void hyperion_unkline_sts(char *server, char *user, char *host)
{
	if (!me.connected)
		return;

	sts(":%s UNKLINE %s@%s %ld", opersvs.nick, user, host, CURRTIME);
}

/* topic wrapper */
static void hyperion_topic_sts(char *channel, char *setter, time_t ts, char *topic)
{
	if (!me.connected)
		return;

	/* Send 0 channelts so this will always be accepted */
	sts(":%s STOPIC %s %s %ld 0 :%s", chansvs.nick, channel, setter, ts, topic);
}

/* mode wrapper */
static void hyperion_mode_sts(char *sender, char *target, char *modes)
{
	if (!me.connected)
		return;

	sts(":%s MODE %s %s", sender, target, modes);
}

/* ping wrapper */
static void hyperion_ping_sts(void)
{
	if (!me.connected)
		return;

	sts("PING :%s", me.name);
}

/* protocol-specific stuff to do on login */
static void hyperion_on_login(char *origin, char *user, char *wantedhost)
{
	user_t *u;

	if (!me.connected)
		return;

	u = user_find(origin);
	if (!u)
		return;
	if (use_svslogin)
		sts(":%s SVSLOGIN %s %s %s %s %s %s", me.name, u->server->name, origin, user, origin, u->user, wantedhost ? wantedhost : u->vhost);
	/* we'll get a SIGNON confirming the changes later, no need
	 * to change the fields yet */

	/* set +e if they're identified to the nick they are using */
	if (nicksvs.me == NULL || irccasecmp(origin, user))
		return;

	sts(":%s MODE %s +e", me.name, origin);
}

/* protocol-specific stuff to do on login */
static boolean_t hyperion_on_logout(char *origin, char *user, char *wantedhost)
{
	user_t *u;

	if (!me.connected)
		return FALSE;

	u = user_find(origin);
	if (!u)
		return FALSE;
	if (use_svslogin)
		sts(":%s SVSLOGIN %s %s %s %s %s %s", me.name, u->server->name, origin, "0", origin, u->user, wantedhost ? u->host : u->vhost);

	if (nicksvs.me == NULL || irccasecmp(origin, user))
		return FALSE;

	sts(":%s MODE %s -e", me.name, origin);
	return FALSE;
}

static void hyperion_jupe(char *server, char *reason)
{
	if (!me.connected)
		return;

	sts(":%s SQUIT %s :%s", opersvs.nick, server, reason);
	sts(":%s SERVER %s 2 :%s", me.name, server, reason);
}

static void hyperion_sethost_sts(char *source, char *target, char *host)
{
	if (!me.connected)
		return;

	sts(":%s SETHOST %s :%s", source, target, host);
}

static void hyperion_fnc_sts(user_t *source, user_t *u, char *newnick, int type)
{
	/* XXX this should be combined with the SVSLOGIN to set login id
	 * and SETHOST, if any -- jilles */
	sts(":%s SVSLOGIN %s %s %s %s %s %s", me.name, u->server->name, u->nick, u->myuser ? u->myuser->name : "0", newnick, u->user, u->vhost);
}

static void m_topic(char *origin, uint8_t parc, char *parv[])
{
	channel_t *c = channel_find(parv[0]);
	user_t *u = user_find(origin);

	if (!c || !u)
		return;

	handle_topic(c, u->nick, CURRTIME, parv[1]);
}

static void m_stopic(char *origin, uint8_t parc, char *parv[])
{
	channel_t *c = channel_find(parv[0]);
	time_t channelts;
	time_t topicts;

	/* hyperion will propagate an STOPIC even if it's not applied
	 * locally :( */
	topicts = atol(parv[2]);
	channelts = atol(parv[3]);
	if (c->topic == NULL || (strcmp(c->topic, parv[1]) && channelts < c->ts || (channelts == c->ts && topicts > c->topicts)))
		handle_topic(c, parv[1], topicts, parv[parc - 1]);
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
	/* -> :proteus.malkier.net SJOIN 1073516550 #shrike +tn :@sycobuny @+rakaur */

	channel_t *c;
	uint8_t userc;
	char *userv[256];
	uint8_t i;
	time_t ts;

	if (origin)
	{
		/* :origin SJOIN ts chan modestr [key or limits] :users */
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

			clear_simple_modes(c);

			LIST_FOREACH(n, c->members.head)
			{
				cu = (chanuser_t *)n->data;
				if (cu->user->server == me.me)
				{
					/* it's a service, reop */
					sts(":%s MODE %s +o %s", cu->user->nick, c->name, cu->user->nick);
					cu->modes = CMODE_OP;
				}
				else
					cu->modes = 0;
			}

			slog(LG_INFO, "m_sjoin(): TS changed for %s (%ld -> %ld)", c->name, c->ts, ts);

			c->ts = ts;
			hook_call_event("channel_tschange", c);
		}

		channel_mode(NULL, c, parc - 3, parv + 2);

		userc = sjtoken(parv[parc - 1], ' ', userv);

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
	if (parc == 9)
	{
		s = server_find(parv[6]);
		if (!s)
		{
			slog(LG_DEBUG, "m_nick(): new user on nonexistant server: %s", parv[6]);
			return;
		}

		slog(LG_DEBUG, "m_nick(): new user on `%s': %s", s->name, parv[0]);

		u = user_add(parv[0], parv[4], parv[5], NULL, parv[7], NULL, parv[8], s, atoi(parv[2]));

		user_mode(u, parv[3]);

		/* umode +e: identified to current nick */
		/* As hyperion clears +e on nick changes, this is safe. */
		if (!use_svslogin && strchr(parv[3], 'e'))
			handle_burstlogin(u, parv[0]);

		/* if the server supports SIGNON, we will get an SNICK
		 * for this user, potentially with a login name
		 */
		if (!use_svslogin)
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

		/* fix up +e if necessary -- jilles */
		if (nicksvs.me != NULL && u->myuser != NULL && !(u->myuser->flags & MU_WAITAUTH) && irccasecmp(u->nick, parv[0]) && !irccasecmp(parv[0], u->myuser->name))
			/* changed nick to registered one, reset +e */
			sts(":%s MODE %s +e", me.name, parv[0]);

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
	/* serves for both KILL and COLLIDE
	 * COLLIDE only originates from servers and may not have
	 * a reason field, but the net effect is identical
	 * -- jilles */
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

static void m_capab(char *origin, uint8_t parc, char *parv[])
{
	char *p;

#ifdef USE_SVSLOGIN
	for (p = strtok(parv[0], " "); p != NULL; p = strtok(NULL, " "))
	{
		if (!irccasecmp(p, "SIGNON"))
		{
			slog(LG_DEBUG, "m_capab(): uplink can do SIGNON, enabling support.");
			use_svslogin = TRUE;
		}
	}
#endif
}

static void m_snick(char *origin, uint8_t parc, char *parv[])
{
	user_t *u;

	/* SNICK <nick> <orignick> <spoofhost> <firsttime> <dnshost> <servlogin> */
	if (parc < 5)
		return;

	u = user_find(parv[0]);

	if (!u)
		return;

	if (strcmp(u->host, parv[2]))	/* User is not using spoofhost, assume no I:line spoof */
	{
		strlcpy(u->vhost, parv[4], HOSTLEN);
	}

	if (use_svslogin)
	{
		if (parc >= 6)
			if (strcmp(parv[5], "0"))
				handle_burstlogin(u, parv[5]);

		handle_nickchange(u);
	}
}

static void m_sethost(char *origin, uint8_t parc, char *parv[])
{
	user_t *u;

	/* SETHOST <nick> <newhost> */
	if (parc < 2)
		return;

	u = user_find(parv[0]);

	if (!u)
		return;

	strlcpy(u->vhost, parv[1], HOSTLEN);
}

static void m_setident(char *origin, uint8_t parc, char *parv[])
{
	user_t *u;

	/* SETHOST <nick> <newident> */
	if (parc < 2)
		return;

	u = user_find(parv[0]);

	if (!u)
		return;

	strlcpy(u->user, parv[1], USERLEN);
}

static void m_setname(char *origin, uint8_t parc, char *parv[])
{
	user_t *u;

	/* SETNAME <nick> <newreal> */
	if (parc < 2)
		return;

	u = user_find(parv[0]);

	if (!u)
		return;

	strlcpy(u->gecos, parv[1], GECOSLEN);
}

/*
 * Make broadcasted changes to a user's login id, nick, user, and hostname.
 *
 * parv[0] = login id
 * parv[1] = new nick
 * parv[2] = new ident
 * parv[3] = new hostname
 * parv[4] = ts
 */
static void m_signon(char *origin, uint8_t parc, char *parv[])
{
	user_t *u;
	char *nick_parv[2];

	if (!origin || parc < 5)
		return;
	u = user_find(origin);
	if (!u)
	{
		slog(LG_DEBUG, "m_signon(): signon from nonexistant user: %s", origin);
		return;
	}
	slog(LG_DEBUG, "m_signon(): signon %s -> %s!%s@%s (login %s)", origin, parv[1], parv[2], parv[3], parv[0]);

	strlcpy(u->user, parv[2], USERLEN);
	strlcpy(u->vhost, parv[3], HOSTLEN);
	nick_parv[0] = parv[1];
	nick_parv[1] = parv[4];
	if (strcmp(origin, parv[1]))
		m_nick(origin, 2, nick_parv);
	/* don't use login id, assume everyone signs in via atheme */
}

static void m_motd(char *origin, uint8_t parc, char *parv[])
{
	handle_motd(user_find(origin));
}

void _modinit(module_t * m)
{
	/* Symbol relocation voodoo. */
	server_login = &hyperion_server_login;
	introduce_nick = &hyperion_introduce_nick;
	quit_sts = &hyperion_quit_sts;
	wallops = &hyperion_wallops;
	join_sts = &hyperion_join_sts;
	kick = &hyperion_kick;
	msg = &hyperion_msg;
	notice_sts = &hyperion_notice;
	wallchops = &hyperion_wallchops;
	numeric_sts = &hyperion_numeric_sts;
	skill = &hyperion_skill;
	part = &hyperion_part;
	kline_sts = &hyperion_kline_sts;
	unkline_sts = &hyperion_unkline_sts;
	topic_sts = &hyperion_topic_sts;
	mode_sts = &hyperion_mode_sts;
	ping_sts = &hyperion_ping_sts;
	ircd_on_login = &hyperion_on_login;
	ircd_on_logout = &hyperion_on_logout;
	jupe = &hyperion_jupe;
	sethost_sts = &hyperion_sethost_sts;
	fnc_sts = &hyperion_fnc_sts;
	invite_sts = &hyperion_invite_sts;

	mode_list = hyperion_mode_list;
	ignore_mode_list = hyperion_ignore_mode_list;
	status_mode_list = hyperion_status_mode_list;
	prefix_mode_list = hyperion_prefix_mode_list;

	ircd = &Hyperion;

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
	pcommand_add("REMOVE", m_kick);	/* same net result */
	pcommand_add("KILL", m_kill);
	pcommand_add("COLLIDE", m_kill); /* same net result */
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
	pcommand_add("STOPIC", m_stopic);
	pcommand_add("SNICK", m_snick);
	pcommand_add("SETHOST", m_sethost);
	pcommand_add("SETIDENT", m_setident);
	pcommand_add("SETNAME", m_setname);
	pcommand_add("SIGNON", m_signon);
	pcommand_add("CAPAB", m_capab);
	pcommand_add("MOTD", m_motd);

	m->mflags = MODTYPE_CORE;

	pmodule_loaded = TRUE;
}
