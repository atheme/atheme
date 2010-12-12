/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Copyright (c) 2005-2006 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains protocol support for bahamut-based ircd.
 *
 */

#include "atheme.h"
#include "uplink.h"
#include "pmodule.h"
#include "protocol/unreal.h"

DECLARE_MODULE_V1("protocol/unreal", true, _modinit, NULL, PACKAGE_STRING, "Atheme Development Group <http://www.atheme.org>");

static bool use_esvid = false;

/* *INDENT-OFF* */

ircd_t Unreal = {
        "UnrealIRCd 3.1 or later",      /* IRCd name */
        "$",                            /* TLD Prefix, used by Global. */
        false,                          /* Whether or not we use IRCNet/TS6 UID */
        false,                          /* Whether or not we use RCOMMAND */
        true,                           /* Whether or not we support channel owners. */
        true,                           /* Whether or not we support channel protection. */
        true,                           /* Whether or not we support halfops. */
	false,				/* Whether or not we use P10 */
	true,				/* Whether or not we use vHosts. */
	CMODE_OPERONLY | CMODE_ADMONLY, /* Oper-only cmodes */
        CSTATUS_OWNER,                    /* Integer flag for owner channel flag. */
        CSTATUS_PROTECT,                  /* Integer flag for protect channel flag. */
        CSTATUS_HALFOP,                   /* Integer flag for halfops. */
        "+q",                           /* Mode we set for owner. */
        "+a",                           /* Mode we set for protect. */
        "+h",                           /* Mode we set for halfops. */
	PROTOCOL_UNREAL,		/* Protocol type */
	0,                              /* Permanent cmodes */
	0,                              /* Oper-immune cmode */
	"beI",                          /* Ban-like cmodes */
	'e',                            /* Except mchar */
	'I',                            /* Invex mchar */
	IRCD_HOLDNICK                   /* Flags */
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

static bool check_jointhrottle(const char *, channel_t *, mychan_t *, user_t *, myuser_t *);
static bool check_flood(const char *value, channel_t *c, mychan_t *mc, user_t *u, myuser_t *mu);
static bool check_forward(const char *value, channel_t *c, mychan_t *mc, user_t *u, myuser_t *mu);

struct extmode unreal_ignore_mode_list[] = {
  { 'j', check_jointhrottle },
  { 'f', check_flood },
  { 'L', check_forward },
  { '\0', 0 }
};

struct cmode_ unreal_status_mode_list[] = {
  { 'q', CSTATUS_OWNER   },
  { 'a', CSTATUS_PROTECT },
  { 'o', CSTATUS_OP      },
  { 'h', CSTATUS_HALFOP  },
  { 'v', CSTATUS_VOICE   },
  { '\0', 0 }
};

struct cmode_ unreal_prefix_mode_list[] = {
  { '*', CSTATUS_OWNER   },
  { '~', CSTATUS_PROTECT },
  { '@', CSTATUS_OP      },
  { '%', CSTATUS_HALFOP  },
  { '+', CSTATUS_VOICE   },
  { '\0', 0 }
};

struct cmode_ unreal_user_mode_list[] = {
  { 'A', UF_ADMIN    },
  { 'i', UF_INVIS    },
  { 'o', UF_IRCOP    },
  { 'd', UF_DEAF     },
  { '\0', 0 }
};

/* *INDENT-ON* */

static bool check_jointhrottle(const char *value, channel_t *c, mychan_t *mc, user_t *u, myuser_t *mu)
{
	const char *p, *arg2;

	p = value, arg2 = NULL;
	while (*p != '\0')
	{
		if (*p == ':')
		{
			if (arg2 != NULL)
				return false;
			arg2 = p + 1;
		}
		else if (!isdigit(*p))
			return false;
		p++;
	}
	if (arg2 == NULL)
		return false;
	if (p - arg2 > 10 || arg2 - value - 1 > 10 || !atoi(value) || !atoi(arg2))
		return false;
	return true;
}

static bool check_flood(const char *value, channel_t *c, mychan_t *mc, user_t *u, myuser_t *mu)
{
	/* way too complicated */
	return false;
}

static bool check_forward(const char *value, channel_t *c, mychan_t *mc, user_t *u, myuser_t *mu)
{
	channel_t *target_c;
	mychan_t *target_mc;

	if (*value != '#' || strlen(value) > 50)
		return false;
	if (u == NULL && mu == NULL)
		return true;
	target_c = channel_find(value);
	target_mc = mychan_find(value);
	if (target_c == NULL && target_mc == NULL)
		return false;
	return true;
}

/* login to our uplink */
static unsigned int unreal_server_login(void)
{
	int ret;

	ret = sts("PASS %s", curr_uplink->pass);
	if (ret == 1)
		return 1;

	me.bursting = true;

	sts("PROTOCTL TOKEN NICKv2 VHP NICKIP UMODE2 SJOIN SJOIN2 SJ3 NOQUIT TKLEXT ESVID");
	sts("SERVER %s 1 :%s", me.name, me.desc);

	services_init();

	return 0;
}

/* introduce a client */
static void unreal_introduce_nick(user_t *u)
{
	const char *umode = user_get_umodestr(u);

	sts("NICK %s 1 %lu %s %s %s * %sS * :%s", u->nick, (unsigned long)u->ts, u->user, u->host, me.name, umode, u->gecos);
}

/* invite a user to a channel */
static void unreal_invite_sts(user_t *sender, user_t *target, channel_t *channel)
{
	sts(":%s INVITE %s %s", sender->nick, target->nick, channel->name);
}

static void unreal_quit_sts(user_t *u, const char *reason)
{
	if (!me.connected)
		return;

	sts(":%s QUIT :%s", u->nick, reason);
}

/* WALLOPS wrapper */
static void unreal_wallops_sts(const char *text)
{
	sts(":%s GLOBOPS :%s", me.name, text);
}

/* join a channel */
static void unreal_join_sts(channel_t *c, user_t *u, bool isnew, char *modes)
{
	if (isnew)
		sts(":%s SJOIN %lu %s %s :@%s", me.name, (unsigned long)c->ts,
				c->name, modes, u->nick);
	else
		sts(":%s SJOIN %lu %s + :@%s", me.name, (unsigned long)c->ts,
				c->name, u->nick);
}

/* lower TS */
static void unreal_chan_lowerts(channel_t *c, user_t *u)
{
	slog(LG_DEBUG, "unreal_chan_lowerts(): lowering TS for %s to %lu",
			c->name, (unsigned long)c->ts);
	sts(":%s SJOIN %lu %s %s :@%s", ME, (unsigned long)c->ts, c->name,
			channel_modes(c, true), CLIENT_NAME(u));
}

/* kicks a user from a channel */
static void unreal_kick(user_t *source, channel_t *c, user_t *u, const char *reason)
{
	sts(":%s KICK %s %s :%s", source->nick, c->name, u->nick, reason);

	chanuser_delete(c, u);
}

/* PRIVMSG wrapper */
static void unreal_msg(const char *from, const char *target, const char *fmt, ...)
{
	va_list ap;
	char buf[BUFSIZE];

	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZE, fmt, ap);
	va_end(ap);

	sts(":%s PRIVMSG %s :%s", from, target, buf);
}

