/*
 * Copyright (c) 2005-2007 William Pitcock, et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains protocol support for hyperion-based ircd.
 *
 * $Id: hyperion.c 8415 2007-06-06 22:34:07Z jilles $
 */

/* option: use SVSLOGIN/SIGNON to remember users even if they're
 * not logged in to their current nick, etc
 */
#define USE_SVSLOGIN

#include "atheme.h"
#include "uplink.h"
#include "pmodule.h"
#include "protocol/hyperion.h"

DECLARE_MODULE_V1("protocol/hyperion", TRUE, _modinit, NULL, "$Id: hyperion.c 8415 2007-06-06 22:34:07Z jilles $", "Atheme Development Group <http://www.atheme.org>");

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
	'I',                            /* Invex mchar */
	0                               /* Flags */
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

	if (*value != '#' || strlen(value) > 30)
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

static node_t *hyperion_next_matching_ban(channel_t *c, user_t *u, int type, node_t *first)
{
	chanban_t *cb;
	node_t *n;
	char hostbuf[NICKLEN+USERLEN+HOSTLEN];
	char realbuf[NICKLEN+USERLEN+HOSTLEN];
	char ipbuf[NICKLEN+USERLEN+HOSTLEN];
	char *ex2;

	snprintf(hostbuf, sizeof hostbuf, "%s!%s@%s", u->nick, u->user, u->vhost);
	snprintf(realbuf, sizeof realbuf, "%s!%s@%s", u->nick, u->user, u->host);
	/* will be nick!user@ if ip unknown, doesn't matter */
	snprintf(ipbuf, sizeof ipbuf, "%s!%s@%s", u->nick, u->user, u->ip);
	LIST_FOREACH(n, first)
	{
		cb = n->data;

		if (cb->type == type)
		{
			ex2 = NULL;
			if (type == 'b')
			{
				/* Look for forward channel and ignore it:
				 * nick!user@host!#channel
				 */
				ex2 = strchr(cb->mask, '!');
				if (ex2 != NULL)
					ex2 = strchr(ex2 + 1, '!');
			}
			if (ex2 != NULL)
				*ex2 = '\0';
			if (!match(cb->mask, hostbuf) || !match(cb->mask, realbuf) || !match(cb->mask, ipbuf))
			{
				if (ex2 != NULL)
					*ex2 = '!';
				return n;
			}
			if (ex2 != NULL)
				*ex2 = '!';
		}
		if (cb->type == 'd' && type == 'b' && !match(cb->mask, u->gecos))
			return n;
	}
	return NULL;
}

static boolean_t hyperion_is_valid_host(const char *host)
{
	const char *p;
	boolean_t dot = FALSE;

	if (*host == '.' || *host == '/' || *host == ':')
		return FALSE;

	for (p = host; *p != '\0'; p++)
	{
		if (*p == '.' || *p == ':' || *p == '/')
			dot = TRUE;
		else if (!((*p >= '0' && *p <= '9') || (*p >= 'A' && *p <= 'Z') ||
					(*p >= 'a' && *p <= 'z') || *p == '-'))
			return FALSE;
	}
	/* hyperion allows a trailing / but RichiH does not want it, whatever */
	if (dot && p[-1] == '/')
		return FALSE;
	return dot;
}

