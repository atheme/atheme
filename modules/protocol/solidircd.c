/*
 * Copyright (c) 2005-2006 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains protocol support for solidircd.
 *
 */

#include "atheme.h"
#include "uplink.h"
#include "pmodule.h"
#include "protocol/solidircd.h"

DECLARE_MODULE_V1("protocol/solidircd", true, _modinit, NULL, PACKAGE_STRING, "Atheme Development Group <http://www.atheme.org>");

/* *INDENT-OFF* */

ircd_t Solidircd = {
        "solid-ircd 3.4.x",             /* IRCd name */
        "$",                            /* TLD Prefix, used by Global. */
        false,                          /* Whether or not we use IRCNet/TS6 UID */
        false,                          /* Whether or not we use RCOMMAND */
        false,                          /* Whether or not we support channel owners. */
        false,                          /* Whether or not we support channel protection. */
        true,                           /* Whether or not we support halfops. */
	false,				/* Whether or not we use P10 */
	false,				/* Whether or not we use vHosts. */
	CMODE_OPERONLY,			/* Oper-only cmodes */
        0,                              /* Integer flag for owner channel flag. */
        0,                              /* Integer flag for protect channel flag. */
	CSTATUS_HALFOP,                   /* Integer flag for halfops. */
        "+",                            /* Mode we set for owner. */
        "+",                            /* Mode we set for protect. */
        "+h",                           /* Mode we set for halfops. */
	PROTOCOL_SOLIDIRCD,		/* Protocol type */
	0,                              /* Permanent cmodes */
	0,                              /* Oper-immune cmode */
	"beI",                          /* Ban-like cmodes */
	'e',                            /* Except mchar */
	'I',                            /* Invex mchar */
	IRCD_HOLDNICK                   /* Flags */
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

static bool check_jointhrottle(const char *, channel_t *, mychan_t *, user_t *, myuser_t *);

struct extmode solidircd_ignore_mode_list[] = {
  { 'j', check_jointhrottle },
  { '\0', 0 }
};

struct cmode_ solidircd_status_mode_list[] = {
  { 'o', CSTATUS_OP    },
  { 'h', CSTATUS_HALFOP },
  { 'v', CSTATUS_VOICE },
  { '\0', 0 }
};

struct cmode_ solidircd_prefix_mode_list[] = {
  { '@', CSTATUS_OP    },
  { '%', CSTATUS_HALFOP  },
  { '+', CSTATUS_VOICE },
  { '\0', 0 }
};

struct cmode_ solidircd_user_mode_list[] = {
  { 'A', UF_ADMIN    },
  { 'i', UF_INVIS    },
  { 'o', UF_IRCOP    },
  { '\0', 0 }
};

/* *INDENT-ON* */

static bool check_jointhrottle(const char *value, channel_t *c, mychan_t *mc, user_t *u, myuser_t *mu)
{
	const char *p, *arg2;
	int num, timeslice, v;

	if (!strcmp(value, "0") && u == NULL && mu == NULL)
		return true;
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
	if (p - arg2 > 3 || arg2 - value - 1 > 3)
		return false;
	num = atoi(value);
	timeslice = atoi(arg2);
	if (num <= 0 || num > 127 || timeslice <= 0 || timeslice > 127)
		return false;
	if (u != NULL || mu != NULL)
	{
		/* the following are the same restrictions bahamut
		 * applies to local clients
		 */
		if (num < 2 || num > 20 || timeslice > 60)
			return false;
		v = (timeslice - 1) / 8 + 1;
		if (num < v)
			return false;
		v = num / 2;
		if (timeslice < v)
			return false;
	}
	return true;
}

/* login to our uplink */
static unsigned int solidircd_server_login(void)
{
	int ret;

	ret = sts("PASS %s :TS", curr_uplink->pass);
	if (ret == 1)
		return 1;

	me.bursting = true;

	sts("CAPAB SSJOIN NOQUIT BURST ZIP NICKIP TSMODE");
	sts("SERVER %s 1 :%s", me.name, me.desc);
	sts("SVINFO 5 3 0 :%lu", (unsigned long)CURRTIME);

	services_init();

	return 0;
}

/* introduce a client */
static void solidircd_introduce_nick(user_t *u)
{
	const char *umode = user_get_umodestr(u);

	sts("NICK %s 1 %lu %s %s %s %s 0 0 :%s", u->nick, (unsigned long)u->ts, umode, u->user, u->host, me.name, u->gecos);
}

/* invite a user to a channel */
static void solidircd_invite_sts(user_t *sender, user_t *target, channel_t *channel)
{
	sts(":%s INVITE %s %s", sender->nick, target->nick, channel->name);
}

static void solidircd_quit_sts(user_t *u, const char *reason)
{
	if (!me.connected)
		return;

	sts(":%s QUIT :%s", u->nick, reason);
}

/* WALLOPS wrapper */
static void solidircd_wallops_sts(const char *text)
{
	sts(":%s GLOBOPS :%s", me.name, text);
}

/* join a channel */
static void solidircd_join_sts(channel_t *c, user_t *u, bool isnew, char *modes)
{
	if (isnew)
		sts(":%s SJOIN %lu %s %s :@%s", me.name, (unsigned long)c->ts,
				c->name, modes, u->nick);
	else
		sts(":%s SJOIN %lu %s + :@%s", me.name, (unsigned long)c->ts,
				c->name, u->nick);
}

static void solidircd_chan_lowerts(channel_t *c, user_t *u)
{
	slog(LG_DEBUG, "solidircd_chan_lowerts(): lowering TS for %s to %lu",
			c->name, (unsigned long)c->ts);
	sts(":%s SJOIN %lu %s %s :@%s", me.name, (unsigned long)c->ts, c->name,
				channel_modes(c, true), u->nick);
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
static void solidircd_kick(user_t *source, channel_t *c, user_t *u, const char *reason)
{
	sts(":%s KICK %s %s :%s", source->nick, c->name, u->nick, reason);

	chanuser_delete(c, u);
}

/* PRIVMSG wrapper */
static void solidircd_msg(const char *from, const char *target, const char *fmt, ...)
{
	va_list ap;
	char buf[BUFSIZE];

	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZE, fmt, ap);
	va_end(ap);

	sts(":%s PRIVMSG %s :%s", from, target, buf);
}

static void solidircd_msg_global_sts(user_t *from, const char *mask, const char *text)
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
static void solidircd_notice_user_sts(user_t *from, user_t *target, const char *text)
{
	sts(":%s NOTICE %s :%s", from ? from->nick : me.name, target->nick, text);
}

static void solidircd_notice_global_sts(user_t *from, const char *mask, const char *text)
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

static void solidircd_notice_channel_sts(user_t *from, channel_t *target, const char *text)
{
	sts(":%s NOTICE %s :%s", from ? from->nick : me.name, target->name, text);
}

static void solidircd_wallchops(user_t *sender, channel_t *channel, const char *message)
{
	sts(":%s NOTICE @%s :%s", sender->nick, channel->name, message);
}

static void solidircd_numeric_sts(server_t *from, int numeric, user_t *target, const char *fmt, ...)
{
	va_list ap;
	char buf[BUFSIZE];

	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZE, fmt, ap);
	va_end(ap);

	sts(":%s %d %s %s", from->name, numeric, target->nick, buf);
}