static void unreal_msg_global_sts(user_t *from, const char *mask, const char *text)
{
	mowgli_node_t *n;
	tld_t *tld;

	if (!strcmp(mask, "*"))
	{
		MOWGLI_ITER_FOREACH(n, tldlist.head)
		{
			tld = n->data;
			sts(":%s PRIVMSG %s*%s :%s", from ? from->nick : me.name, ircd->tldprefix, tld->name, text);
		}
	}
	else
		sts(":%s PRIVMSG %s%s :%s", from ? from->nick : me.name, ircd->tldprefix, mask, text);
}

/* NOTICE wrapper */
static void unreal_notice_user_sts(user_t *from, user_t *target, const char *text)
{
	sts(":%s NOTICE %s :%s", from ? from->nick : me.name, target->nick, text);
}

static void unreal_notice_global_sts(user_t *from, const char *mask, const char *text)
{
	mowgli_node_t *n;
	tld_t *tld;

	if (!strcmp(mask, "*"))
	{
		MOWGLI_ITER_FOREACH(n, tldlist.head)
		{
			tld = n->data;
			sts(":%s NOTICE %s*%s :%s", from ? from->nick : me.name, ircd->tldprefix, tld->name, text);
		}
	}
	else
		sts(":%s NOTICE %s%s :%s", from ? from->nick : me.name, ircd->tldprefix, mask, text);
}

