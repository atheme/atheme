/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains protocol support for IRCnet ircd's.
 * Derived mainly from the documentation (or lack thereof)
 * in my protocol bridge.
 *
 * $Id: undernet.c 4157 2005-12-18 00:46:59Z jilles $
 */

#include "atheme.h"
#include "protocol/undernet.h"

DECLARE_MODULE_V1("protocol/undernet", TRUE, _modinit, NULL, "$Id: undernet.c 4157 2005-12-18 00:46:59Z jilles $", "Atheme Development Group <http://www.atheme.org>");

/* *INDENT-OFF* */

ircd_t Undernet = {
        "ircu 2.10.11.07 or later",     /* IRCd name */
        "$$",                           /* TLD Prefix, used by Global. */
        TRUE,                           /* Whether or not we use IRCNet/TS6 UID */
        FALSE,                          /* Whether or not we use RCOMMAND */
        FALSE,                          /* Whether or not we support channel owners. */
        FALSE,                          /* Whether or not we support channel protection. */
        FALSE,                          /* Whether or not we support halfops. */
	TRUE,				/* Whether or not we use P10 */
	FALSE,				/* Whether or not we use vHosts. */
	0,				/* Oper-only cmodes */
        0,                              /* Integer flag for owner channel flag. */
        0,                              /* Integer flag for protect channel flag. */
        0,                              /* Integer flag for halfops. */
        "+",                            /* Mode we set for owner. */
        "+",                            /* Mode we set for protect. */
        "+",                            /* Mode we set for halfops. */
	PROTOCOL_UNDERNET		/* Protocol type */
};

struct cmode_ undernet_mode_list[] = {
  { 'i', CMODE_INVITE },
  { 'm', CMODE_MOD    },
  { 'n', CMODE_NOEXT  },
  { 'p', CMODE_PRIV   },
  { 's', CMODE_SEC    },
  { 't', CMODE_TOPIC  },
  { '\0', 0 }
};

struct cmode_ undernet_ignore_mode_list[] = {
  { 'e', CMODE_EXEMPT },
  { 'I', CMODE_INVEX  },
  { '\0', 0 }
};

struct cmode_ undernet_status_mode_list[] = {
  { 'o', CMODE_OP    },
  { 'v', CMODE_VOICE },
  { '\0', 0 }
};

struct cmode_ undernet_prefix_mode_list[] = {
  { '@', CMODE_OP    },
  { '+', CMODE_VOICE },
  { '\0', 0 }
};

/* *INDENT-ON* */

/* login to our uplink */
static uint8_t undernet_server_login(void)
{
	int8_t ret;

	ret = sts("PASS :%s", curr_uplink->pass);
	if (ret == 1)
		return 1;

	me.bursting = TRUE;

	/* SERVER irc.undernet.org 1 933022556 947908144 J10 AA]]] :[127.0.0.1] A Undernet Server */
	sts("SERVER %s 1 %ld %ld J10 %s]]] :%s", me.name, me.start, CURRTIME, me.numeric, me.desc);

	services_init();

	sts("%s EB", me.numeric);

	return 0;
}

/* introduce a client */
static void undernet_introduce_nick(char *nick, char *user, char *host, char *real, char *uid)
{
	sts("%s N %s 1 %ld %s %s +%s%sk A %s :%s", me.numeric, nick, CURRTIME, user, host, "io", chansvs.fantasy ? "" : "d", uid, real);
}

/* invite a user to a channel */
static void undernet_invite_sts(user_t *sender, user_t *target, channel_t *channel)
{
	sts("%s I %s %s", sender->uid, target->uid, channel->name);
}

static void undernet_quit_sts(user_t *u, char *reason)
{
	if (!me.connected)
		return;

	sts("%s Q :%s", u->uid, reason);
}

/* WALLOPS wrapper */
static void undernet_wallops(char *fmt, ...)
{
	va_list ap;
	char buf[BUFSIZE];

	if (config_options.silent)
		return;

	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZE, fmt, ap);
	va_end(ap);

	sts("%s WA :%s", chansvs.me->me->uid, buf);
}

/* join a channel */
static void undernet_join_sts(channel_t *c, user_t *u, boolean_t isnew, char *modes)
{
	/* If the channel doesn't exist, we need to create it. */
	if (isnew)
	{
		sts("%s C %s %ld", u->uid, c->name, c->ts);
		if (modes[0] && modes[1])
			sts("%s M %s %s %ld", u->uid, c->name, modes, c->ts);
	}
	else
	{
		sts("%s J %s %ld", u->uid, c->name, c->ts);
		sts("%s M %s +o %s %ld", me.numeric, c->name, u->uid, c->ts);
	}
}

