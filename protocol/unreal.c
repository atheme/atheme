/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains protocol support for bahamut-based ircd.
 *
 * $Id: unreal.c 6299 2006-09-06 15:23:54Z jilles $
 */

#include "atheme.h"
#include "uplink.h"
#include "pmodule.h"
#include "protocol/unreal.h"

DECLARE_MODULE_V1("protocol/unreal", TRUE, _modinit, NULL, "$Id: unreal.c 6299 2006-09-06 15:23:54Z jilles $", "Atheme Development Group <http://www.atheme.org>");

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
	PROTOCOL_UNREAL,		/* Protocol type */
	0,                              /* Permanent cmodes */
	"beI",                          /* Ban-like cmodes */
	'e',                            /* Except mchar */
	'I'                             /* Invex mchar */
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
  { 'G', CMODE_CENSOR   },
  { '\0', 0 }
};

static boolean_t check_jointhrottle(const char *, channel_t *, mychan_t *, user_t *, myuser_t *);

struct extmode unreal_ignore_mode_list[] = {
  { 'j', check_jointhrottle },
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

static boolean_t check_jointhrottle(const char *value, channel_t *c, mychan_t *mc, user_t *u, myuser_t *mu)
{
	const char *p, *arg2;

	p = value, arg2 = NULL;
	while (*p != '\0')
	{
		if (*p == ':')
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
	if (p - arg2 > 10 || arg2 - value - 1 > 10 || !atoi(value) || !atoi(arg2))
		return FALSE;
	return TRUE;
}

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
	sts("NICK %s 1 %ld %s %s %s 0 +%sS * :%s", nick, CURRTIME, user, host, me.name, "io", real);
}

/* invite a user to a channel */
static void unreal_invite_sts(user_t *sender, user_t *target, channel_t *channel)
{
	sts(":%s INVITE %s %s", sender->nick, target->nick, channel->name);
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
	user_t *u;

	if (!me.connected)
		return;

	/* Can only do this for nickserv, and can only record identified
	 * state if logged in to correct nick, sorry -- jilles
	 */
	if (nicksvs.me == NULL || irccasecmp(origin, user))
		return;

	u = user_find(origin);
	if (u == NULL)
		return;

	/* imo, we should be using SVS2MODE to show the modechange here and on logout --w00t */
	sts(":%s SVS2MODE %s +rd %ld", nicksvs.nick, origin, u->ts);
}

/* protocol-specific stuff to do on logout */
static boolean_t unreal_on_logout(char *origin, char *user, char *wantedhost)
{
	if (!me.connected)
		return FALSE;

	if (nicksvs.me == NULL || irccasecmp(origin, user))
		return FALSE;

	sts(":%s SVS2MODE %s -r+d 0", nicksvs.nick, origin);
	return FALSE;
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

	sts(":%s CHGHOST %s :%s", source, target, host);
}

static void unreal_fnc_sts(user_t *source, user_t *u, char *newnick, int type)
{
	sts(":%s SVSNICK %s %s %lu", source->nick, u->nick, newnick,
			(unsigned long)(CURRTIME - 60));
}

static void unreal_holdnick_sts(user_t *source, int duration, const char *nick, myuser_t *account)
{
	sts(":%s SVSHOLD %s %d :Reserved by %s for nickname owner (%s)",
			source->nick, nick, duration, source->nick,
			account != NULL ? account->name : nick);
}

static void m_topic(sourceinfo_t *si, uint8_t parc, char *parv[])
{
	channel_t *c = channel_find(parv[0]);

	if (!c)
		return;

	handle_topic_from(si, c, parv[1], atol(parv[2]), parv[3]);
}

static void m_ping(sourceinfo_t *si, uint8_t parc, char *parv[])
{
	/* reply to PING's */
	sts(":%s PONG %s %s", me.name, me.name, parv[0]);
}

static void m_pong(sourceinfo_t *si, uint8_t parc, char *parv[])
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

static void m_privmsg(sourceinfo_t *si, uint8_t parc, char *parv[])
{
	if (parc != 2)
		return;

	handle_message(si, parv[0], FALSE, parv[1]);
}

static void m_notice(sourceinfo_t *si, uint8_t parc, char *parv[])
{
	if (parc != 2)
		return;

	handle_message(si, parv[0], TRUE, parv[1]);
}

static void remove_our_modes(channel_t *c)
{
	/* the TS changed.  a TS change requires the following things
	 * to be done to the channel:  reset all modes to nothing, remove
	 * all status modes on known users on the channel (including ours),
	 * and set the new TS.
	 */
	chanuser_t *cu;
	node_t *n;

	clear_simple_modes(c);

	LIST_FOREACH(n, c->members.head)
	{
		cu = (chanuser_t *)n->data;
		cu->modes = 0;
	}
}

static void m_sjoin(sourceinfo_t *si, uint8_t parc, char *parv[])
{
	/*
	 *  -> :proteus.malkier.net SJOIN 1073516550 #shrike +tn :@sycobuny @+rakaur
	 *      also:
	 *  -> :nenolod_ SJOIN 1117334567 #chat
	 *      also:
	 *  -> SJOIN 1117334567 #chat :@nenolod
	 */

	channel_t *c;
	uint8_t userc;
	char *userv[256];
	uint8_t i;
	time_t ts;

	if (parc >= 4)
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
			remove_our_modes(c);
			slog(LG_INFO, "m_sjoin(): TS changed for %s (%ld -> %ld)", c->name, c->ts, ts);
			c->ts = ts;
			hook_call_event("channel_tschange", c);
		}