static void unreal_notice_channel_sts(user_t *from, channel_t *target, const char *text)
{
	sts(":%s NOTICE %s :%s", from ? from->nick : me.name, target->name, text);
}

static void unreal_numeric_sts(server_t *from, int numeric, user_t *target, const char *fmt, ...)
{
	va_list ap;
	char buf[BUFSIZE];

	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZE, fmt, ap);
	va_end(ap);

	sts(":%s %d %s %s", from->name, numeric, target->nick, buf);
}

/* KILL wrapper */
static void unreal_kill_id_sts(user_t *killer, const char *id, const char *reason)
{
	if (killer != NULL)
	{
		if (nicksvs.me != NULL && killer == nicksvs.me->me)
		{
			sts(":%s SVSKILL %s :Killed (%s (%s))",
					killer->nick, id,
					killer->nick, reason);
			/* We still send KILL too, for two reasons:
			 * 1. SVSKILL does not do nick chase as KILL does,
			 *    so the below KILL ensures the user is removed
			 *    everywhere if kill and nick change cross.
			 *    If this happens between two ircds, unrealircd's
			 *    unknown prefix kills (nick(?) <- someserver)
			 *    will fix it up, but we do not kill unknown
			 *    prefixes.
			 * 2. SVSKILL is ignored if we are not U:lined.
			 *
			 * If the SVSKILL is effective, the KILL will be
			 * dropped.
			 */
		}
		sts(":%s KILL %s :%s!%s (%s)", killer->nick, id, killer->host, killer->nick, reason);
	}
	else
		sts(":%s KILL %s :%s (%s)", me.name, id, me.name, reason);
}

/* PART wrapper */
static void unreal_part_sts(channel_t *c, user_t *u)
{
	sts(":%s PART %s", u->nick, c->name);
}

/* server-to-server KLINE wrapper */
static void unreal_kline_sts(const char *server, const char *user, const char *host, long duration, const char *reason)
{
	service_t *svs;

	if (!me.connected)
		return;

	svs = service_find("operserv");
	sts(":%s TKL + G %s %s %s %lu %lu :%s", me.name, user, host, svs != NULL ? svs->nick : me.name, (unsigned long)(duration > 0 ? CURRTIME + duration : 0), (unsigned long)CURRTIME, reason);
}

/* server-to-server UNKLINE wrapper */
static void unreal_unkline_sts(const char *server, const char *user, const char *host)
{
	service_t *svs;

	if (!me.connected)
		return;

	svs = service_find("operserv");
	sts(":%s TKL - G %s %s %s", me.name, user, host, svs != NULL ? svs->nick : me.name);
}

static void unreal_xline_sts(const char *server, const char *realname, long duration, const char *reason)
{
	char escapedreason[512], *p;

	if (!me.connected)
		return;

	if (duration > 0)
	{
		slog(LG_INFO, "SGLINE: Could not set temporary SGLINE on \2%s\2, not supported by unrealircd.", realname);
		return;
	}

	strlcpy(escapedreason, reason, sizeof escapedreason);
	for (p = escapedreason; *p != '\0'; p++)
		if (*p == ' ')
			*p = '_';
	if (*escapedreason == ':')
		*escapedreason = ';';

	sts(":%s BR + %s :%s", me.name, escapedreason, realname);
}

static void unreal_unxline_sts(const char *server, const char *realname)
{
	if (!me.connected)
		return;

	sts(":%s BR - :%s", me.name, realname);
}

static void unreal_qline_sts(const char *server, const char *name, long duration, const char *reason)
{
	service_t *svs;

	if (!me.connected)
		return;

	if (*name == '#' || *name == '&')
	{
		slog(LG_INFO, "SQLINE: Could not set SQLINE on \2%s\2, not supported by unrealircd.", name);
		return;
	}

	svs = service_find("operserv");
	sts(":%s TKL + Q * %s %s %lu %lu :%s", me.name, name, svs != NULL ? svs->nick : me.name, (unsigned long)(duration > 0 ? CURRTIME + duration : 0), (unsigned long)CURRTIME, reason);
}

static void unreal_unqline_sts(const char *server, const char *name)
{
	service_t *svs;

	if (!me.connected)
		return;

	svs = service_find("operserv");
	sts(":%s TKL - Q * %s %s", me.name, name, svs != NULL ? svs->nick : me.name);
}