/* KILL wrapper */
static void solidircd_kill_id_sts(user_t *killer, const char *id, const char *reason)
{
	if (killer != NULL)
		sts(":%s KILL %s :%s!%s (%s)", killer->nick, id, killer->host, killer->nick, reason);
	else
		sts(":%s KILL %s :%s (%s)", me.name, id, me.name, reason);
}

/* PART wrapper */
static void solidircd_part_sts(channel_t *c, user_t *u)
{
	sts(":%s PART %s", u->nick, c->name);
}

/* server-to-server KLINE wrapper */
static void solidircd_kline_sts(const char *server, const char *user, const char *host, long duration, const char *reason)
{
	service_t *svs;

	if (!me.connected)
		return;

	svs = service_find("operserv");
	sts(":%s AKILL %s %s %ld %s %lu :%s", me.name, host, user, duration, svs != NULL ? svs->nick : me.name, (unsigned long)CURRTIME, reason);
}

/* server-to-server UNKLINE wrapper */
static void solidircd_unkline_sts(const char *server, const char *user, const char *host)
{
	if (!me.connected)
		return;

	sts(":%s RAKILL %s %s", me.name, host, user);
}

/* topic wrapper */
static void solidircd_topic_sts(channel_t *c, user_t *source, const char *setter, time_t ts, time_t prevts, const char *topic)
{
	if (!me.connected || !c)
		return;

	sts(":%s TOPIC %s %s %lu :%s", source->nick, c->name, setter, (unsigned long)ts, topic);
}