/* login to our uplink */
static unsigned int hyperion_server_login(void)
{
	int ret;

	ret = sts("PASS %s :TS", curr_uplink->pass);
	if (ret == 1)
		return 1;

	me.bursting = TRUE;

	sts("CAPAB :QS EX DE CHW IE QU DNCR SRV SIGNON");
	sts("SERVER %s 1 :%s", me.name, me.desc);
	sts("SVINFO 5 3 0 :%lu", (unsigned long)CURRTIME);

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
static void hyperion_introduce_nick(user_t *u)
{
	const char *privs = is_ircop(u) ? "6@BFmMopPRUX" : "";
	sts("NICK %s 1 %lu +ei%s %s %s %s 0.0.0.0 :%s", u->nick, (unsigned long)u->ts, privs, u->user, u->host, me.name, u->gecos);
	if (is_ircop(u))
		sts(":%s OPER %s +%s", me.name, u->nick, privs);
}

/* invite a user to a channel */
static void hyperion_invite_sts(user_t *sender, user_t *target, channel_t *channel)
{
	sts(":%s INVITE %s %s", sender->nick, target->nick, channel->name);
}

static void hyperion_quit_sts(user_t *u, const char *reason)
{
	if (!me.connected)
		return;

	sts(":%s QUIT :%s", u->nick, reason);
}

/* WALLOPS wrapper */
static void hyperion_wallops_sts(const char *text)
{
	/* Generate +s server notice -- jilles */
	sts(":%s WALLOPS 1-0 :%s", me.name, text);
}

/* join a channel */
static void hyperion_join_sts(channel_t *c, user_t *u, boolean_t isnew, char *modes)
{
	if (isnew)
		sts(":%s SJOIN %lu %s %s :@%s", me.name, (unsigned long)c->ts,
				c->name, modes, u->nick);
	else
		sts(":%s SJOIN %lu %s + :@%s", me.name, (unsigned long)c->ts,
				c->name, u->nick);
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
static void hyperion_msg(const char *from, const char *target, const char *fmt, ...)
{
	va_list ap;
	char buf[BUFSIZE];

	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZE, fmt, ap);
	va_end(ap);

	sts(":%s PRIVMSG %s :%s", from, target, buf);
}

/* NOTICE wrapper */
static void hyperion_notice_user_sts(user_t *from, user_t *target, const char *text)
{
	sts(":%s NOTICE %s :%s", from ? from->nick : me.name, target->nick, text);
}

static void hyperion_notice_global_sts(user_t *from, const char *mask, const char *text)
{
	sts(":%s NOTICE %s%s :%s", from ? from->nick : me.name, ircd->tldprefix, mask, text);
}

static void hyperion_notice_channel_sts(user_t *from, channel_t *target, const char *text)
{
	sts(":%s NOTICE %s :%s", from ? from->nick : me.name, target->name, text);
}

static void hyperion_wallchops(user_t *sender, channel_t *channel, const char *message)
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
static void hyperion_kill_id_sts(user_t *killer, const char *id, const char *reason)
{
#if 0
	if (killer != NULL)
		sts(":%s KILL %s :%s!%s (%s)", killer->nick, id, killer->host, killer->nick, reason);
	else
		sts(":%s KILL %s :%s (%s)", me.name, id, me.name, reason);
#else
	/* Use COLLIDE to cut down on server notices.
	 * The current version of hyperion does not do COLLIDE reasons,
	 * so fake it.
	 */
	sts(":%s NOTICE %s :*** Disconnecting you (%s (%s))", me.name,
			id, killer ? killer->nick : me.name, reason);
	sts(":%s COLLIDE %s :(%s)", me.name, id, reason);
#endif
}

/* PART wrapper */
static void hyperion_part_sts(channel_t *c, user_t *u)
{
	sts(":%s PART %s", u->nick, c->name);
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

	sts(":%s UNKLINE %s@%s %lu", opersvs.nick, user, host,
			(unsigned long)CURRTIME);
}

/* topic wrapper */
static void hyperion_topic_sts(channel_t *c, const char *setter, time_t ts, time_t prevts, const char *topic)
{
	if (!me.connected || !c)
		return;

	/* Send 0 channelts so this will always be accepted */
	sts(":%s STOPIC %s %s %lu 0 :%s", chansvs.nick, c->name, setter,
			(unsigned long)ts, topic);
}

/* mode wrapper */
static void hyperion_mode_sts(char *sender, channel_t *target, char *modes)
{
	if (!me.connected)
		return;

	sts(":%s MODE %s %s", sender, target->name, modes);
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
	metadata_t *md;

	if (!me.connected)
		return;

	u = user_find(origin);
	if (!u)
		return;
	if (use_svslogin)
	{
		/* XXX needed to avoid race */
		if (!wantedhost && u->myuser)
		{
			md = metadata_find(u->myuser, METADATA_USER, "private:usercloak");
			if (md)
				wantedhost = md->value;
		}
		if (wantedhost && strlen(wantedhost) >= HOSTLEN)
			wantedhost = NULL;
		sts(":%s SVSLOGIN %s %s %s %s %s %s", me.name, u->server->name, origin, user, origin, u->user, wantedhost ? wantedhost : u->vhost);
		/* we'll get a SIGNON confirming the changes later, no need
		 * to change the fields yet */
		/* XXX try to avoid a redundant SETHOST */
		if (wantedhost)
			strlcpy(u->vhost, wantedhost, HOSTLEN);
	}

	/* set +e if they're identified to the nick they are using */
	if (should_reg_umode(u))
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

	if (!nicksvs.no_nick_ownership)
		sts(":%s MODE %s -e", me.name, origin);

	return FALSE;
}

static void hyperion_jupe(const char *server, const char *reason)
{
	if (!me.connected)
		return;

	server_delete(server);
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

static void m_topic(sourceinfo_t *si, int parc, char *parv[])
{
	channel_t *c = channel_find(parv[0]);

	if (!c || !si->su)
		return;

	handle_topic_from(si, c, si->su->nick, CURRTIME, parv[1]);
}

static void m_stopic(sourceinfo_t *si, int parc, char *parv[])
{
	channel_t *c = channel_find(parv[0]);
	time_t channelts;
	time_t topicts;

	if (c == NULL)
		return;

	/* Our uplink is trying to change the topic during burst,
	 * and we have already set a topic. Assume our change won.
	 * -- jilles */
	if (si->s != NULL && si->s->uplink == me.me &&
			!(si->s->flags & SF_EOB) && c->topic != NULL)
		return;

	/* hyperion will propagate an STOPIC even if it's not applied
	 * locally :( */
	topicts = atol(parv[2]);
	channelts = atol(parv[3]);
	if (c->topic == NULL || ((strcmp(c->topic, parv[1]) && channelts < c->ts) || (channelts == c->ts && topicts > c->topicts)))
		handle_topic_from(si, c, parv[1], topicts, parv[parc - 1]);
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

		wallops("Finished synching to network in %d %s.", (tv2ms(&burstime) > 1000) ? (tv2ms(&burstime) / 1000) : tv2ms(&burstime), (tv2ms(&burstime) > 1000) ? "s" : "ms");
#else
		slog(LG_INFO, "m_pong(): finished synching with uplink");
		wallops("Finished synching to network.");
#endif

		me.bursting = FALSE;
	}
}

static void m_privmsg(sourceinfo_t *si, int parc, char *parv[])
{
	if (parc != 2)
		return;

	handle_message(si, parv[0], FALSE, parv[1]);
}

static void m_notice(sourceinfo_t *si, int parc, char *parv[])
{
	if (parc != 2)
		return;

	handle_message(si, parv[0], TRUE, parv[1]);
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

		slog(LG_DEBUG, "m_sjoin(): TS changed for %s (%lu -> %lu)", c->name, (unsigned long)c->ts, (unsigned long)ts);

		c->ts = ts;
		hook_call_event("channel_tschange", c);
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
	boolean_t realchange;

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
		if (u == NULL)
			return;

		user_mode(u, parv[3]);

		/* umode +e: identified to current nick */
		/* As hyperion clears +e on nick changes, this is safe. */
		if (!use_svslogin && strchr(parv[3], 'e'))
			handle_burstlogin(u, NULL);

		/* if the server supports SIGNON, we will get an SNICK
		 * for this user, potentially with a login name
		 */
		if (!use_svslogin)
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

		/* fix up +e if necessary -- jilles */
		if (realchange && should_reg_umode(si->su))
			/* changed nick to registered one, reset +e */
			sts(":%s MODE %s +e", me.name, parv[0]);

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
	user_delete(si->su);
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
	/* serves for both KILL and COLLIDE
	 * COLLIDE only originates from servers and may not have
	 * a reason field, but the net effect is identical
	 * -- jilles */
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

static void m_pass(sourceinfo_t *si, int parc, char *parv[])
{
	if (strcmp(curr_uplink->pass, parv[0]))
	{
		slog(LG_INFO, "m_pass(): password mismatch from uplink; continuing anyway");
	}
}

static void m_error(sourceinfo_t *si, int parc, char *parv[])
{
	slog(LG_INFO, "m_error(): error from server: %s", parv[0]);
}

static void m_capab(sourceinfo_t *si, int parc, char *parv[])
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

static void m_snick(sourceinfo_t *si, int parc, char *parv[])
{
	user_t *u;

	/* SNICK <nick> <orignick> <spoofhost> <firsttime> <dnshost> <servlogin> */
	u = user_find(parv[0]);

	if (!u)
		return;

	if (strcmp(u->vhost, parv[2]))	/* User is not using spoofhost, assume no I:line spoof */
	{
		strlcpy(u->host, parv[4], HOSTLEN);
	}

	if (use_svslogin)
	{
		if (parc >= 6)
			if (strcmp(parv[5], "0"))
				handle_burstlogin(u, parv[5]);

		handle_nickchange(u);
	}
}

static void m_sethost(sourceinfo_t *si, int parc, char *parv[])
{
	user_t *u;

	/* SETHOST <nick> <newhost> */
	u = user_find(parv[0]);

	if (!u)
		return;

	strlcpy(u->vhost, parv[1], HOSTLEN);
}

static void m_setident(sourceinfo_t *si, int parc, char *parv[])
{
	user_t *u;

	/* SETHOST <nick> <newident> */
	u = user_find(parv[0]);

	if (!u)
		return;

	strlcpy(u->user, parv[1], USERLEN);
}

static void m_setname(sourceinfo_t *si, int parc, char *parv[])
{
	user_t *u;

	/* SETNAME <nick> <newreal> */
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
static void m_signon(sourceinfo_t *si, int parc, char *parv[])
{
	char *nick_parv[2];

	slog(LG_DEBUG, "m_signon(): signon %s -> %s!%s@%s (login %s)", si->su->nick, parv[1], parv[2], parv[3], parv[0]);

	strlcpy(si->su->user, parv[2], USERLEN);
	strlcpy(si->su->vhost, parv[3], HOSTLEN);
	nick_parv[0] = parv[1];
	nick_parv[1] = parv[4];
	if (strcmp(si->su->nick, parv[1]))
		m_nick(si, 2, nick_parv);
	/* don't use login id, assume everyone signs in via atheme */
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
		sts(":%s MODE %s +e", me.name, u->nick);
}

static void nick_ungroup(hook_user_req_t *hdata)
{
	user_t *u;

	u = hdata->si->su != NULL && !irccasecmp(hdata->si->su->nick, hdata->mn->nick) ? hdata->si->su : user_find_named(hdata->mn->nick);
	if (u != NULL && !nicksvs.no_nick_ownership)
		sts(":%s MODE %s -e", me.name, u->nick);
}

void _modinit(module_t * m)
{
	/* Symbol relocation voodoo. */
	server_login = &hyperion_server_login;
	introduce_nick = &hyperion_introduce_nick;
	quit_sts = &hyperion_quit_sts;
	wallops_sts = &hyperion_wallops_sts;
	join_sts = &hyperion_join_sts;
	kick = &hyperion_kick;
	msg = &hyperion_msg;
	notice_user_sts = &hyperion_notice_user_sts;
	notice_global_sts = &hyperion_notice_global_sts;
	notice_channel_sts = &hyperion_notice_channel_sts;
	wallchops = &hyperion_wallchops;
	numeric_sts = &hyperion_numeric_sts;
	kill_id_sts = &hyperion_kill_id_sts;
	part_sts = &hyperion_part_sts;
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
	next_matching_ban = &hyperion_next_matching_ban;
	is_valid_host = &hyperion_is_valid_host;

	mode_list = hyperion_mode_list;
	ignore_mode_list = hyperion_ignore_mode_list;
	status_mode_list = hyperion_status_mode_list;
	prefix_mode_list = hyperion_prefix_mode_list;

	ircd = &Hyperion;

	pcommand_add("PING", m_ping, 1, MSRC_USER | MSRC_SERVER);
	pcommand_add("PONG", m_pong, 1, MSRC_SERVER);
	pcommand_add("PRIVMSG", m_privmsg, 2, MSRC_USER);
	pcommand_add("NOTICE", m_notice, 2, MSRC_UNREG | MSRC_USER | MSRC_SERVER);
	pcommand_add("SJOIN", m_sjoin, 4, MSRC_SERVER);
	pcommand_add("PART", m_part, 1, MSRC_USER);
	pcommand_add("NICK", m_nick, 2, MSRC_USER | MSRC_SERVER);
	pcommand_add("QUIT", m_quit, 1, MSRC_USER);
	pcommand_add("MODE", m_mode, 2, MSRC_USER | MSRC_SERVER);
	pcommand_add("KICK", m_kick, 2, MSRC_USER | MSRC_SERVER);
	pcommand_add("REMOVE", m_kick, 2, MSRC_USER | MSRC_SERVER);	/* same net result */
	pcommand_add("KILL", m_kill, 1, MSRC_USER | MSRC_SERVER);
	pcommand_add("COLLIDE", m_kill, 1, MSRC_SERVER); /* same net result */
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
	pcommand_add("TOPIC", m_topic, 2, MSRC_USER);
	pcommand_add("STOPIC", m_stopic, 5, MSRC_USER | MSRC_SERVER);
	pcommand_add("SNICK", m_snick, 5, MSRC_SERVER);
	pcommand_add("SETHOST", m_sethost, 2, MSRC_USER | MSRC_SERVER);
	pcommand_add("SETIDENT", m_setident, 2, MSRC_USER | MSRC_SERVER);
	pcommand_add("SETNAME", m_setname, 2, MSRC_USER | MSRC_SERVER);
	pcommand_add("SIGNON", m_signon, 5, MSRC_USER);
	pcommand_add("CAPAB", m_capab, 1, MSRC_UNREG);
	pcommand_add("MOTD", m_motd, 1, MSRC_USER);

	hook_add_event("nick_group");
	hook_add_hook("nick_group", (void (*)(void *))nick_group);
	hook_add_event("nick_ungroup");
	hook_add_hook("nick_ungroup", (void (*)(void *))nick_ungroup);

	m->mflags = MODTYPE_CORE;

	pmodule_loaded = TRUE;
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
