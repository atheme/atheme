/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains protocol support for spanning-tree inspircd, b6 or later.
 *
 * $Id: inspircd.c 4109 2005-12-17 02:43:15Z w00t $
 */

#include "atheme.h"
#include "protocol/inspircd.h"

DECLARE_MODULE_V1("protocol/inspircd", TRUE, _modinit, NULL, "$Id: inspircd.c 4109 2005-12-17 02:43:15Z w00t $", "InspIRCd Core Team <http://www.inspircd.org/>");

/* *INDENT-OFF* */

ircd_t InspIRCd = {
        "InspIRCd 1.0 Beta 6 or later", /* IRCd name */
        "$",                            /* TLD Prefix, used by Global. */
        FALSE,                          /* Whether or not we use IRCNet/TS6 UID */
        FALSE,                          /* Whether or not we use RCOMMAND */
        TRUE,                           /* Whether or not we support channel owners. */
        TRUE,                           /* Whether or not we support channel protection. */
        TRUE,                           /* Whether or not we support halfops. */
	FALSE,				/* Whether or not we use P10 */
	TRUE,				/* Whether or not we use vHosts. */
	CMODE_OPERONLY,			/* Oper-only cmodes */
        CMODE_OWNER,                    /* Integer flag for owner channel flag. */
        CMODE_PROTECT,                  /* Integer flag for protect channel flag. */
        CMODE_HALFOP,                   /* Integer flag for halfops. */
        "+q",                           /* Mode we set for owner. */
        "+a",                           /* Mode we set for protect. */
        "+h",                           /* Mode we set for halfops. */
	PROTOCOL_INSPIRCD		/* Protocol type */
};

struct cmode_ inspircd_mode_list[] = {
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
  { 'S', CMODE_STRIP    },
  { 'K', CMODE_NOKNOCK  },
  { 'V', CMODE_NOINVITE },
  { 'C', CMODE_NOCTCP   },
  { 'N', CMODE_STICKY   },
  { '\0', 0 }
};

struct cmode_ inspircd_ignore_mode_list[] = {
  { 'e', CMODE_EXEMPT },
  { 'I', CMODE_INVEX  },
  { 'g', CMODE_CENSOR },
  { '\0', 0 }
};

struct cmode_ inspircd_status_mode_list[] = {
  { 'q', CMODE_OWNER   },
  { 'a', CMODE_PROTECT },
  { 'o', CMODE_OP      },
  { 'h', CMODE_HALFOP  },
  { 'v', CMODE_VOICE   },
  { '\0', 0 }
};

struct cmode_ inspircd_prefix_mode_list[] = {
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
static uint8_t inspircd_server_login(void)
{
	int8_t ret;

	ret = sts("PASS %s", curr_uplink->pass);
	if (ret == 1)
		return 1;

	me.bursting = TRUE;

	sts("SERVER %s %s 0 :%s", me.name, curr_uplink->pass, me.desc);
	sts("BURST");
	/* XXX: Being able to get this data as a char* would be nice - Brain */
        sts(":%s VERSION :atheme-%s. %s %s%s%s%s%s%s%s%s%s",me.name, version, me.name, (match_mapping) ? "A" : "",
								                      (me.loglevel & LG_DEBUG) ? "d" : "",
							                              (me.auth) ? "e" : "",
										      (config_options.flood_msgs) ? "F" : "",
										      (config_options.leave_chans) ? "l" : "",
										      (config_options.join_chans) ? "j" : "",
										      (!match_mapping) ? "R" : "",
										      (config_options.raw) ? "r" : "",
										      (runflags & RF_LIVE) ? "n" : "");
	services_init();
	return 0;
}

/* introduce a client */
static void inspircd_introduce_nick(char *nick, char *user, char *host, char *real, char *uid)
{
	/* :services-dev.chatspike.net NICK 1133994664 OperServ chatspike.net chatspike.net services +oii 0.0.0.0 :Operator Server  */
	sts(":%s NICK %ld %s %s %s %s +%s 0.0.0.0 :%s", me.name, CURRTIME, nick, host, host, user, "io", real);
	sts(":%s OPERTYPE Services",nick);
}

static void inspircd_quit_sts(user_t *u, char *reason)
{
	if (!me.connected)
		return;

	sts(":%s QUIT :%s", u->nick, reason);
}

/* WALLOPS wrapper */
static void inspircd_wallops(char *fmt, ...)
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
static void inspircd_join_sts(channel_t *c, user_t *u, boolean_t isnew, char *modes)
{
	if (isnew)
	{
		sts(":%s FJOIN %s %ld @%s", me.name, c->name, c->ts, u->nick);
		/* This FMODE is only for compatibility with old inspircd */
		sts(":%s FMODE %s +o %s", me.name, c->name, u->nick);
		if (modes[0] && modes[1])
			sts(":%s MODE %s %s", me.name, c->name, modes);
	}
	else
	{
		sts(":%s JOIN %s", u->nick, c->name);
		sts(":%s FMODE %s +o %s", me.name, c->name, u->nick);
	}
}

/* kicks a user from a channel */
static void inspircd_kick(char *from, char *channel, char *to, char *reason)
{
	channel_t *chan = channel_find(channel);
	user_t *user = user_find(to);

	if (!chan || !user)
		return;

	sts(":%s KICK %s %s :%s", from, channel, to, reason);

	chanuser_delete(chan, user);
}

/* PRIVMSG wrapper */
static void inspircd_msg(char *from, char *target, char *fmt, ...)
{
	va_list ap;
	char buf[BUFSIZE];

	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZE, fmt, ap);
	va_end(ap);