/* topic wrapper */
static void unreal_topic_sts(channel_t *c, user_t *source, const char *setter, time_t ts, time_t prevts, const char *topic)
{
	if (!me.connected || !c)
		return;

	sts(":%s TOPIC %s %s %lu :%s", source->nick, c->name, setter, (unsigned long)ts, topic);
}

/* mode wrapper */
static void unreal_mode_sts(char *sender, channel_t *target, char *modes)
{
	if (!me.connected)
		return;

	sts(":%s MODE %s %s", sender, target->name, modes);
}

/* ping wrapper */
static void unreal_ping_sts(void)
{
	if (!me.connected)
		return;

	sts("PING :%s", me.name);
}

/* protocol-specific stuff to do on login */
static void unreal_on_login(user_t *u, myuser_t *account, const char *wantedhost)
{
	if (!me.connected || u == NULL)
		return;

	if (!use_esvid)
	{
		/* Can only do this for nickserv, and can only record identified
		 * state if logged in to correct nick, sorry -- jilles
		 */
		/* imo, we should be using SVS2MODE to show the modechange here and on logout --w00t */
		if (should_reg_umode(u))
			sts(":%s SVS2MODE %s +rd %lu", nicksvs.nick, u->nick, (unsigned long)u->ts);

		return;
	}

	sts(":%s SVS2MODE %s +rd %s", nicksvs.nick, u->nick, entity(account)->name);
}

/* protocol-specific stuff to do on logout */
static bool unreal_on_logout(user_t *u, const char *account)
{
	if (!me.connected || u == NULL)
		return false;

	if (!use_esvid && !nicksvs.no_nick_ownership)
		sts(":%s SVS2MODE %s -r+d 0", nicksvs.nick, u->nick);
	else
		sts(":%s SVS2MODE %s -r+d *", nicksvs.nick, u->nick);

	return false;
}

static void unreal_jupe(const char *server, const char *reason)
{
	service_t *svs;

	if (!me.connected)
		return;

	server_delete(server);

	svs = service_find("operserv");
	sts(":%s SQUIT %s :%s", svs != NULL ? svs->nick : me.name, server, reason);
	sts(":%s SERVER %s 2 :%s", me.name, server, reason);
}

static void unreal_sethost_sts(user_t *source, user_t *target, const char *host)
{
	if (!me.connected)
		return;

	if (irccasecmp(target->host, host))
		numeric_sts(me.me, 396, target, "%s :is now your hidden host (set by %s)", host, source->nick);
	else
		numeric_sts(me.me, 396, target, "%s :hostname reset by %s", host, source->nick);
	sts(":%s CHGHOST %s :%s", source->nick, target->nick, host);
}

static void unreal_fnc_sts(user_t *source, user_t *u, char *newnick, int type)
{
	sts(":%s SVSNICK %s %s %lu", source->nick, u->nick, newnick,
			(unsigned long)(CURRTIME - 60));
}

static void unreal_holdnick_sts(user_t *source, int duration, const char *nick, myuser_t *mu)
{
	if (duration > 0)
		sts(":%s TKL + Q H %s %s %lu %lu :Reserved by %s for nickname owner (%s)",
				me.name, nick, source->nick,
				(unsigned long)(CURRTIME + duration),
				(unsigned long)CURRTIME,
				source->nick,
				mu ? entity(mu)->name : nick);
	else
		sts(":%s TKL - Q H %s %s", me.name, nick, source->nick);
}

static void m_topic(sourceinfo_t *si, int parc, char *parv[])
{
	channel_t *c = channel_find(parv[0]);

	if (!c)
		return;

	/* Our uplink is trying to change the topic during burst,
	 * and we have already set a topic. Assume our change won.
	 * -- jilles */
	if (si->s != NULL && si->s->uplink == me.me &&
			!(si->s->flags & SF_EOB) && c->topic != NULL)
		return;

	handle_topic_from(si, c, parv[1], atol(parv[2]), parv[3]);
}

static void m_ping(sourceinfo_t *si, int parc, char *parv[])
{
	/* reply to PING's */
	sts(":%s PONG %s %s", me.name, me.name, parv[0]);
}

