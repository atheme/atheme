/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Copyright (c) 2005-2006 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains protocol support for ptlink ircd.
 *
 */

#include "atheme.h"
#include "uplink.h"
#include "pmodule.h"
#include "protocol/ptlink.h"

DECLARE_MODULE_V1("protocol/ptlink", true, _modinit, NULL, PACKAGE_STRING, "Atheme Development Group <http://www.atheme.org>");

/* *INDENT-OFF* */

ircd_t PTLink = {
        "PTLink IRCd",			/* IRCd name */
        "$",                            /* TLD Prefix, used by Global. */
        false,                          /* Whether or not we use IRCNet/TS6 UID */
        false,                          /* Whether or not we use RCOMMAND */
        false,                          /* Whether or not we support channel owners. */
        true,                           /* Whether or not we support channel protection. */
        true,                           /* Whether or not we support halfops. */
	false,				/* Whether or not we use P10 */
	true,				/* Whether or not we use vHosts. */
	0,				/* Oper-only cmodes */
        0,                              /* Integer flag for owner channel flag. */
        CSTATUS_PROTECT,                /* Integer flag for protect channel flag. */
        CSTATUS_HALFOP,                 /* Integer flag for halfops. */
        "+",                            /* Mode we set for owner. */
        "+a",                           /* Mode we set for protect. */
        "+h",                           /* Mode we set for halfops. */
	PROTOCOL_PTLINK,		/* Protocol type */
	0,                              /* Permanent cmodes */
	0,                              /* Oper-immune cmode */
	"b",                            /* Ban-like cmodes */
	0,                              /* Except mchar */
	0,                              /* Invex mchar */
	0                               /* Flags */
};

struct cmode_ ptlink_mode_list[] = {
  { 'i', CMODE_INVITE   },
  { 'm', CMODE_MOD      },
  { 'n', CMODE_NOEXT    },
  { 'p', CMODE_PRIV     },
  { 's', CMODE_SEC      },
  { 't', CMODE_TOPIC    },
  { 'c', CMODE_NOCOLOR  },
  { 'q', CMODE_NOQUIT   },
  { 'd', CMODE_NOREPEAT },
  { 'B', CMODE_NOBOTS   },
  { 'C', CMODE_SSLONLY  },
  { 'K', CMODE_KNOCK    },
  { 'N', CMODE_STICKY   },
  { 'R', CMODE_REGONLY  },
  { 'S', CMODE_NOSPAM   },
  { '\0', 0 }
};

static bool check_flood(const char *, channel_t *, mychan_t *, user_t *, myuser_t *);

struct extmode ptlink_ignore_mode_list[] = {
  { 'f', check_flood  },
  { '\0', 0 }
};

struct cmode_ ptlink_status_mode_list[] = {
  { 'a', CSTATUS_PROTECT},
  { 'o', CSTATUS_OP    },
  { 'h', CSTATUS_HALFOP},
  { 'v', CSTATUS_VOICE },
  { '\0', 0 }
};

struct cmode_ ptlink_prefix_mode_list[] = {
  { '.', CSTATUS_PROTECT},
  { '@', CSTATUS_OP    },
  { '%', CSTATUS_HALFOP},
  { '+', CSTATUS_VOICE },
  { '\0', 0 }
};

struct cmode_ ptlink_user_mode_list[] = {
  { 'A', UF_ADMIN    },
  { 'i', UF_INVIS    },
  { 'o', UF_IRCOP    },
  { '\0', 0 }
};

/* *INDENT-ON* */

static bool check_flood(const char *value, channel_t *c, mychan_t *mc, user_t *u, myuser_t *mu)
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
	if (p - arg2 > 2 || arg2 - value - 1 > 2 || atoi(value) <= 1 || !atoi(arg2))
		return false;
	return true;
}

/* login to our uplink */
static unsigned int ptlink_server_login(void)
{
	int ret;

	ret = sts("PASS %s :TS", curr_uplink->pass);
	if (ret == 1)
		return 1;

	me.bursting = true;

	sts("CAPAB :QS PTS4");
	sts("SERVER %s 1 Atheme-%s :%s", me.name, version, me.desc);
	sts("SVINFO 10 3 0 :%lu", (unsigned long)CURRTIME);

	services_init();

	return 0;
}