/* kicks a user from a channel */
static void undernet_kick(char *from, char *channel, char *to, char *reason)
{
	channel_t *chan = channel_find(channel);
	user_t *fptr = user_find(from);
	user_t *user = user_find(to);

	if (!chan || !user || !fptr)
		return;

	sts("%s K %s %s :%s", fptr->uid, channel, user->uid, reason);

	chanuser_delete(chan, user);
}

/* PRIVMSG wrapper */
static void undernet_msg(char *from, char *target, char *fmt, ...)
{
	va_list ap;
	user_t *u = user_find(from);
	char buf[BUFSIZE];

	if (!u)
		return;

	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZE, fmt, ap);
	va_end(ap);

	sts("%s P %s :%s", u->uid, target, buf);
}

/* NOTICE wrapper */
static void undernet_notice(char *from, char *target, char *fmt, ...)
{
	va_list ap;
	char buf[BUFSIZE];
	user_t *u = user_find(from);
	user_t *t = user_find(target);

	if (u == NULL && (from == NULL || (irccasecmp(from, me.name) && irccasecmp(from, ME))))
	{
		slog(LG_DEBUG, "undernet_notice(): unknown source %s for notice to %s", from, target);
		return;
	}

	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZE, fmt, ap);
	va_end(ap);

	sts("%s O %s :%s", u ? u->uid : me.numeric, t ? t->uid : target, buf);
}

static void undernet_wallchops(user_t *sender, channel_t *channel, char *message)
{
	sts("%s WC %s :%s", sender->uid, channel->name, message);
}

/* numeric wrapper */
static void undernet_numeric_sts(char *from, int numeric, char *target, char *fmt, ...)
{
	va_list ap;
	char buf[BUFSIZE];
	user_t *source_p, *target_p;

	source_p = user_find(from);
	target_p = user_find(target);

	if (!target_p)
		return;

	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZE, fmt, ap);
	va_end(ap);

	sts("%s %d %s %s", source_p ? source_p->uid : me.numeric, numeric, target_p->uid, buf);
}

/* KILL wrapper */
static void undernet_skill(char *from, char *nick, char *fmt, ...)
{
	va_list ap;
	char buf[BUFSIZE];
	user_t *fptr = user_find(from);
	user_t *tptr = user_find(nick);

	if (!fptr || !tptr)
		return;

	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZE, fmt, ap);
	va_end(ap);

	sts("%s D %s :%s!%s!%s (%s)", fptr->uid, tptr->uid, from, from, from, buf);
}

/* PART wrapper */
static void undernet_part(char *chan, char *nick)
{
	user_t *u = user_find(nick);
	channel_t *c = channel_find(chan);
	chanuser_t *cu;

	if (!u || !c)
		return;

	if (!(cu = chanuser_find(c, u)))
		return;

	sts("%s L %s", u->uid, c->name);

	chanuser_delete(c, u);
}

/* server-to-server KLINE wrapper */
static void undernet_kline_sts(char *server, char *user, char *host, long duration, char *reason)
{
	if (!me.connected)
		return;

	sts("%s GL * +%s@%s %ld :%s", me.numeric, user, host, duration, reason);
}

/* server-to-server UNKLINE wrapper */
static void undernet_unkline_sts(char *server, char *user, char *host)
{
	if (!me.connected)
		return;

	sts("%s GL * -%s@%s", me.numeric, user, host);
}

/* topic wrapper */
static void undernet_topic_sts(char *channel, char *setter, time_t ts, char *topic)
{
	/* ircu does not support remote topic propagation */
}

/* mode wrapper */
static void undernet_mode_sts(char *sender, char *target, char *modes)
{
	user_t *fptr = user_find(sender);
	channel_t *cptr = channel_find(target);

	if (!fptr || !cptr)
		return;

	sts("%s M %s %s %ld", fptr->uid, target, modes, cptr->ts);
}

/* ping wrapper */
static void undernet_ping_sts(void)
{
	if (!me.connected)
		return;

	sts("%s G !%ld %s %ld", me.numeric, CURRTIME, me.name, CURRTIME);
}

/* protocol-specific stuff to do on login */
static void undernet_on_login(char *origin, char *user, char *wantedhost)
{
	user_t *u = user_find(origin);

	if (!u)
		return;

	sts("%s AC %s %s", me.numeric, u->uid, u->myuser->name);
}

/* protocol-specific stuff to do on login */
static void undernet_on_logout(char *origin, char *user, char *wantedhost)
{
	/* nothing to do on ratbox */
	return;
}