		channel_mode(NULL, c, parc - 3, parv - 2);
		userc = sjtoken(parv[parc - 1], ' ', userv);

		for (i = 0; i < userc; i++)
			if (*userv[i] == '&')	/* channel ban */
				chanban_add(c, userv[i] + 1, 'b');
			else if (*userv[i] == '"')	/* exception */
				chanban_add(c, userv[i] + 1, 'e');
			else if (*userv[i] == '\'')	/* invex */
				chanban_add(c, userv[i] + 1, 'I');
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
			slog(LG_DEBUG, "m_sjoin(): new channel: %s (modes lost)", parv[1]);
			c = channel_add(parv[1], ts);
		}

		if (ts < c->ts)
		{
			remove_our_modes(c);
			slog(LG_INFO, "m_sjoin(): TS changed for %s (%ld -> %ld)", c->name, c->ts, ts);
			c->ts = ts;
			hook_call_event("channel_tschange", c);
		}

		channel_mode_va(NULL, c, 1, "+");

		userc = sjtoken(parv[parc - 1], ' ', userv);

		for (i = 0; i < userc; i++)
			if (*userv[i] == '&')	/* channel ban */
				chanban_add(c, userv[i] + 1, 'b');
			else if (*userv[i] == '"')	/* exception */
				chanban_add(c, userv[i] + 1, 'e');
			else if (*userv[i] == '\'')	/* invex */
				chanban_add(c, userv[i] + 1, 'I');
			else
				chanuser_add(c, userv[i]);
	}
	else if (parc == 2)
	{
		c = channel_find(parv[1]);
		ts = atol(parv[0]);
		if (!c)
		{
			slog(LG_DEBUG, "m_sjoin(): new channel: %s (modes lost)", parv[1]);
			c = channel_add(parv[1], ts);
		}

		if (ts < c->ts)
		{
			remove_our_modes(c);
			slog(LG_INFO, "m_sjoin(): TS changed for %s (%ld -> %ld)", c->name, c->ts, ts);
			c->ts = ts;
			/* XXX lost modes! -- XXX - pardon? why do we worry about this? TS reset requires modes reset.. */
			hook_call_event("channel_tschange", c);
		}

		channel_mode_va(NULL, c, 1, "+");

		chanuser_add(c, si->su->nick);
	}
	else
		return;

	if (c->nummembers == 0 && !(c->modes & ircd->perm_mode))
		channel_delete(c->name);
}