static void m_pong(sourceinfo_t *si, int parc, char *parv[])
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

		wallops("Finished synchronizing with network in %d %s.", (tv2ms(&burstime) > 1000) ? (tv2ms(&burstime) / 1000) : tv2ms(&burstime), (tv2ms(&burstime) > 1000) ? "s" : "ms");
#else
		slog(LG_INFO, "m_pong(): finished synching with uplink");
		wallops("Finished synchronizing with network.");
#endif

		me.bursting = false;
	}
}

static void m_privmsg(sourceinfo_t *si, int parc, char *parv[])
{
	if (parc != 2)
		return;

	handle_message(si, parv[0], false, parv[1]);
}

static void m_notice(sourceinfo_t *si, int parc, char *parv[])
{
	if (parc != 2)
		return;

	handle_message(si, parv[0], true, parv[1]);
}

static void remove_our_modes(channel_t *c)
{
	/* the TS changed.  a TS change requires the following things
	 * to be done to the channel:  reset all modes to nothing, remove
	 * all status modes on known users on the channel (including ours),
	 * and set the new TS.
	 */
	chanuser_t *cu;
	mowgli_node_t *n;

	clear_simple_modes(c);

	MOWGLI_ITER_FOREACH(n, c->members.head)
	{
		cu = (chanuser_t *)n->data;
		cu->modes = 0;
	}
}

static void m_sjoin(sourceinfo_t *si, int parc, char *parv[])
{
	/*
	 *  -> :proteus.malkier.net SJOIN 1073516550 #shrike +tn :@sycobuny @+rakaur
	 *      also:
	 *  -> :nenolod_ SJOIN 1117334567 #chat
	 *      also:
	 *  -> SJOIN 1117334567 #chat :@nenolod
	 */

	channel_t *c;
	unsigned int userc;
	char *userv[256];
	unsigned int i;
	time_t ts;

	if (parc >= 4)
	{
		/* :origin SJOIN ts chan modestr [key or limits] :users */
		c = channel_find(parv[1]);
		ts = atol(parv[0]);

		if (!c)
		{
			slog(LG_DEBUG, "m_sjoin(): new channel: %s", parv[1]);
			c = channel_add(parv[1], ts, si->s);
		}

		if (ts < c->ts)
		{
			remove_our_modes(c);
			slog(LG_DEBUG, "m_sjoin(): TS changed for %s (%lu -> %lu)", c->name, (unsigned long)c->ts, (unsigned long)ts);
			c->ts = ts;
			hook_call_channel_tschange(c);
		}

		channel_mode(NULL, c, parc - 3, parv + 2);
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
			c = channel_add(parv[1], ts, si->s);
		}

		if (ts < c->ts)
		{
			remove_our_modes(c);
			slog(LG_DEBUG, "m_sjoin(): TS changed for %s (%lu -> %lu)", c->name, (unsigned long)c->ts, (unsigned long)ts);
			c->ts = ts;
			hook_call_channel_tschange(c);
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
			c = channel_add(parv[1], ts, si->su->server);
		}

		if (ts < c->ts)
		{
			remove_our_modes(c);
			slog(LG_DEBUG, "m_sjoin(): TS changed for %s (%lu -> %lu)", c->name, (unsigned long)c->ts, (unsigned long)ts);
			c->ts = ts;
			/* XXX lost modes! -- XXX - pardon? why do we worry about this? TS reset requires modes reset.. */
			hook_call_channel_tschange(c);
		}

		channel_mode_va(NULL, c, 1, "+");

		chanuser_add(c, si->su->nick);
	}
	else
		return;

	if (c->nummembers == 0 && !(c->modes & ircd->perm_mode))
		channel_delete(c);
}

static void m_part(sourceinfo_t *si, int parc, char *parv[])
{
	int chanc;
	char *chanv[256];
	int i;

	chanc = sjtoken(parv[0], ',', chanv);
	for (i = 0; i < chanc; i++)
	{
		slog(LG_DEBUG, "m_part(): user left channel: %s -> %s", si->su->nick, chanv[i]);

		chanuser_delete(channel_find(chanv[i]), si->su);
	}
}