	sts(":%s PRIVMSG %s :%s", from, target, buf);
}

/* NOTICE wrapper */
static void inspircd_notice(char *from, char *target, char *fmt, ...)
{
	va_list ap;
	char buf[BUFSIZE];

	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZE, fmt, ap);
	va_end(ap);

	sts(":%s NOTICE %s :%s", from, target, buf);
}

static void inspircd_numeric_sts(char *from, int numeric, char *target, char *fmt, ...)
{
	va_list ap;
	char buf[BUFSIZE];

	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZE, fmt, ap);
	va_end(ap);

	/* InspIRCd doesnt pass numerics around */
}

/* KILL wrapper */
static void inspircd_skill(char *from, char *nick, char *fmt, ...)
{
	va_list ap;
	char buf[BUFSIZE];

	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZE, fmt, ap);
	va_end(ap);

	sts(":%s KILL %s :[%s] Killed (%s (%s))", from, nick, me.name, from, buf);
}

/* PART wrapper */
static void inspircd_part(char *chan, char *nick)
{
	user_t *u = user_find(nick);
	channel_t *c = channel_find(chan);
	chanuser_t *cu;

	if (!u || !c)
		return;

	if (!(cu = chanuser_find(c, u)))
		return;

	sts(":%s PART %s :Leaving", u->nick, c->name);

	chanuser_delete(c, u);
}

/* server-to-server KLINE wrapper */
static void inspircd_kline_sts(char *server, char *user, char *host, long duration, char *reason)
{
	if (!me.connected)
		return;

	/* :services-dev.chatspike.net ADDLINE G test@test.com Brain 1133994664 0 :You are banned from this network */
	sts(":%s ADDLINE G %s@%s %s %ld 0 :%s", me.name, user, host, opersvs.nick, time(NULL), reason);
}

/* server-to-server UNKLINE wrapper */
static void inspircd_unkline_sts(char *server, char *user, char *host)
{
	if (!me.connected)
		return;

	/* I know this looks wrong, but it's really not. Trust me. --w00t */
	sts(":%s GLINE %s@%s",opersvs.nick, user, host);
}

/* topic wrapper */
static void inspircd_topic_sts(char *channel, char *setter, time_t ts, char *topic)
{
	if (!me.connected)
		return;

	sts(":%s FTOPIC %s %ld %s :%s", me.name, channel, ts, setter, topic);
}

/* mode wrapper */
static void inspircd_mode_sts(char *sender, char *target, char *modes)
{
	if (!me.connected)
		return;

	sts(":%s MODE %s %s", sender, target, modes);
}

/* ping wrapper */
static void inspircd_ping_sts(void)
{
	if (!me.connected)
		return;

	sts(":%s PING :%s", me.name, me.name);
}

/* protocol-specific stuff to do on login */
static void inspircd_on_login(char *origin, char *user, char *wantedhost)
{
	if (!me.connected)
		return;

	/* Can only do this for nickserv, and can only record identified
	 * state if logged in to correct nick, sorry -- jilles
	 */
	if (nicksvs.me == NULL || irccasecmp(origin, user))
		return;

	/* In InspIRCd, SVSMODE shows the +r if not already set */
	sts(":%s SVSMODE %s +r", nicksvs.nick, origin);
}