/* introduce a client */
static void ptlink_introduce_nick(user_t *u)
{
	const char *umode = user_get_umodestr(u);

	sts("NICK %s 1 %lu %sp %s %s %s %s :%s", u->nick, (unsigned long)u->ts, umode, u->user, u->host, u->host, me.name, u->gecos);
}

/* invite a user to a channel */
static void ptlink_invite_sts(user_t *sender, user_t *target, channel_t *channel)
{
	sts(":%s INVITE %s %s", sender->nick, target->nick, channel->name);
}

static void ptlink_quit_sts(user_t *u, const char *reason)
{
	if (!me.connected)
		return;

	sts(":%s QUIT :%s", u->nick, reason);
}

/* WALLOPS wrapper */
static void ptlink_wallops_sts(const char *text)
{
	sts(":%s WALLOPS :%s", me.name, text);
}

/* join a channel */
static void ptlink_join_sts(channel_t *c, user_t *u, bool isnew, char *modes)
{
	if (isnew)
		sts(":%s SJOIN %lu %s %s :@%s", me.name, (unsigned long)c->ts,
				c->name, modes, u->nick);
	else
		sts(":%s SJOIN %lu %s + :@%s", me.name, (unsigned long)c->ts,
				c->name, u->nick);
}

/* kicks a user from a channel */
static void ptlink_kick(user_t *source, channel_t *c, user_t *u, const char *reason)
{
	sts(":%s KICK %s %s :%s", source->nick, c->name, u->nick, reason);

	chanuser_delete(c, u);
}

/* PRIVMSG wrapper */
static void ptlink_msg(const char *from, const char *target, const char *fmt, ...)
{
	va_list ap;
	char buf[BUFSIZE];

	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZE, fmt, ap);
	va_end(ap);

	sts(":%s PRIVMSG %s :%s", from, target, buf);
}

static void ptlink_msg_global_sts(user_t *from, const char *mask, const char *text)
{
	sts(":%s PRIVMSG %s%s :%s", from ? from->nick : me.name, ircd->tldprefix, mask, text);
}

/* NOTICE wrapper */
static void ptlink_notice_user_sts(user_t *from, user_t *target, const char *text)
{
	sts(":%s NOTICE %s :%s", from ? from->nick : me.name, target->nick, text);
}

static void ptlink_notice_global_sts(user_t *from, const char *mask, const char *text)
{
	sts(":%s NOTICE %s%s :%s", from ? from->nick : me.name, ircd->tldprefix, mask, text);
}

static void ptlink_notice_channel_sts(user_t *from, channel_t *target, const char *text)
{
	sts(":%s NOTICE %s :%s", from ? from->nick : me.name, target->name, text);
}

/* numeric wrapper */
static void ptlink_numeric_sts(server_t *from, int numeric, user_t *target, const char *fmt, ...)
{
	va_list ap;
	char buf[BUFSIZE];

	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZE, fmt, ap);
	va_end(ap);

	sts(":%s %d %s %s", from->name, numeric, target->nick, buf);
}

/* KILL wrapper */
static void ptlink_kill_id_sts(user_t *killer, const char *id, const char *reason)
{
	if (killer != NULL)
		sts(":%s KILL %s :%s!%s (%s)", killer->nick, id, killer->host, killer->nick, reason);
	else
		sts(":%s KILL %s :%s (%s)", me.name, id, me.name, reason);
}

/* PART wrapper */
static void ptlink_part_sts(channel_t *c, user_t *u)
{
	sts(":%s PART %s", u->nick, c->name);
}

/* server-to-server KLINE wrapper */
static void ptlink_kline_sts(const char *server, const char *user, const char *host, long duration, const char *reason)
{
	service_t *svs;

	if (!me.connected)
		return;

	/* we don't know the real setter here :( */
	svs = service_find("operserv");
	sts(":%s GLINE %s@%s %ld %s :%s", svs != NULL ? svs->nick : me.name, user, host, duration, svs != NULL ? svs->nick : me.name, reason);
}