static void m_nick(sourceinfo_t *si, int parc, char *parv[])
{
	server_t *s;
	user_t *u;
	bool realchange;
	const char *vhost;
	const char *ipb64;
	char ipstring[64];
	int af;
	size_t iplen;
	char ipdata[16];

	if (parc == 10 || parc == 11)
	{
		s = server_find(parv[5]);
		if (!s)
		{
			slog(LG_DEBUG, "m_nick(): new user on nonexistant server: %s", parv[5]);
			return;
		}

		slog(LG_DEBUG, "m_nick(): new user on `%s': %s", s->name, parv[0]);

		vhost = strcmp(parv[8], "*") ? parv[8] : NULL;
		iplen = 0;
		if (parc == 11 && strcmp(parv[parc - 2], "*"))
		{
			ipb64 = parv[parc - 2];
			af = AF_INET;
			if (strlen(ipb64) == 8)
			{
				iplen = 4;
				if (!base64_decode(ipb64, 8, ipdata, &iplen) ||
						iplen != 4)
					iplen = 0;
				af = AF_INET;
			}
#ifdef AF_INET6
			else if (strlen(ipb64) == 24)
			{
				iplen = 16;
				if (!base64_decode(ipb64, 24, ipdata, &iplen) ||
						iplen != 16)
					iplen = 0;
				af = AF_INET6;
			}
#endif
			if (iplen != 0)
				if (!inet_ntop(af, ipdata, ipstring, sizeof ipstring))
					iplen = 0;
		}
		u = user_add(parv[0], parv[3], parv[4], vhost, iplen != 0 ? ipstring : NULL, NULL, parv[parc - 1], s, atoi(parv[2]));
		if (u == NULL)
			return;

		user_mode(u, parv[7]);

		/*
		 * with ESVID:
		 * If the user's SVID is equal to their accountname,
		 * they're properly logged in.  Alternatively, the
		 * 'without ESVID' criteria is used. --nenolod
		 *
		 * without ESVID:
		 * If the user's SVID is equal to their nick TS,
		 * they're properly logged in --jilles
		 */
		if (use_esvid && *parv[6] != '*' && !IsDigit(*parv[6]))
			handle_burstlogin(u, parv[6], 0);
		else if (u->ts > 100 && (time_t)atoi(parv[6]) == u->ts)
			handle_burstlogin(u, NULL, 0);

		handle_nickchange(u);
	}

	/* if it's only 2 then it's a nickname change */
	else if (parc == 2)
	{
                if (!si->su)
                {       
                        slog(LG_DEBUG, "m_nick(): server trying to change nick: %s", si->s != NULL ? si->s->name : "<none>");
                        return;
                }
                
		slog(LG_DEBUG, "m_nick(): nickname change from `%s': %s", si->su->nick, parv[0]);

		realchange = irccasecmp(si->su->nick, parv[0]);

		if (user_changenick(si->su, parv[0], atoi(parv[1])))
			return;

		/* fix up +r if necessary -- jilles */
		if (realchange && !nicksvs.no_nick_ownership && !use_esvid)
		{
			if (should_reg_umode(si->su))
				/* changed nick to registered one, reset +r */
				sts(":%s SVS2MODE %s +rd %lu", nicksvs.nick, parv[0], atol(parv[1]));
			else
				/* changed from registered nick, remove +r */
				sts(":%s SVS2MODE %s -r+d 0", nicksvs.nick, parv[0]);
		}

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

static void m_quit(sourceinfo_t *si, int parc, char *parv[])
{
	slog(LG_DEBUG, "m_quit(): user leaving: %s", si->su->nick);

	/* user_delete() takes care of removing channels and so forth */
	user_delete(si->su, parv[0]);
}

static void unreal_user_mode(user_t *u, const char *changes)
{
	const char *p;
	int dir;

	if (u == NULL)
		return;
	user_mode(u, changes);
	dir = 0;
	for (p = changes; *p != '\0'; p++)
		switch (*p)
		{
			case '-': dir = MTYPE_DEL; break;
			case '+': dir = MTYPE_ADD; break;
			case 'x':
				/* If +x is set then the users vhost is set to their cloaked host - Adam */
				if (dir == MTYPE_ADD)
				{
					/* It is possible for the users vhost to not be their cloaked host after +x.
					 * This only occurs when a user is introduced after a netmerge with their
					 * vhost instead of their cloaked host. - Adam
					 */
					if (strcmp(u->vhost, u->chost))
						strlcpy(u->chost, u->vhost, HOSTLEN);
				}
				else if (dir == MTYPE_DEL)
					strlcpy(u->vhost, u->host, HOSTLEN);
				break;
		}
}

static void m_mode(sourceinfo_t *si, int parc, char *parv[])
{
	if (*parv[0] == '#')
		channel_mode(NULL, channel_find(parv[0]), parc - 1, &parv[1]);
	else
		unreal_user_mode(user_find(parv[0]), parv[1]);
}

static void m_umode(sourceinfo_t *si, int parc, char *parv[])
{
	unreal_user_mode(si->su, parv[0]);
}

static void m_kick(sourceinfo_t *si, int parc, char *parv[])
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

static void m_kill(sourceinfo_t *si, int parc, char *parv[])
{
	handle_kill(si, parv[0], parc > 1 ? parv[1] : "<No reason given>");
}

static void m_squit(sourceinfo_t *si, int parc, char *parv[])
{
	slog(LG_DEBUG, "m_squit(): server leaving: %s from %s", parv[0], parv[1]);
	server_delete(parv[0]);
}

static void m_server(sourceinfo_t *si, int parc, char *parv[])
{
	server_t *s;
	const char *inf;

	slog(LG_DEBUG, "m_server(): new server: %s", parv[0]);
	if (si->s == NULL && (inf = strchr(parv[2], ' ')) != NULL)
		inf++;
	else
		inf = parv[2];
	s = handle_server(si, parv[0], NULL, atoi(parv[1]), inf);

	if (s != NULL && s->uplink != me.me)
	{
		/* elicit PONG for EOB detection; pinging uplink is
		 * already done elsewhere -- jilles
		 */
		sts(":%s PING %s %s", me.name, me.name, s->name);
	}
}

static void m_stats(sourceinfo_t *si, int parc, char *parv[])
{
	handle_stats(si->su, parv[0][0]);
}

static void m_admin(sourceinfo_t *si, int parc, char *parv[])
{
	handle_admin(si->su);
}

static void m_version(sourceinfo_t *si, int parc, char *parv[])
{
	handle_version(si->su);
}

static void m_info(sourceinfo_t *si, int parc, char *parv[])
{
	handle_info(si->su);
}

static void m_whois(sourceinfo_t *si, int parc, char *parv[])
{
	handle_whois(si->su, parv[1]);
}

static void m_trace(sourceinfo_t *si, int parc, char *parv[])
{
	handle_trace(si->su, parv[0], parc >= 2 ? parv[1] : NULL);
}

static void m_away(sourceinfo_t *si, int parc, char *parv[])
{
	handle_away(si->su, parc >= 1 ? parv[0] : NULL);
}

static void m_join(sourceinfo_t *si, int parc, char *parv[])
{
	chanuser_t *cu;
	mowgli_node_t *n, *tn;

	/* JOIN 0 is really a part from all channels */
	if (parv[0][0] == '0')
	{
		MOWGLI_ITER_FOREACH_SAFE(n, tn, si->su->channels.head)
		{
			cu = (chanuser_t *)n->data;
			chanuser_delete(cu->chan, si->su);
		}
	}
}

static void m_pass(sourceinfo_t *si, int parc, char *parv[])
{
	if (strcmp(curr_uplink->pass, parv[0]))
	{
		slog(LG_INFO, "m_pass(): password mismatch from uplink; aborting");
		runflags |= RF_SHUTDOWN;
	}
}

static void m_error(sourceinfo_t *si, int parc, char *parv[])
{
	slog(LG_INFO, "m_error(): error from server: %s", parv[0]);
}

static void m_sethost(sourceinfo_t *si, int parc, char *parv[])
{
	strlcpy(si->su->vhost, parv[0], HOSTLEN);
}

static void m_chghost(sourceinfo_t *si, int parc, char *parv[])
{
	user_t *u = user_find(parv[0]);

	if (!u)
		return;

	strlcpy(u->vhost, parv[1], HOSTLEN);
}

static void m_motd(sourceinfo_t *si, int parc, char *parv[])
{
	handle_motd(si->su);
}

static void nick_group(hook_user_req_t *hdata)
{
	user_t *u;

	u = hdata->si->su != NULL && !irccasecmp(hdata->si->su->nick, hdata->mn->nick) ? hdata->si->su : user_find_named(hdata->mn->nick);
	if (!use_esvid && u != NULL && should_reg_umode(u))
		sts(":%s SVS2MODE %s +rd %lu", nicksvs.nick, u->nick,
				(unsigned long)u->ts);
}

static void nick_ungroup(hook_user_req_t *hdata)
{
	user_t *u;

	u = hdata->si->su != NULL && !irccasecmp(hdata->si->su->nick, hdata->mn->nick) ? hdata->si->su : user_find_named(hdata->mn->nick);
	if (u != NULL && !nicksvs.no_nick_ownership && !use_esvid)
		sts(":%s SVS2MODE %s -r+d *", nicksvs.nick, u->nick);
}

static void m_protoctl(sourceinfo_t *si, int parc, char *parv[])
{
	int i;

	for (i = 0; i < parc; i++)
	{
		if (!irccasecmp(parv[i], "ESVID"))
			use_esvid = true;
	}
}

void _modinit(module_t * m)
{
	/* Symbol relocation voodoo. */
	server_login = &unreal_server_login;
	introduce_nick = &unreal_introduce_nick;
	quit_sts = &unreal_quit_sts;
	wallops_sts = &unreal_wallops_sts;
	join_sts = &unreal_join_sts;
	kick = &unreal_kick;
	msg = &unreal_msg;
	msg_global_sts = &unreal_msg_global_sts;
	notice_user_sts = &unreal_notice_user_sts;
	notice_global_sts = &unreal_notice_global_sts;
	notice_channel_sts = &unreal_notice_channel_sts;
	numeric_sts = &unreal_numeric_sts;
	kill_id_sts = &unreal_kill_id_sts;
	part_sts = &unreal_part_sts;
	kline_sts = &unreal_kline_sts;
	unkline_sts = &unreal_unkline_sts;
	xline_sts = &unreal_xline_sts;
	unxline_sts = &unreal_unxline_sts;
	qline_sts = &unreal_qline_sts;
	unqline_sts = &unreal_unqline_sts;
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
	chan_lowerts = &unreal_chan_lowerts;

	mode_list = unreal_mode_list;
	ignore_mode_list = unreal_ignore_mode_list;
	status_mode_list = unreal_status_mode_list;
	prefix_mode_list = unreal_prefix_mode_list;
	user_mode_list = unreal_user_mode_list;
	ignore_mode_list_size = ARRAY_SIZE(unreal_ignore_mode_list);

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
	pcommand_add("AWAY", m_away, 0, MSRC_USER);
	pcommand_add("JOIN", m_join, 1, MSRC_USER);
	pcommand_add("PASS", m_pass, 1, MSRC_UNREG);
	pcommand_add("ERROR", m_error, 1, MSRC_UNREG | MSRC_SERVER);
	pcommand_add("TOPIC", m_topic, 4, MSRC_USER | MSRC_SERVER);
	pcommand_add("SETHOST", m_sethost, 1, MSRC_USER);
	pcommand_add("CHGHOST", m_chghost, 2, MSRC_USER | MSRC_SERVER);
	pcommand_add("MOTD", m_motd, 1, MSRC_USER);
	pcommand_add("PROTOCTL", m_protoctl, 10, MSRC_UNREG);

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
	pcommand_add("#", m_whois, 2, MSRC_USER);
	pcommand_add("b", m_trace, 1, MSRC_USER);
	pcommand_add("6", m_away, 0, MSRC_USER);
	pcommand_add("C", m_join, 1, MSRC_USER);
	pcommand_add("<", m_pass, 1, MSRC_UNREG);
	pcommand_add("5", m_error, 1, MSRC_UNREG | MSRC_SERVER);
	pcommand_add(")", m_topic, 4, MSRC_USER | MSRC_SERVER);
	pcommand_add("AA", m_sethost, 1, MSRC_USER);
	pcommand_add("AL", m_chghost, 2, MSRC_USER | MSRC_SERVER);
	pcommand_add("F", m_motd, 1, MSRC_USER);
	pcommand_add("_", m_protoctl, 10, MSRC_UNREG);

	hook_add_event("nick_group");
	hook_add_nick_group(nick_group);
	hook_add_event("nick_ungroup");
	hook_add_nick_ungroup(nick_ungroup);

	m->mflags = MODTYPE_CORE;

	pmodule_loaded = true;
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
