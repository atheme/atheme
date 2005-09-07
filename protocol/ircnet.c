/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains protocol support for IRCnet ircd's.
 * Derived mainly from the documentation (or lack thereof)
 * in my protocol bridge.
 *
 * $Id: ircnet.c 2185 2005-09-07 02:43:08Z nenolod $
 */

#include "atheme.h"
#include "protocol/ircnet.h"

DECLARE_MODULE_V1
(
	"protocol/ircnet", FALSE, _modinit, NULL,
	"$Id: ircnet.c 2185 2005-09-07 02:43:08Z nenolod $",
	"Atheme Development Group <http://www.atheme.org>"
);

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
        0,                              /* Integer flag for owner channel flag. */
        0,                              /* Integer flag for protect channel flag. */
        0,                              /* Integer flag for halfops. */
        "+",                            /* Mode we set for owner. */
        "+",                            /* Mode we set for protect. */
        "+"                             /* Mode we set for halfops. */
};

struct cmode_ ircnet_mode_list[] = {
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

struct cmode_ ircnet_ignore_mode_list[] = {
  { 'e', CMODE_EXEMPT },
  { 'I', CMODE_INVEX  },
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

        sts("SERVER %s 1 %s :%s", me.name, curr_uplink->numeric, me.desc);

	services_init();