static void undernet_jupe(char *server, char *reason)
{
	if (!me.connected)
		return;

	sts("%s JU * !+%s %ld :%s", me.numeric, server, CURRTIME, reason);
}

static void m_topic(char *origin, uint8_t parc, char *parv[])
{
	channel_t *c = channel_find(parv[0]);
	user_t *u = user_find(origin);

	if (!c || !u)
		return;

	handle_topic(c, u->nick, CURRTIME, parv[1]);
}

/* AB G !1119920789.573932 services.atheme.org 1119920789.573932 */
static void m_ping(char *origin, uint8_t parc, char *parv[])
{
	/* reply to PING's */
	sts("%s Z %s %s %s", me.numeric, parv[0], parv[1], parv[2]);
}

static void m_pong(char *origin, uint8_t parc, char *parv[])
{
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

static void m_create(char *origin, uint8_t parc, char *parv[])
{
	char buf[BUFSIZE];
	uint8_t chanc;
	char *chanv[256];
	uint8_t i;

	chanc = sjtoken(parv[0], ',', chanv);

	for (i = 0; i < chanc; i++)
	{
		channel_t *c = channel_add(chanv[i], atoi(parv[1]));

		buf[0] = '@';
		buf[1] = '\0';

		strlcat(buf, origin, BUFSIZE);

		chanuser_add(c, buf);
	}
}

static void m_join(char *origin, uint8_t parc, char *parv[])
{
	char buf[BUFSIZE];
	uint8_t chanc;
	char *chanv[256];
	uint8_t i;

	chanc = sjtoken(parv[0], ',', chanv);

	for (i = 0; i < chanc; i++)
	{
		channel_t *c = channel_find(chanv[i]);

		if (!c)
			c = channel_add(chanv[i], atoi(parv[1]));

		buf[0] = '@';
		buf[1] = '\0';

		strlcat(buf, origin, BUFSIZE);

		chanuser_add(c, buf);
	}
}

static void m_burst(char *origin, uint8_t parc, char *parv[])
{
	channel_t *c;
	uint8_t modec = 0;
	char *modev[16];
	uint8_t userc;
	char *userv[256];
	uint8_t i;

	if (parc >= 3)
	{
		c = channel_find(parv[0]);

		if (parc >= 4)
			modev[modec++] = parv[2];
		if (parc >= 5)
			modev[modec++] = parv[3];
		if (parc >= 6)
			modev[modec++] = parv[4];

		if (!c)
		{
			slog(LG_DEBUG, "m_burst(): new channel: %s", parv[0]);
			c = channel_add(parv[0], atoi(parv[1]));
		}

		if (parc >= 4)
			channel_mode(NULL, c, modec, modev);

		/* handle bans. */
		if (parv[parc - 1][0] == '%')
			userc = sjtoken(parv[parc - 2], ',', userv);
		else
			userc = sjtoken(parv[parc - 1], ',', userv);

		for (i = 0; i < userc; i++)
		{
			slog(LG_DEBUG, "m_burst(): parsing %s", userv[i]);

			if (strchr(userv[i], ':'))
			{
				char buf[BUFSIZE];
				char *nick = strtok(userv[i], ":");
				char *status = strtok(NULL, ":");
				char *bptr = buf;

				memset(buf, 0, sizeof(buf));

				while (*status)
				{
					if (*status == 'o')
						*bptr = '@';
					else if (*status == 'v')
						*bptr = '+';
					status++;
					bptr++;
				}

				strlcat(buf, nick, BUFSIZE);

				slog(LG_DEBUG, "m_burst(): converted %s into %s.", userv[i], buf);

				chanuser_add(c, buf);

				continue;
			}

			chanuser_add(c, userv[i]);
		}
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
	if (parc == 10)
	{
		s = server_find(origin);
		if (!s)
		{
			slog(LG_DEBUG, "m_nick(): new user on nonexistant server: %s", origin);
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
			kline_sts(origin, k->user, k->host, (k->expires - CURRTIME), k->reason);

			return;
		}

		u = user_add(parv[0], parv[3], parv[4], NULL, NULL, parv[8], parv[9], s, atoi(parv[2]));

		user_mode(u, parv[5]);

		handle_nickchange(u);
	}
	else if (parc == 9)
	{
		s = server_find(origin);
		if (!s)
		{
			slog(LG_DEBUG, "m_nick(): new user on nonexistant server: %s", origin);
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
			kline_sts(origin, k->user, k->host, (k->expires - CURRTIME), k->reason);

			return;
		}

		u = user_add(parv[0], parv[3], parv[4], NULL, NULL, parv[7], parv[8], s, atoi(parv[2]));

		user_mode(u, parv[5]);

		handle_nickchange(u);
	}
	else if (parc == 8)
	{
		s = server_find(origin);
		if (!s)
		{
			slog(LG_DEBUG, "m_nick(): new user on nonexistant server: %s", origin);
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
			kline_sts(origin, k->user, k->host, (k->expires - CURRTIME), k->reason);

			return;
		}

		u = user_add(parv[0], parv[3], parv[4], NULL, NULL, parv[6], parv[7], s, atoi(parv[2]));

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
		slog(LG_DEBUG, "m_nick(): got NICK with wrong (%d) number of params", parc);

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

/* SERVER ircu.devel.atheme.org 1 1119902586 1119908830 J10 ABAP] + :lets lol */
static void m_server(char *origin, uint8_t parc, char *parv[])
{
	/* We dont care about the max connections. */
	parv[5][2] = '\0';

	slog(LG_DEBUG, "m_server(): new server: %s, id %s", parv[0], parv[5]);
	server_add(parv[0], atoi(parv[1]), origin ? origin : me.name, parv[5], parv[7]);

	if (cnt.server == 2)
		me.actual = sstrdup(parv[0]);

	me.recvsvr = TRUE;
}

static void m_stats(char *origin, uint8_t parc, char *parv[])
{
	handle_stats(origin, parv[0][0]);
}

static void m_admin(char *origin, uint8_t parc, char *parv[])
{
	user_t *u = user_find(origin);

	if (!u)
		return;

	handle_admin(u->nick);
}

static void m_version(char *origin, uint8_t parc, char *parv[])
{
	user_t *u = user_find(origin);

	if (!u)
		return;

	handle_version(u->nick);
}

static void m_info(char *origin, uint8_t parc, char *parv[])
{
	user_t *u = user_find(origin);

	if (!u)
		return;

	handle_info(u->nick);
}

static void m_whois(char *origin, uint8_t parc, char *parv[])
{
	handle_whois(origin, parc >= 2 ? parv[1] : "*");
}

static void m_trace(char *origin, uint8_t parc, char *parv[])
{
	handle_trace(origin, parc >= 1 ? parv[0] : "*", parc >= 2 ? parv[1] : NULL);
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

static void m_eos(char *origin, uint8_t parc, char *parv[])
{
	sts("%s EA", me.numeric);
}

void _modinit(module_t * m)
{
	/* Symbol relocation voodoo. */
	server_login = &undernet_server_login;
	introduce_nick = &undernet_introduce_nick;
	quit_sts = &undernet_quit_sts;
	wallops = &undernet_wallops;
	join_sts = &undernet_join_sts;
	kick = &undernet_kick;
	msg = &undernet_msg;
	notice = &undernet_notice;
	wallchops = &undernet_wallchops;
	numeric_sts = &undernet_numeric_sts;
	skill = &undernet_skill;
	part = &undernet_part;
	kline_sts = &undernet_kline_sts;
	unkline_sts = &undernet_unkline_sts;
	topic_sts = &undernet_topic_sts;
	mode_sts = &undernet_mode_sts;
	ping_sts = &undernet_ping_sts;
	ircd_on_login = &undernet_on_login;
	ircd_on_logout = &undernet_on_logout;
	jupe = &undernet_jupe;
	invite_sts = &undernet_invite_sts;

	parse = &p10_parse;

	mode_list = undernet_mode_list;
	ignore_mode_list = undernet_ignore_mode_list;
	status_mode_list = undernet_status_mode_list;
	prefix_mode_list = undernet_prefix_mode_list;

	ircd = &Undernet;

	pcommand_add("G", m_ping);
	pcommand_add("Z", m_pong);
	pcommand_add("P", m_privmsg);
	pcommand_add("O", m_notice);
	pcommand_add("C", m_create);
	pcommand_add("J", m_join);
	pcommand_add("EB", m_eos);
	pcommand_add("B", m_burst);
	pcommand_add("L", m_part);
	pcommand_add("N", m_nick);
	pcommand_add("Q", m_quit);
	pcommand_add("M", m_mode);
	pcommand_add("K", m_kick);
	pcommand_add("D", m_kill);
	pcommand_add("SQ", m_squit);
	pcommand_add("S", m_server);
	pcommand_add("SERVER", m_server);
	pcommand_add("R", m_stats);
	pcommand_add("AD", m_admin);
	pcommand_add("V", m_version);
	pcommand_add("F", m_info);
	pcommand_add("W", m_whois);
	pcommand_add("TR", m_trace);
	pcommand_add("PASS", m_pass);
	pcommand_add("ERROR", m_error);
	pcommand_add("T", m_topic);

	m->mflags = MODTYPE_CORE;

	pmodule_loaded = TRUE;
}