/* protocol-specific stuff to do on logout */
static void inspircd_on_logout(char *origin, char *user, char *wantedhost)
{
	if (!me.connected)
		return;

	if (nicksvs.me == NULL || irccasecmp(origin, user))
		return;

	sts(":%s SVSMODE %s -r", nicksvs.nick, origin);
}

static void inspircd_jupe(char *server, char *reason)
{
	if (!me.connected)
		return;

	sts(":%s SERVER %s * 1 :%s", me.name, server, reason);
}

static void inspircd_sethost_sts(char *source, char *target, char *host)
{
	if (!me.connected)
		return;

	sts(":%s CHGHOST %s %s", source, target, host);
}

static void m_topic(char *origin, uint8_t parc, char *parv[])
{
	channel_t *c = channel_find(parv[0]);

	if (!c)
		return;

	handle_topic(c, origin, time(NULL), parv[1]);
}

static void m_ftopic(char *origin, uint8_t parc, char *parv[])
{
	channel_t *c = channel_find(parv[0]);

	if (!c)
		return;

	handle_topic(c, parv[2], atol(parv[1]), parv[3]);
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

static void m_fjoin(char *origin, uint8_t parc, char *parv[])
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
	char nicklist[512];

	if (parc >= 3)
	{
		c = channel_find(parv[0]);
		ts = atol(parv[1]);

		if (!c)
		{
			slog(LG_DEBUG, "m_fjoin(): new channel: %s", parv[0]);
			c = channel_add(parv[0], ts);
		}

		if (ts < c->ts)
		{
			/* the TS changed.  a TS change requires us to do
			 * bugger all except update the TS, because in InspIRCd,
			 * remote servers enforce the TS change (this means that
			 * rogue servers cant really get around it) - Brain
			 */
			c->ts = ts;
		}

		/* XXX: InspIRCd has no leading colon on the nick list,
		 * so we must build one. This is a mess, someone pleaaaase
		 * tidy it up for me - Brain
		 */
		*nicklist = '\0';
		for (i = 2; i < parc; i++)
		{
			if (i != 2)
				strncat(nicklist," ",512);
			strncat(nicklist,parv[i],512);
		}

		userc = sjtoken(nicklist, ' ', userv);

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

	/* :services-dev.chatspike.net NICK 1133994664 DevNull chatspike.net chatspike.net services +i 0.0.0.0 :/dev/null -- message sink */
	if (parc == 8)
	{
		s = server_find(origin);
		if (!s)
		{
			slog(LG_DEBUG, "m_nick(): new user on nonexistant server: %s", origin);
			return;
		}

		slog(LG_DEBUG, "m_nick(): new user on `%s': %s", s->name, parv[1]);

		if ((k = kline_find(parv[4], parv[2])))
		{
			/* the new user matches a kline.
			 * the server introducing the user probably wasn't around when
			 * we added the kline or isn't accepting klines from us.
			 * either way, we'll KILL the user and send the server
			 * a new KLINE.
			 */

			skill(opersvs.nick, parv[1], k->reason);
			kline_sts(origin, k->user, k->host, (k->expires - CURRTIME), k->reason);

			return;
		}

		/* char *nick, char *user, char *host, char *vhost, char *ip, char *uid, char *gecos, server_t *server, uint32_t ts */
		u = user_add(parv[1], parv[4], parv[2], parv[3], parv[6], NULL, parv[7], s, atol(parv[0]));

		user_mode(u, parv[5]);

		if (strchr(parv[7], 'r'))
			handle_burstlogin(u, parv[0]);

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

		/* fix up +r if necessary -- jilles */
		if (nicksvs.me != NULL && u->myuser != NULL && irccasecmp(u->nick, parv[0]) && !irccasecmp(parv[0], u->myuser->name))
			/* changed nick to registered one, reset +r */
			sts(":%s SVSMODE %s +r", nicksvs.nick, parv[0]);

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
	slog(LG_DEBUG, "m_squit(): server leaving: %s from %s", parv[0], origin);
	server_delete(parv[0]);
}

static void m_server(char *origin, uint8_t parc, char *parv[])
{
	slog(LG_DEBUG, "m_server(): new server: %s", parv[0]);
	server_add(parv[0], atoi(parv[2]), origin ? origin : me.name, NULL, parv[3]);

	if (cnt.server == 2)
		me.actual = sstrdup(parv[0]);
}

static void m_join(char *origin, uint8_t parc, char *parv[])
{
	user_t *u = user_find(origin);
	channel_t *c;
	chanuser_t *cu;
	node_t *n, *tn;
	char* modev[2];

	if (!u)
		return;

	modev[0] = "+o";
	modev[1] = origin;
	c = channel_find(parv[0]);
	if (!c)
	{
		slog(LG_DEBUG, "m_join(): new channel: %s", parv[0]);
		c = channel_add(parv[0], CURRTIME);
		chanuser_add(c, origin);
		channel_mode(NULL, c, 2, modev);
	}
	else
	{
		chanuser_add(c, origin);
	}

		
}

static void m_sajoin(char *origin, uint8_t parc, char *parv[])
{
	m_join(parv[0], 1, &parv[1]);
}

static void m_sapart(char *origin, uint8_t parc, char *parv[])
{
	m_part(parv[0], 1, &parv[1]);
}

static void m_sanick(char *origin, uint8_t parc, char *parv[])
{
	m_nick(parv[0], 1, &parv[1]);
}

static void m_samode(char *origin, uint8_t parc, char *parv[])
{
	m_mode(me.name, parc - 1, &parv[1]);
}

static void m_error(char *origin, uint8_t parc, char *parv[])
{
	slog(LG_INFO, "m_error(): error from server: %s", parv[0]);
}

static void m_idle(char* origin, uint8_t parc, char *parv[])
{
	if (parc == 1)
	{
		sts(":%s IDLE %s %ld 0",parv[0],origin,CURRTIME);
	}
	else
	{
		slog(LG_INFO, "m_idle(): Received an IDLE response but we didn't WHOIS anybody!");
	}
}

static void m_fhost(char *origin, uint8_t parc, char *parv[])
{
	user_t *u = user_find(origin);
	if (!u)
		return;
	strlcpy(u->vhost, parv[0], HOSTLEN);
}

void _modinit(module_t * m)
{
	/* Symbol relocation voodoo. */
	server_login = &inspircd_server_login;
	introduce_nick = &inspircd_introduce_nick;
	quit_sts = &inspircd_quit_sts;
	wallops = &inspircd_wallops;
	join_sts = &inspircd_join_sts;
	kick = &inspircd_kick;
	msg = &inspircd_msg;
	notice = &inspircd_notice;
	numeric_sts = &inspircd_numeric_sts;
	skill = &inspircd_skill;
	part = &inspircd_part;
	kline_sts = &inspircd_kline_sts;
	unkline_sts = &inspircd_unkline_sts;
	topic_sts = &inspircd_topic_sts;
	mode_sts = &inspircd_mode_sts;
	ping_sts = &inspircd_ping_sts;
	ircd_on_login = &inspircd_on_login;
	ircd_on_logout = &inspircd_on_logout;
	jupe = &inspircd_jupe;
	sethost_sts = &inspircd_sethost_sts;

	mode_list = inspircd_mode_list;
	ignore_mode_list = inspircd_ignore_mode_list;
	status_mode_list = inspircd_status_mode_list;
	prefix_mode_list = inspircd_prefix_mode_list;

	ircd = &InspIRCd;

	pcommand_add("PING", m_ping);
	pcommand_add("PONG", m_pong);
	pcommand_add("PRIVMSG", m_privmsg);
	pcommand_add("NOTICE", m_notice);
	pcommand_add("FJOIN", m_fjoin);
	pcommand_add("PART", m_part);
	pcommand_add("NICK", m_nick);
	pcommand_add("QUIT", m_quit);
	pcommand_add("MODE", m_mode);
	pcommand_add("FMODE", m_mode);
	pcommand_add("SAMODE", m_samode);
	pcommand_add("SAJOIN", m_sajoin);
	pcommand_add("SAPART", m_sapart);
	pcommand_add("SANICK", m_sanick);
	pcommand_add("KICK", m_kick);
	pcommand_add("KILL", m_kill);
	pcommand_add("SQUIT", m_squit);
	pcommand_add("SERVER", m_server);
	pcommand_add("FTOPIC", m_ftopic);
	pcommand_add("JOIN", m_join);
	pcommand_add("ERROR", m_error);
	pcommand_add("TOPIC", m_topic);
	pcommand_add("FHOST", m_fhost);
	pcommand_add("IDLE", m_idle);

	m->mflags = MODTYPE_CORE;

	pmodule_loaded = TRUE;
}