/* mode wrapper */
static void solidircd_mode_sts(char *sender, channel_t *target, char *modes)
{
	if (!me.connected)
		return;

	sts(":%s MODE %s %lu %s", sender, target->name, (unsigned long)target->ts, modes);
}

/* ping wrapper */
static void solidircd_ping_sts(void)
{
	if (!me.connected)
		return;

	sts("PING :%s", me.name);
}

/* protocol-specific stuff to do on login */
static void solidircd_on_login(user_t *u, myuser_t *account, const char *wantedhost)
{
	if (!me.connected || u == NULL)
		return;

	/* Can only do this for nickserv, and can only record identified
	 * state if logged in to correct nick, sorry -- jilles
	 */
	if (should_reg_umode(u))
		sts(":%s SVSMODE %s +rd %lu", nicksvs.nick, u->nick, (unsigned long)CURRTIME);
}

/* protocol-specific stuff to do on login */
static bool solidircd_on_logout(user_t *u, const char *account)
{
	if (!me.connected || u == NULL)
		return false;

	if (!nicksvs.no_nick_ownership)
		sts(":%s SVSMODE %s -r+d %lu", nicksvs.nick, u->nick, (unsigned long)CURRTIME);
	return false;
}

static void solidircd_jupe(const char *server, const char *reason)
{
	server_t *s;
	service_t *svs;

	if (!me.connected)
		return;

	svs = service_find("operserv");

	sts(":%s SQUIT %s :%s", svs != NULL ? svs->nick : me.name, server, reason);
	s = server_find(server);
	/* If the server is not directly connected to our uplink, we
	 * need to wait for its uplink to process the SQUIT :(
	 * -- jilles */
	if (s != NULL && s->uplink != NULL && s->uplink->uplink != me.me)
		s->flags |= SF_JUPE_PENDING;
	else
		sts(":%s SERVER %s 2 :%s", me.name, server, reason);
}


static void solidircd_sethost_sts(user_t *source, user_t *target, const char *host)
{
	if (!me.connected)
		return;

	if (irccasecmp(target->host, host))
		numeric_sts(me.me, 396, target, "%s :is now your hidden host (set by %s)", host, source->nick);
	else
		numeric_sts(me.me, 396, target, "%s :hostname reset by %s", host, source->nick);
	sts(":%s SVSMODE %s +v", source->nick, target->nick);
	sts(":%s SVHOST %s :%s", me.name, target->nick, host);
}

static void solidircd_fnc_sts(user_t *source, user_t *u, char *newnick, int type)
{
	sts(":%s SVSNICK %s %s %lu", source->nick, u->nick, newnick,
			(unsigned long)(CURRTIME - 60));
}