/* server-to-server UNKLINE wrapper */
static void ptlink_unkline_sts(const char *server, const char *user, const char *host)
{
	service_t *svs;

	if (!me.connected)
		return;

	svs = service_find("operserv");
	sts(":%s UNGLINE %s@%s", svs != NULL ? svs->nick : me.name, user, host);
}

/* topic wrapper */
static void ptlink_topic_sts(channel_t *c, user_t *source, const char *setter, time_t ts, time_t prevts, const char *topic)
{
	if (!me.connected || !c)
		return;

	sts(":%s TOPIC %s %s %lu :%s", source->nick, c->name, setter, (unsigned long)ts, topic);
}

/* mode wrapper */
static void ptlink_mode_sts(char *sender, channel_t *target, char *modes)
{
	if (!me.connected)
		return;

	sts(":%s MODE %s %s", sender, target->name, modes);
}

/* ping wrapper */
static void ptlink_ping_sts(void)
{
	if (!me.connected)
		return;

	sts("PING :%s", me.name);
}

/* protocol-specific stuff to do on login */
static void ptlink_on_login(user_t *u, myuser_t *account, const char *wantedhost)
{
	if (!me.connected || u == NULL)
		return;

	/* Can only do this for nickserv, and can only record identified
	 * state if logged in to correct nick, sorry -- jilles
	 */
	if (should_reg_umode(u))
		sts(":%s SVSMODE %s +r", me.name, u->nick);
}

/* protocol-specific stuff to do on login */
static bool ptlink_on_logout(user_t *u, const char *account)
{
	if (!me.connected)
		return false;

	if (!nicksvs.no_nick_ownership)
		sts(":%s SVSMODE %s -r", me.name, u->nick);

	return false;
}

static void ptlink_jupe(const char *server, const char *reason)
{
	service_t *svs;

	if (!me.connected)
		return;

	svs = service_find("operserv");

	server_delete(server);
	sts(":%s SQUIT %s :%s", svs != NULL ? svs->nick : me.name, server, reason);
	sts(":%s SERVER %s 2 Atheme-%s-jupe :%s", me.name, server, version, reason);
}

static void ptlink_sethost_sts(user_t *source, user_t *target, const char *host)
{
	sts(":%s NEWMASK %s :%s", me.name, host, target->nick);
}