static void m_part(sourceinfo_t *si, uint8_t parc, char *parv[])
{
	uint8_t chanc;
	char *chanv[256];
	int i;

	if (parc < 1)
		return;
	chanc = sjtoken(parv[0], ',', chanv);
	for (i = 0; i < chanc; i++)
	{
		slog(LG_DEBUG, "m_part(): user left channel: %s -> %s", si->su->nick, chanv[i]);

		chanuser_delete(channel_find(chanv[i]), si->su);
	}
}

static void m_nick(sourceinfo_t *si, uint8_t parc, char *parv[])
{
	server_t *s;
	user_t *u;

	if (parc == 10)
	{
		s = server_find(parv[5]);
		if (!s)
		{
			slog(LG_DEBUG, "m_nick(): new user on nonexistant server: %s", parv[5]);
			return;
		}

		slog(LG_DEBUG, "m_nick(): new user on `%s': %s", s->name, parv[0]);

		u = user_add(parv[0], parv[3], parv[4], parv[8], NULL, NULL, parv[9], s, atoi(parv[2]));

		user_mode(u, parv[7]);

		/* If the user's SVID is equal to their nick TS,
		 * they're properly logged in -- jilles */
		if (u->ts > 100 && (uint32_t)atoi(parv[6]) == u->ts)
			handle_burstlogin(u, parv[0]);

		handle_nickchange(u);
	}

	/* if it's only 2 then it's a nickname change */
	else if (parc == 2)
	{
		node_t *n;

                if (!si->su)
                {       
                        slog(LG_DEBUG, "m_nick(): server trying to change nick: %s", si->s != NULL ? si->s->name : "<none>");
                        return;
                }
                
		slog(LG_DEBUG, "m_nick(): nickname change from `%s': %s", si->su->nick, parv[0]);

		/* fix up +r if necessary -- jilles */
		if (nicksvs.me != NULL && si->su->myuser != NULL && !(si->su->myuser->flags & MU_WAITAUTH) && irccasecmp(si->su->nick, parv[0]))
		{
			if (!irccasecmp(parv[0], si->su->myuser->name))
				/* changed nick to registered one, reset +r */
				sts(":%s SVS2MODE %s +rd %ld", nicksvs.nick, parv[0], atoi(parv[1]));
			else if (!irccasecmp(si->su->nick, si->su->myuser->name))
				/* changed from registered nick, remove +r */
				sts(":%s SVS2MODE %s -r+d 0", nicksvs.nick, parv[0]);
		}

		/* remove the current one from the list */
		n = node_find(si->su, &userlist[si->su->hash]);
		node_del(n, &userlist[si->su->hash]);
		node_free(n);

		/* change the nick */
		strlcpy(si->su->nick, parv[0], NICKLEN);
		si->su->ts = atoi(parv[1]);

		/* readd with new nick (so the hash works) */
		n = node_create();
		si->su->hash = UHASH((unsigned char *)si->su->nick);
		node_add(si->su, n, &userlist[si->su->hash]);

		handle_nickchange(si->su);
	}
	else
	{
		int i;
		slog(LG_DEBUG, "m_nick(): got NICK with wrong number of params");

		for (i = 0; i < parc; i++)
			slog(LG_DEBUG, "m_nick():   parv[%d] = %s", i, parv[i]);
	}
}

static void m_quit(sourceinfo_t *si, uint8_t parc, char *parv[])
{
	slog(LG_DEBUG, "m_quit(): user leaving: %s", si->su->nick);

	/* user_delete() takes care of removing channels and so forth */
	user_delete(si->su);
}