static void solidircd_holdnick_sts(user_t *source, int duration, const char *nick, myuser_t *mu)
{
	sts(":%s SVSHOLD %s %d :Reserved by %s for nickname owner (%s)",
			source->nick, nick, duration, source->nick,
			mu ? entity(mu)->name : nick);
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
	/* Postpone EOB for our uplink until topic burst is also done */
	if (s->uplink != me.me)
		handle_eob(s);

	if (irccasecmp(me.actual, parv[0]))
		return;

	me.uplinkpong = CURRTIME;

	/* -> :test.projectxero.net PONG test.projectxero.net :shrike.malkier.net */
}

static void m_burst(sourceinfo_t *si, int parc, char *parv[])
{
	server_t *s;

	/* Ignore "BURST" at start of burst */
	if (parc != 1)
		return;

	s = server_find(me.actual);
	if (s != NULL)
		handle_eob(s);

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

static void m_sjoin(sourceinfo_t *si, int parc, char *parv[])
{
	/*
	 *  -> :proteus.malkier.net SJOIN 1073516550 #shrike +tn :@sycobuny @+rakaur
	 *      also:
	 *  -> :nenolod_ SJOIN 1117334567 #chat
	 */

	channel_t *c;
	bool keep_new_modes = true;
	unsigned int userc;
	char *userv[256];
	unsigned int i;
	time_t ts;
	char *p;

	if (parc >= 4 && si->s != NULL)
	{
		/* :origin SJOIN ts chan modestr [key or limits] :users */
		c = channel_find(parv[1]);
		ts = atol(parv[0]);

		if (!c)
		{
			slog(LG_DEBUG, "m_sjoin(): new channel: %s", parv[1]);
			c = channel_add(parv[1], ts, si->s);
		}

		if (ts == 0 || c->ts == 0)
		{
			if (c->ts != 0)
				slog(LG_INFO, "m_sjoin(): server %s changing TS on %s from %lu to 0", si->s->name, c->name, (unsigned long)c->ts);
			c->ts = 0;
			hook_call_channel_tschange(c);
		}
		else if (ts < c->ts)
		{
			chanuser_t *cu;
			mowgli_node_t *n;

			/* the TS changed.  a TS change requires the following things
			 * to be done to the channel:  reset all modes to nothing, remove
			 * all status modes on known users on the channel (including ours),
			 * and set the new TS.
			 * also clear all bans and the topic
			 */

			clear_simple_modes(c);
			chanban_clear(c);
			handle_topic_from(si, c, "", 0, "");

			MOWGLI_ITER_FOREACH(n, c->members.head)
			{
				cu = (chanuser_t *)n->data;
				if (cu->user->server == me.me)
				{
					/* it's a service, reop */
					sts(":%s PART %s :Reop", cu->user->nick, c->name);
					sts(":%s SJOIN %lu %s + :@%s", me.name, (unsigned long)ts, c->name, cu->user->nick);
					cu->modes = CSTATUS_OP;
				}
				else
					cu->modes = 0;
			}

			slog(LG_DEBUG, "m_sjoin(): TS changed for %s (%lu -> %lu)", c->name, (unsigned long)c->ts, (unsigned long)ts);

			c->ts = ts;
			hook_call_channel_tschange(c);
		}
		else if (ts > c->ts)
			keep_new_modes = false;

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

		if (c->nummembers == 0 && !(c->modes & ircd->perm_mode))
			channel_delete(c);
	}
	else if (parc >= 2 && si->su != NULL)
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

		chanuser_add(c, CLIENT_NAME(si->su));
	}
	else
	{
		slog(LG_DEBUG, "m_sjoin(): invalid source/parameters: origin %s parc %d",
				si->su != NULL ? si->su->nick : (si->s != NULL ? si->s->name : "<none>"), parc);
	}
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
	struct in_addr ip;
	char ipstring[64];
	bool realchange;

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
		if (u == NULL)
			return;

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
		if (realchange && should_reg_umode(si->su))
			/* changed nick to registered one, reset +r */
			sts(":%s SVSMODE %s +rd %lu", nicksvs.nick, parv[0], (unsigned long)CURRTIME);

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

static void m_mode(sourceinfo_t *si, int parc, char *parv[])
{
	channel_t *c;

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
		slog(LG_DEBUG, "m_kick(): i got kicked from `%s'; rejoining", parv[0]);
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

	slog(LG_DEBUG, "m_server(): new server: %s", parv[0]);
	s = handle_server(si, parv[0], NULL, atoi(parv[1]), parv[2]);

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

static void m_motd(sourceinfo_t *si, int parc, char *parv[])
{
	handle_motd(si->su);
}

static void m_svhost(sourceinfo_t *si, int parc, char *parv[])
{
	user_t *u;

	u = user_find(parv[0]);
	if (u == NULL)
		return;
	strlcpy(u->vhost, parv[1], HOSTLEN);
}

static void nick_group(hook_user_req_t *hdata)
{
	user_t *u;

	u = hdata->si->su != NULL && !irccasecmp(hdata->si->su->nick, hdata->mn->nick) ? hdata->si->su : user_find_named(hdata->mn->nick);
	if (u != NULL && should_reg_umode(u))
		sts(":%s SVSMODE %s +rd %lu", nicksvs.nick, u->nick, (unsigned long)CURRTIME);
}

static void nick_ungroup(hook_user_req_t *hdata)
{
	user_t *u;

	u = hdata->si->su != NULL && !irccasecmp(hdata->si->su->nick, hdata->mn->nick) ? hdata->si->su : user_find_named(hdata->mn->nick);
	if (u != NULL && !nicksvs.no_nick_ownership)
		sts(":%s SVSMODE %s -r+d %lu", nicksvs.nick, u->nick, (unsigned long)CURRTIME);
}

void _modinit(module_t * m)
{
	/* Symbol relocation voodoo. */
	server_login = &solidircd_server_login;
	introduce_nick = &solidircd_introduce_nick;
	quit_sts = &solidircd_quit_sts;
	wallops_sts = &solidircd_wallops_sts;
	join_sts = &solidircd_join_sts;
	chan_lowerts = &solidircd_chan_lowerts;
	kick = &solidircd_kick;
	msg = &solidircd_msg;
	msg_global_sts = &solidircd_msg_global_sts;
	notice_user_sts = &solidircd_notice_user_sts;
	notice_global_sts = &solidircd_notice_global_sts;
	notice_channel_sts = &solidircd_notice_channel_sts;
	wallchops = &solidircd_wallchops;
	numeric_sts = &solidircd_numeric_sts;
	kill_id_sts = &solidircd_kill_id_sts;
	part_sts = &solidircd_part_sts;
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
	holdnick_sts = &solidircd_holdnick_sts;

	mode_list = solidircd_mode_list;
	ignore_mode_list = solidircd_ignore_mode_list;
	status_mode_list = solidircd_status_mode_list;
	prefix_mode_list = solidircd_prefix_mode_list;
	user_mode_list = solidircd_user_mode_list;
	ignore_mode_list_size = ARRAY_SIZE(solidircd_ignore_mode_list);

	ircd = &Solidircd;

	pcommand_add("PING", m_ping, 1, MSRC_USER | MSRC_SERVER);
	pcommand_add("PONG", m_pong, 1, MSRC_SERVER);
	pcommand_add("PRIVMSG", m_privmsg, 2, MSRC_USER);
	pcommand_add("NOTICE", m_notice, 2, MSRC_UNREG | MSRC_USER | MSRC_SERVER);
	pcommand_add("SJOIN", m_sjoin, 2, MSRC_USER | MSRC_SERVER);
	pcommand_add("PART", m_part, 1, MSRC_USER);
	pcommand_add("NICK", m_nick, 2, MSRC_USER | MSRC_SERVER);
	pcommand_add("QUIT", m_quit, 1, MSRC_USER);
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
	pcommand_add("MOTD", m_motd, 1, MSRC_USER);
	pcommand_add("BURST", m_burst, 0, MSRC_SERVER);
	pcommand_add("SVHOST", m_svhost, 2, MSRC_USER | MSRC_SERVER);

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