        return 0;
}

/* introduce a client */
static user_t *ircnet_introduce_nick(char *nick, char *user, char *host, char *real, char *modes)
{
	user_t *u;
	char *uid = uid_get();

	sts(":%s UNICK %s %s %s %s 0.0.0.0 +%s :%s", me.numeric, nick, uid, user, host, modes, real);

	u = user_add(nick, user, host, NULL, uid, real, me.me);
	if (strchr(modes, 'o'))
		u->flags |= UF_IRCOP;

	return u;
}

static void ircnet_quit_sts(user_t *u, char *reason)
{
	if (!me.connected)
		return;

	sts(":%s QUIT :%s", u->nick, reason);

	user_delete(u->nick);
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
static void ircnet_join(char *chan, char *nick)
{
	channel_t *c = channel_find(chan);
	user_t *u = user_find(nick);
	chanuser_t *cu;

	if (!u)
		return;

	if (!c)
	{
		sts(":%s NJOIN %s :@%s", me.numeric, chan, u->uid);

		c = channel_add(chan, CURRTIME);
	}
	else
	{
		if ((cu = chanuser_find(c, user_find(nick))))
		{
			slog(LG_DEBUG, "join(): i'm already in `%s'", c->name);
			return;
		}

		sts(":%s NJOIN %s :@%s", me.numeric, chan, u->uid);

		c = channel_add(chan, CURRTIME);
	}

	cu = chanuser_add(c, nick);
	cu->modes |= CMODE_OP;
}

/* kicks a user from a channel */
static void ircnet_kick(char *from, char *channel, char *to, char *reason)
{
        channel_t *chan = channel_find(channel);
        user_t *user = user_find(to);

        if (!chan || !user)
                return;

        sts(":%s KICK %s %s :%s", from, channel, to, reason);

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

        va_start(ap, fmt);
        vsnprintf(buf, BUFSIZE, fmt, ap);
        va_end(ap);

        sts(":%s NOTICE %s :%s", from, target, buf);
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

	sts(":%s KLINE %s %ld %s %s :%s", me.name, server, duration, user, host, reason);
}

/* server-to-server UNKLINE wrapper */
static void ircnet_unkline_sts(char *server, char *user, char *host)
{
	if (!me.connected)
		return;

	sts(":%s UNKLINE %s %s %s", me.name, server, user, host);
}

/* topic wrapper */
static void ircnet_topic_sts(char *channel, char *setter, char *topic)
{
	if (!me.connected)
		return;

	sts(":%s TOPIC %s :%s (%s)", chansvs.nick, channel, topic, setter);
}

/* mode wrapper */
static void ircnet_mode_sts(char *sender, char *target, char *modes)
{
	if (!me.connected)
		return;

	/* On IRCNet ircd's, we must join the channel to set a mode. */
	if (strcasecmp(sender, chansvs.nick))
		join(target, sender);

	sts(":%s MODE %s %s", sender, target, modes);

	if (strcasecmp(sender, chansvs.nick))
		part(target, sender);
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
static void ircnet_on_logout(char *origin, char *user, char *wantedhost)
{
	/* nothing to do on ratbox */
	return;
}

static void ircnet_jupe(char *server, char *reason)
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

static void m_privmsg(char *origin, uint8_t parc, char *parv[])
{
	user_t *u;
	service_t *sptr;

	/* we should have no more and no less */
	if (parc != 2 || !origin)
		return;

	if (!(u = user_find(origin)))
	{
		slog(LG_DEBUG, "m_privmsg(): got message from nonexistant user `%s'", origin);
		return;
	}

	/* run it through flood checks */
	if ((config_options.flood_msgs) && (!is_sra(u->myuser)) && (!is_ircop(u)))
	{
		/* check if they're being ignored */
		if (u->offenses > 10)
		{
			if ((CURRTIME - u->lastmsg) > 30)
			{
				u->offenses -= 10;
				u->lastmsg = CURRTIME;
				u->msgs = 0;
			}
			else
				return;
		}

		if ((CURRTIME - u->lastmsg) > config_options.flood_time)
		{
			u->lastmsg = CURRTIME;
			u->msgs = 0;
		}

		/* fix for 15: ignore channel messages. */
		if (*parv[0] != '#')
			u->msgs++;

		if (u->msgs > config_options.flood_msgs)
		{
			/* they're flooding. */
			if (!u->offenses)
			{
				/* ignore them the first time */
				u->lastmsg = CURRTIME;
				u->msgs = 0;
				u->offenses = 11;

				notice(parv[0], origin, "You have triggered services flood protection.");
				notice(parv[0], origin, "This is your first offense. You will be ignored for " "30 seconds.");

				snoop("FLOOD: \2%s\2", u->nick);

				return;
			}

			if (u->offenses == 1)
			{
				/* ignore them the second time */
				u->lastmsg = CURRTIME;
				u->msgs = 0;
				u->offenses = 12;

				notice(parv[0], origin, "You have triggered services flood protection.");
				notice(parv[0], origin, "This is your last warning. You will be ignored for " "30 seconds.");

				snoop("FLOOD: \2%s\2", u->nick);

				return;
			}

			if (u->offenses == 2)
			{
				kline_t *k;

				/* kline them the third time */
				k = kline_add("*", u->host, "ten minute ban: flooding services", 600);
				k->setby = sstrdup(chansvs.nick);

				snoop("FLOOD:KLINE: \2%s\2", u->nick);

				return;
			}
		}
	}

	sptr = find_service(parv[0]);

	if (sptr)
		sptr->handler(origin, parc, parv);
}

static void m_njoin(char *origin, uint8_t parc, char *parv[])
{
	channel_t *c;
	uint8_t userc;
	char *userv[256];
	uint8_t i;

	if (origin)
	{
		c = channel_find(parv[0]);

		if (!c)
		{
			slog(LG_DEBUG, "m_njoin(): new channel: %s", parv[0]);
			c = channel_add(parv[0], CURRTIME);
		}

		userc = sjtoken(parv[parc - 1], ',', userv);

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
	if (parc == 7)
	{
		s = server_find(origin);
		if (!s)
		{
			slog(LG_DEBUG, "m_nick(): new user on nonexistant server: %s", origin);
			return;
		}

		slog(LG_DEBUG, "m_nick(): new user on `%s': %s", s->name, parv[0]);

		if ((k = kline_find(parv[2], parv[3])))
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

		u = user_add(parv[0], parv[2], parv[3], NULL, parv[1], parv[6], s);

		user_mode(u, parv[5]);

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
	server_add(parv[0], atoi(parv[1]), origin ? origin : 
		me.name, parv[2], parv[3]);

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
        server_login = &ircnet_server_login;
        introduce_nick = &ircnet_introduce_nick;
        quit_sts = &ircnet_quit_sts;
        wallops = &ircnet_wallops;
        join = &ircnet_join;
        kick = &ircnet_kick;
        msg = &ircnet_msg;
        notice = &ircnet_notice;
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

	mode_list = ircnet_mode_list;
	ignore_mode_list = ircnet_ignore_mode_list;
	status_mode_list = ircnet_status_mode_list;
	prefix_mode_list = ircnet_prefix_mode_list;

	ircd = &IRCNet;

        pcommand_add("PING", m_ping);
        pcommand_add("PONG", m_pong);
        pcommand_add("PRIVMSG", m_privmsg);
	pcommand_add("NOTICE", m_privmsg);
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
        pcommand_add("JOIN", m_join);
        pcommand_add("PASS", m_pass);
        pcommand_add("ERROR", m_error);
        pcommand_add("TOPIC", m_topic);

	m->mflags = MODTYPE_CORE;

	pmodule_loaded = TRUE;
}