static void m_mode(sourceinfo_t *si, uint8_t parc, char *parv[])
{
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

static void m_umode(sourceinfo_t *si, uint8_t parc, char *parv[])
{
	user_mode(si->su, parv[0]);
}

static void m_kick(sourceinfo_t *si, uint8_t parc, char *parv[])
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

static void m_kill(sourceinfo_t *si, uint8_t parc, char *parv[])
{
	if (parc < 1)
		return;
	handle_kill(si, parv[0], parc > 1 ? parv[1] : "<No reason given>");
}

static void m_squit(sourceinfo_t *si, uint8_t parc, char *parv[])
{
	slog(LG_DEBUG, "m_squit(): server leaving: %s from %s", parv[0], parv[1]);
	server_delete(parv[0]);
}

static void m_server(sourceinfo_t *si, uint8_t parc, char *parv[])
{
	slog(LG_DEBUG, "m_server(): new server: %s", parv[0]);
	server_add(parv[0], atoi(parv[1]), si->origin ? si->origin : me.name, NULL, parv[2]);

	if (cnt.server == 2)
		me.actual = sstrdup(parv[0]);
	else
	{
		/* elicit PONG for EOB detection; pinging uplink is
		 * already done elsewhere -- jilles
		 */
		sts(":%s PING %s %s", me.name, me.name, parv[0]);
	}

	me.recvsvr = TRUE;
}

static void m_stats(sourceinfo_t *si, uint8_t parc, char *parv[])
{
	handle_stats(si->su, parv[0][0]);
}

static void m_admin(sourceinfo_t *si, uint8_t parc, char *parv[])
{
	handle_admin(si->su);
}

static void m_version(sourceinfo_t *si, uint8_t parc, char *parv[])
{
	handle_version(si->su);
}

static void m_info(sourceinfo_t *si, uint8_t parc, char *parv[])
{
	handle_info(si->su);
}

static void m_whois(sourceinfo_t *si, uint8_t parc, char *parv[])
{
	handle_whois(si->su, parc >= 2 ? parv[1] : "*");
}

static void m_trace(sourceinfo_t *si, uint8_t parc, char *parv[])
{
	handle_trace(si->su, parc >= 1 ? parv[0] : "*", parc >= 2 ? parv[1] : NULL);
}

static void m_join(sourceinfo_t *si, uint8_t parc, char *parv[])
{
	chanuser_t *cu;
	node_t *n, *tn;

	/* JOIN 0 is really a part from all channels */
	if (parv[0][0] == '0')
	{
		LIST_FOREACH_SAFE(n, tn, si->su->channels.head)
		{
			cu = (chanuser_t *)n->data;
			chanuser_delete(cu->chan, si->su);
		}
	}
}

static void m_pass(sourceinfo_t *si, uint8_t parc, char *parv[])
{
	if (strcmp(curr_uplink->pass, parv[0]))
	{
		slog(LG_INFO, "m_pass(): password mismatch from uplink; aborting");
		runflags |= RF_SHUTDOWN;
	}
}

static void m_error(sourceinfo_t *si, uint8_t parc, char *parv[])
{
	slog(LG_INFO, "m_error(): error from server: %s", parv[0]);
}

static void m_chghost(sourceinfo_t *si, uint8_t parc, char *parv[])
{
	user_t *u = user_find(parv[0]);

	if (!u)
		return;

	strlcpy(u->vhost, parv[1], HOSTLEN);
}

static void m_motd(sourceinfo_t *si, uint8_t parc, char *parv[])
{
	handle_motd(si->su);
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
	notice_sts = &unreal_notice;
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
	fnc_sts = &unreal_fnc_sts;
	invite_sts = &unreal_invite_sts;
	holdnick_sts = &unreal_holdnick_sts;

	mode_list = unreal_mode_list;
	ignore_mode_list = unreal_ignore_mode_list;
	status_mode_list = unreal_status_mode_list;
	prefix_mode_list = unreal_prefix_mode_list;

	ircd = &Unreal;

	pcommand_add("PING", m_ping, 1, MSRC_USER | MSRC_SERVER);
	pcommand_add("PONG", m_pong, 1, MSRC_SERVER);
	pcommand_add("PRIVMSG", m_privmsg, 2, MSRC_USER);
	pcommand_add("NOTICE", m_notice, 2, MSRC_UNREG | MSRC_USER | MSRC_SERVER);
	pcommand_add("SJOIN", m_sjoin, 2, MSRC_USER | MSRC_SERVER);
	pcommand_add("PART", m_part, 1, MSRC_USER);
	pcommand_add("NICK", m_nick, 2, MSRC_USER | MSRC_SERVER);
	pcommand_add("QUIT", m_quit, 1, MSRC_USER);
	pcommand_add("UMODE2", m_umode, 1, MSRC_USER);
	pcommand_add("MODE", m_mode, 2, MSRC_USER | MSRC_SERVER);
	pcommand_add("KICK", m_kick, 2, MSRC_USER | MSRC_SERVER);
	pcommand_add("KILL", m_kill, 1, MSRC_USER | MSRC_SERVER);
	pcommand_add("SQUIT", m_squit, 1, MSRC_USER | MSRC_SERVER);
	pcommand_add("SERVER", m_server, 3, MSRC_UNREG | MSRC_SERVER);
	pcommand_add("STATS", m_stats, 2, MSRC_USER);
	pcommand_add("ADMIN", m_admin, 1, MSRC_USER);
	pcommand_add("VERSION", m_version, 1, MSRC_USER);
	pcommand_add("INFO", m_info, 1, MSRC_USER);
	pcommand_add("WHOIS", m_whois, 2, MSRC_USER);
	pcommand_add("TRACE", m_trace, 1, MSRC_USER);
	pcommand_add("JOIN", m_join, 1, MSRC_USER);
	pcommand_add("PASS", m_pass, 1, MSRC_UNREG);
	pcommand_add("ERROR", m_error, 1, MSRC_UNREG | MSRC_SERVER);
	pcommand_add("TOPIC", m_topic, 4, MSRC_USER | MSRC_SERVER);
	pcommand_add("CHGHOST", m_chghost, 2, MSRC_USER | MSRC_SERVER);
	pcommand_add("MOTD", m_motd, 1, MSRC_USER);

	/* 
	 * for fun, and to give nenolod a heart attack
	 * whenever he reads unreal.c, let us do tokens... --w00t
	 * 
	 * oh, as a side warning: don't remove the above. not only
	 * are they for compatability, but unreal's server protocol
	 * can be odd in places (ie not sending JOIN token after synch
	 * with no SJOIN, not using KILL token... etc. --w00t.
	 */

	pcommand_add("8", m_ping, 1, MSRC_USER | MSRC_SERVER);
	pcommand_add("9", m_pong, 1, MSRC_SERVER);
	pcommand_add("!", m_privmsg, 2, MSRC_USER);
	pcommand_add("B", m_notice, 2, MSRC_UNREG | MSRC_USER | MSRC_SERVER);
	pcommand_add("~", m_sjoin, 2, MSRC_USER | MSRC_SERVER);
	pcommand_add("D", m_part, 1, MSRC_USER);
	pcommand_add("&", m_nick, 2, MSRC_USER | MSRC_SERVER);
	pcommand_add(",", m_quit, 1, MSRC_USER);
	pcommand_add("|", m_umode, 1, MSRC_USER);
	pcommand_add("G", m_mode, 2, MSRC_USER | MSRC_SERVER);
	pcommand_add("H", m_kick, 2, MSRC_USER | MSRC_SERVER);
	pcommand_add(".", m_kill, 1, MSRC_USER | MSRC_SERVER);
	pcommand_add("-", m_squit, 1, MSRC_USER | MSRC_SERVER);
	pcommand_add("'", m_server, 3, MSRC_UNREG | MSRC_SERVER);
	pcommand_add("2", m_stats, 2, MSRC_USER);
	pcommand_add("@", m_admin, 1, MSRC_USER);
	pcommand_add("+", m_version, 1, MSRC_USER);
	pcommand_add("/", m_info, 1, MSRC_USER);
	pcommand_add("C", m_join, 1, MSRC_USER);
	pcommand_add("<", m_pass, 1, MSRC_UNREG);
	pcommand_add("5", m_error, 1, MSRC_UNREG | MSRC_SERVER);
	pcommand_add(")", m_topic, 4, MSRC_USER | MSRC_SERVER);
	pcommand_add("AL", m_chghost, 2, MSRC_USER | MSRC_SERVER);
	pcommand_add("F", m_motd, 1, MSRC_USER);

	m->mflags = MODTYPE_CORE;

	pmodule_loaded = TRUE;
}