static void ptlink_fnc_sts(user_t *source, user_t *u, char *newnick, int type)
{
	sts(":%s SVSNICK %s %lu %s", me.name, u->nick, (unsigned long)u->ts, newnick);
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

static void m_sjoin(sourceinfo_t *si, int parc, char *parv[])
{
	/* -> :proteus.malkier.net SJOIN 1073516550 #shrike +tn :@sycobuny @+rakaur */

	channel_t *c;
	unsigned int userc;
	char *userv[256];
	unsigned int i;
	time_t ts;

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
		chanuser_t *cu;
		mowgli_node_t *n;

		/* the TS changed.  a TS change requires the following things
		 * to be done to the channel:  reset all modes to nothing, remove
		 * all status modes on known users on the channel (including ours),
		 * and set the new TS.
		 */

		clear_simple_modes(c);

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

	channel_mode(NULL, c, parc - 3, parv + 2);

	userc = sjtoken(parv[parc - 1], ' ', userv);

	for (i = 0; i < userc; i++)
		chanuser_add(c, userv[i]);

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

	/* got the right number of args for an introduction? */
	if (parc == 9)
	{
		s = server_find(parv[7]);
		if (!s)
		{
			slog(LG_DEBUG, "m_nick(): new user on nonexistant server: %s", parv[6]);
			return;
		}

		slog(LG_DEBUG, "m_nick(): new user on `%s': %s", s->name, parv[0]);

		u = user_add(parv[0], parv[4], parv[5], parv[6], NULL, NULL, parv[8], s, atoi(parv[2]));
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
			sts(":%s SVSMODE %s +r", me.name, parv[0]);

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
	if (*parv[0] == '#')
		channel_mode(NULL, channel_find(parv[0]), parc - 1, &parv[1]);
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

static void m_newmask(sourceinfo_t *si, int parc, char *parv[])
{
	user_t *target;
	char *p;

	target = parc >= 2 ? user_find(parv[1]) : si->su;
	if (target == NULL)
		return;
	p = strchr(parv[0], '@');
	if (p != NULL)
	{
		strlcpy(target->vhost, p + 1, sizeof target->vhost);
		if ((size_t)(p - parv[0]) < sizeof target->user && p > parv[0])
		{
			memcpy(target->user, parv[0], p - parv[0]);
			target->user[p - parv[0]] = '\0';
		}
	}
	else
		strlcpy(target->vhost, parv[0], sizeof target->vhost);
	slog(LG_DEBUG, "m_newmask(): %s -> %s@%s",
			target->nick, target->user, target->vhost);
}

static void m_motd(sourceinfo_t *si, int parc, char *parv[])
{
	handle_motd(si->su);
}

static void nick_group(hook_user_req_t *hdata)
{
	user_t *u;

	u = hdata->si->su != NULL && !irccasecmp(hdata->si->su->nick, hdata->mn->nick) ? hdata->si->su : user_find_named(hdata->mn->nick);
	if (u != NULL && should_reg_umode(u))
		sts(":%s SVSMODE %s +r", me.name, u->nick);
}

static void nick_ungroup(hook_user_req_t *hdata)
{
	user_t *u;

	u = hdata->si->su != NULL && !irccasecmp(hdata->si->su->nick, hdata->mn->nick) ? hdata->si->su : user_find_named(hdata->mn->nick);
	if (u != NULL && !nicksvs.no_nick_ownership)
		sts(":%s SVSMODE %s -r", me.name, u->nick);
}

void _modinit(module_t * m)
{
	/* Symbol relocation voodoo. */
	server_login = &ptlink_server_login;
	introduce_nick = &ptlink_introduce_nick;
	quit_sts = &ptlink_quit_sts;
	wallops_sts = &ptlink_wallops_sts;
	join_sts = &ptlink_join_sts;
	kick = &ptlink_kick;
	msg = &ptlink_msg;
	msg_global_sts = &ptlink_msg_global_sts;
	notice_user_sts = &ptlink_notice_user_sts;
	notice_global_sts = &ptlink_notice_global_sts;
	notice_channel_sts = &ptlink_notice_channel_sts;
	numeric_sts = &ptlink_numeric_sts;
	kill_id_sts = &ptlink_kill_id_sts;
	part_sts = &ptlink_part_sts;
	kline_sts = &ptlink_kline_sts;
	unkline_sts = &ptlink_unkline_sts;
	topic_sts = &ptlink_topic_sts;
	mode_sts = &ptlink_mode_sts;
	ping_sts = &ptlink_ping_sts;
	ircd_on_login = &ptlink_on_login;
	ircd_on_logout = &ptlink_on_logout;
	jupe = &ptlink_jupe;
	invite_sts = &ptlink_invite_sts;
	sethost_sts = &ptlink_sethost_sts;
	fnc_sts = &ptlink_fnc_sts;

	mode_list = ptlink_mode_list;
	ignore_mode_list = ptlink_ignore_mode_list;
	status_mode_list = ptlink_status_mode_list;
	prefix_mode_list = ptlink_prefix_mode_list;
	user_mode_list = ptlink_user_mode_list;
	ignore_mode_list_size = ARRAY_SIZE(ptlink_ignore_mode_list);

	ircd = &PTLink;

	pcommand_add("PING", m_ping, 1, MSRC_USER | MSRC_SERVER);
	pcommand_add("PONG", m_pong, 1, MSRC_SERVER);
	pcommand_add("PRIVMSG", m_privmsg, 2, MSRC_USER);
	pcommand_add("NOTICE", m_notice, 2, MSRC_UNREG | MSRC_USER | MSRC_SERVER);
	pcommand_add("SJOIN", m_sjoin, 4, MSRC_SERVER);
	pcommand_add("NJOIN", m_sjoin, 2, MSRC_USER | MSRC_SERVER);
	pcommand_add("PART", m_part, 1, MSRC_USER);
	pcommand_add("NICK", m_nick, 2, MSRC_USER | MSRC_SERVER);
	pcommand_add("NNICK", m_nick, 2, MSRC_USER | MSRC_SERVER);
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
	pcommand_add("NEWMASK", m_newmask, 1, MSRC_USER | MSRC_SERVER);
	pcommand_add("MOTD", m_motd, 1, MSRC_USER);

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
