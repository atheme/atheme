/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Copyright (c) 2005-2007 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains protocol support for TS6-based ircd.
 */

/*
 * protocol/ts6-generic
 *
 * This module is not very useful on it's own.  It implements
 * the basis of the TS6 extended linking profile, which is used
 * by hybrid, charybdis, ratbox (--enable-services), weircd
 * and others.
 *
 * Everything in this module can be subclassed.  To do so, we
 * recommend either directly subclassing this module, or subclassing
 * the module your ircd is based on.  You can override (or redirect)
 * IRC messages to your protocol and leave the bulk of the command
 * processing and message sending in this module, which should
 * "just work" unless your IRCd is broken.
 *
 * A note to IRCd developers:
 *
 * If you are working with TS6 as your linking protocol, it is
 * important that you follow the TS6 standards exactly.  If you are
 * confused on what you should do, look at this module.  This
 * module describes all of the hardcoded requirements of the
 * extended linking profile.
 *
 * This module is designed where it can downgrade to TS6 strict, or
 * TS5/TS3.  Earlier link protocol versions than TS3 will likely not
 * work.
 */

#include "atheme.h"
#include "uplink.h"
#include "pmodule.h"

DECLARE_MODULE_V1("protocol/ts6-generic", true, _modinit, NULL, PACKAGE_STRING, "Atheme Development Group <http://www.atheme.org>");

static bool use_rserv_support = false;
static bool use_tb = false;
static bool use_euid = false;
static bool use_eopmod = false;
static bool use_mlock = false;

static void server_eob(server_t *s);
static server_t *sid_find(char *name);

static char ts6sid[3 + 1] = "";

static bool ts6_is_valid_host(const char *host)
{
	const char *p;

	for (p = host; *p != '\0'; p++)
		if (!((*p >= '0' && *p <= '9') || (*p >= 'A' && *p <= 'Z') ||
					(*p >= 'a' && *p <= 'z') || *p == '.'
					|| *p == ':' || *p == '-'))
			return false;
	return true;
}

/* login to our uplink */
static unsigned int ts6_server_login(void)
{
	int ret = 1;

	if (!me.numeric)
	{
		ircd->uses_uid = false;
		ret = sts("PASS %s :TS", curr_uplink->pass);
	}
	else if (strlen(me.numeric) == 3 && isdigit(*me.numeric))
	{
		ircd->uses_uid = true;
		ret = sts("PASS %s TS 6 :%s", curr_uplink->pass, me.numeric);
	}
	else
	{
		slog(LG_ERROR, "Invalid numeric (SID) %s", me.numeric);
	}
	if (ret == 1)
		return 1;

	me.bursting = true;

	sts("CAPAB :QS EX IE KLN UNKLN ENCAP TB SERVICES EUID EOPMOD MLOCK");
	sts("SERVER %s 1 :%s", me.name, me.desc);
	sts("SVINFO %d 3 0 :%lu", ircd->uses_uid ? 6 : 5,
			(unsigned long)CURRTIME);

	return 0;
}

/* introduce a client */
static void ts6_introduce_nick(user_t *u)
{
	const char *umode = user_get_umodestr(u);

	if (ircd->uses_uid && use_euid)
		sts(":%s EUID %s 1 %lu %sS %s %s 0 %s * * :%s", me.numeric, u->nick, (unsigned long)u->ts, umode, u->user, u->host, u->uid, u->gecos);
	else if (ircd->uses_uid)
		sts(":%s UID %s 1 %lu %sS %s %s 0 %s :%s", me.numeric, u->nick, (unsigned long)u->ts, umode, u->user, u->host, u->uid, u->gecos);
	else
		sts("NICK %s 1 %lu %sS %s %s %s :%s", u->nick, (unsigned long)u->ts, umode, u->user, u->host, me.name, u->gecos);
}

/* invite a user to a channel */
static void ts6_invite_sts(user_t *sender, user_t *target, channel_t *channel)
{
	sts(":%s INVITE %s %s", CLIENT_NAME(sender), CLIENT_NAME(target), channel->name);
}

static void ts6_quit_sts(user_t *u, const char *reason)
{
	if (!me.connected)
		return;

	sts(":%s QUIT :%s", CLIENT_NAME(u), reason);
}

/* WALLOPS wrapper */
static void ts6_wallops_sts(const char *text)
{
	sts(":%s WALLOPS :%s", ME, text);
}

/* join a channel */
static void ts6_join_sts(channel_t *c, user_t *u, bool isnew, char *modes)
{
	if (isnew)
		sts(":%s SJOIN %lu %s %s :@%s", ME, (unsigned long)c->ts,
				c->name, modes, CLIENT_NAME(u));
	else
		sts(":%s SJOIN %lu %s + :@%s", ME, (unsigned long)c->ts,
				c->name, CLIENT_NAME(u));
}

static void ts6_chan_lowerts(channel_t *c, user_t *u)
{
	slog(LG_DEBUG, "ts6_chan_lowerts(): lowering TS for %s to %lu",
			c->name, (unsigned long)c->ts);
	sts(":%s SJOIN %lu %s %s :@%s", ME, (unsigned long)c->ts, c->name,
				channel_modes(c, true), CLIENT_NAME(u));
	if (ircd->uses_uid)
		chanban_clear(c);
}

/* kicks a user from a channel */
static void ts6_kick(user_t *source, channel_t *c, user_t *u, const char *reason)
{
	if (c->ts != 0 || chanuser_find(c, source))
		sts(":%s KICK %s %s :%s", CLIENT_NAME(source), c->name, CLIENT_NAME(u), reason);
	else
		sts(":%s KICK %s %s :%s", ME, c->name, CLIENT_NAME(u), reason);

	chanuser_delete(c, u);
}

/* PRIVMSG wrapper */
static void ts6_msg(const char *from, const char *target, const char *fmt, ...)
{
	va_list ap;
	char buf[BUFSIZE];
	user_t *u = user_find(from);
	user_t *t = user_find(target);

	if (!u)
		return;

	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZE, fmt, ap);
	va_end(ap);

	/* If this is to a channel, it's the snoop channel so chanserv
	 * is on it -- jilles
	 *
	 * Well, now it's operserv, but yes it's safe to assume that
	 * the source would be able to send to whatever target it is 
	 * sending to. --nenolod
	 */
	sts(":%s PRIVMSG %s :%s", CLIENT_NAME(u), t ? CLIENT_NAME(t) : target, buf);
}

static void ts6_msg_global_sts(user_t *from, const char *mask, const char *text)
{
	mowgli_node_t *n;
	tld_t *tld;

	if (!strcmp(mask, "*"))
	{
		MOWGLI_ITER_FOREACH(n, tldlist.head)
		{
			tld = n->data;
			sts(":%s PRIVMSG %s*%s :%s", from ? CLIENT_NAME(from) : ME, ircd->tldprefix, tld->name, text);
		}
	}
	else
		sts(":%s PRIVMSG %s%s :%s", from ? CLIENT_NAME(from) : ME, ircd->tldprefix, mask, text);
}

/* NOTICE wrapper */
static void ts6_notice_user_sts(user_t *from, user_t *target, const char *text)
{
	sts(":%s NOTICE %s :%s", from ? CLIENT_NAME(from) : ME, CLIENT_NAME(target), text);
}

static void ts6_notice_global_sts(user_t *from, const char *mask, const char *text)
{
	mowgli_node_t *n;
	tld_t *tld;

	if (!strcmp(mask, "*"))
	{
		MOWGLI_ITER_FOREACH(n, tldlist.head)
		{
			tld = n->data;
			sts(":%s NOTICE %s*%s :%s", from ? CLIENT_NAME(from) : ME, ircd->tldprefix, tld->name, text);
		}
	}
	else
		sts(":%s NOTICE %s%s :%s", from ? CLIENT_NAME(from) : ME, ircd->tldprefix, mask, text);
}

static void ts6_notice_channel_sts(user_t *from, channel_t *target, const char *text)
{
	if (from == NULL || chanuser_find(target, from))
		sts(":%s NOTICE %s :%s", from ? CLIENT_NAME(from) : ME, target->name, text);
	else
		sts(":%s NOTICE %s :[%s:%s] %s", ME, target->name, from->nick, target->name, text);
}

static void ts6_wallchops(user_t *sender, channel_t *channel, const char *message)
{
	if (chanuser_find(channel, sender))
		sts(":%s NOTICE @%s :%s", CLIENT_NAME(sender), channel->name,
				message);
	else /* do not join for this, everyone would see -- jilles */
		generic_wallchops(sender, channel, message);
}

/* numeric wrapper */
static void ts6_numeric_sts(server_t *from, int numeric, user_t *target, const char *fmt, ...)
{
	va_list ap;
	char buf[BUFSIZE];

	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZE, fmt, ap);
	va_end(ap);

	sts(":%s %d %s %s", SERVER_NAME(from), numeric, CLIENT_NAME(target), buf);
}

/* KILL wrapper */
static void ts6_kill_id_sts(user_t *killer, const char *id, const char *reason)
{
	if (killer != NULL)
		sts(":%s KILL %s :%s!%s (%s)", CLIENT_NAME(killer), id, killer->host, killer->nick, reason);
	else
		sts(":%s KILL %s :%s (%s)", ME, id, me.name, reason);
}

/* PART wrapper */
static void ts6_part_sts(channel_t *c, user_t *u)
{
	sts(":%s PART %s", CLIENT_NAME(u), c->name);
}

/* server-to-server KLINE wrapper */
static void ts6_kline_sts(const char *server, const char *user, const char *host, long duration, const char *reason)
{
	service_t *svs;

	if (!me.connected)
		return;

	svs = service_find("operserv");
	sts(":%s ENCAP %s KLINE %ld %s %s :%s", svs != NULL ? CLIENT_NAME(svs->me) : ME, server, duration, user, host, reason);
}

/* server-to-server UNKLINE wrapper */
static void ts6_unkline_sts(const char *server, const char *user, const char *host)
{
	service_t *svs;

	if (!me.connected)
		return;

	svs = service_find("operserv");
	sts(":%s ENCAP %s UNKLINE %s %s", svs != NULL ? CLIENT_NAME(svs->me) : ME, server, user, host);
}

/* server-to-server XLINE wrapper */
static void ts6_xline_sts(const char *server, const char *realname, long duration, const char *reason)
{
	service_t *svs;

	if (!me.connected)
		return;

	svs = service_find("operserv");
	sts(":%s ENCAP %s XLINE %ld %s 2 :%s", svs != NULL ? CLIENT_NAME(svs->me) : ME, server, duration, realname, reason);
}

/* server-to-server UNXLINE wrapper */
static void ts6_unxline_sts(const char *server, const char *realname)
{
	service_t *svs;

	if (!me.connected)
		return;

	svs = service_find("operserv");
	sts(":%s ENCAP %s UNXLINE %s", svs != NULL ? CLIENT_NAME(svs->me) : ME, server, realname);
}

/* server-to-server QLINE wrapper */
static void ts6_qline_sts(const char *server, const char *name, long duration, const char *reason)
{
	service_t *svs;

	if (!me.connected)
		return;

	svs = service_find("operserv");
	sts(":%s ENCAP %s RESV %ld %s 0 :%s", svs != NULL ? CLIENT_NAME(svs->me) : ME, server, duration, name, reason);
}

/* server-to-server UNQLINE wrapper */
static void ts6_unqline_sts(const char *server, const char *name)
{
	service_t *svs;

	if (!me.connected)
		return;

	svs = service_find("operserv");
	sts(":%s ENCAP %s UNRESV %s", svs != NULL ? CLIENT_NAME(svs->me) : ME, server, name);
}

/* topic wrapper */
static void ts6_topic_sts(channel_t *c, user_t *source, const char *setter, time_t ts, time_t prevts, const char *topic)
{
	int joined = 0;

	if (!me.connected || !c)
		return;

	/* If possible, try to use ETB */
	if (use_eopmod && (c->ts > 0 || ts > prevts))
	{
		sts(":%s ETB 0 %s %lu %s :%s",
				CLIENT_NAME(source),
				c->name, (unsigned long)ts, setter, topic);
		return;
	}

	/* If possible, try to use TB
	 * Note that because TOPIC does not contain topicTS, it may be
	 * off a few seconds on other servers, hence the 60 seconds here.
	 * -- jilles */
	if (use_tb && *topic != '\0')
	{
		/* Restoring old topic */
		if (ts < prevts || prevts == 0)
		{
			if (prevts != 0 && ts + 60 > prevts)
				ts = prevts - 60;
			sts(":%s TB %s %lu %s :%s", ME, c->name, (unsigned long)ts, setter, topic);
			c->topicts = ts;
			return;
		}
		/* Tweaking a topic */
		else if (ts == prevts)
		{
			ts -= 60;
			sts(":%s TB %s %lu %s :%s", ME, c->name, (unsigned long)ts, setter, topic);
			c->topicts = ts;
			return;
		}
	}
	/* We have to be on channel to change topic.
	 * We cannot nicely change topic from the server:
	 * :server.name TOPIC doesn't propagate and TB requires
	 * us to specify an older topicts.
	 * -- jilles
	 */
	if (!chanuser_find(c, source))
	{
		sts(":%s SJOIN %lu %s + :@%s", ME, (unsigned long)c->ts, c->name, CLIENT_NAME(source));
		joined = 1;
	}
	sts(":%s TOPIC %s :%s", CLIENT_NAME(source), c->name, topic);
	if (joined)
		sts(":%s PART %s :Topic set for %s",
				CLIENT_NAME(source), c->name, setter);
	c->topicts = CURRTIME;
}

/* mode wrapper */
static void ts6_mode_sts(char *sender, channel_t *target, char *modes)
{
	user_t *u = user_find(sender);

	if (!me.connected || !u)
		return;

	if (ircd->uses_uid)
		sts(":%s TMODE %lu %s %s", CLIENT_NAME(u), (unsigned long)target->ts, target->name, modes);
	else
		sts(":%s MODE %s %s", CLIENT_NAME(u), target->name, modes);
}

/* ping wrapper */
static void ts6_ping_sts(void)
{
	if (!me.connected)
		return;

	sts("PING :%s", me.name);
}

/* protocol-specific stuff to do on login */
static void ts6_on_login(user_t *u, myuser_t *mu, const char *wantedhost)
{
	if (!me.connected || !use_rserv_support || u == NULL)
		return;

	sts(":%s ENCAP * SU %s %s", ME, CLIENT_NAME(u), entity(mu)->name);
}

/* protocol-specific stuff to do on login */
static bool ts6_on_logout(user_t *u, const char *account)
{
	if (!me.connected || !use_rserv_support || u == NULL)
		return false;

	sts(":%s ENCAP * SU %s", ME, CLIENT_NAME(u));
	return false;
}

/* XXX we don't have an appropriate API for this, what about making JUPE
 * serverside like in P10?
 *       --nenolod
 */
static void ts6_jupe(const char *server, const char *reason)
{
	service_t *svs;

	if (!me.connected)
		return;

	server_delete(server);

	svs = service_find("operserv");
	sts(":%s SQUIT %s :%s", svs != NULL ? CLIENT_NAME(svs->me) : ME, server, reason);
	sts(":%s SERVER %s 2 :(H) %s", me.name, server, reason);
}

static void ts6_sethost_sts(user_t *source, user_t *target, const char *host)
{
	if (use_euid)
		sts(":%s CHGHOST %s :%s", ME, CLIENT_NAME(target), host);
	else
		sts(":%s ENCAP * CHGHOST %s :%s", ME, target->nick, host);
}

static void ts6_fnc_sts(user_t *source, user_t *u, char *newnick, int type)
{
	/* XXX assumes the server will accept this -- jilles */
	sts(":%s ENCAP %s RSFNC %s %s %lu %lu", ME,
			u->server->name,
			CLIENT_NAME(u), newnick,
			(unsigned long)(CURRTIME - 60),
			(unsigned long)u->ts);
}

static void ts6_svslogin_sts(char *target, char *nick, char *user, char *host, char *login)
{
	user_t *tu = user_find(target);
	server_t *s;

	if(tu)
		s = tu->server;
	else if(ircd->uses_uid) /* Non-announced UID - must be a SASL client. */
		s = sid_find(target);
	else
		return;

	sts(":%s ENCAP %s SVSLOGIN %s %s %s %s %s", ME, s->name,
			target, nick, user, host, login);
}

static void ts6_sasl_sts(char *target, char mode, char *data)
{
	service_t *svs;
	server_t *s = sid_find(target);

	if(s == NULL)
		return;

	svs = service_find("saslserv");
	if (svs == NULL)
	{
		slog(LG_ERROR, "ts6_sasl_sts(): saslserv does not exist!");
		return;
	}

	sts(":%s ENCAP %s SASL %s %s %c %s", ME, s->name,
			svs->me->uid,
			target,
			mode,
			data);
}

static void ts6_holdnick_sts(user_t *source, int duration, const char *nick, myuser_t *mu)
{
	if (use_euid)
		sts(":%s ENCAP * NICKDELAY %d %s", ME, duration, nick);
	else
	{
		if (duration == 0)
			return; /* can't do this safely */
		sts(":%s ENCAP * RESV %d %s 0 :Reserved by %s for nickname owner (%s)",
				CLIENT_NAME(source), duration > 300 ? 300 : duration,
				nick, source->nick,
				mu ? entity(mu)->name : nick);
	}
}

static void ts6_mlock_sts(channel_t *c)
{
	mychan_t *mc = mychan_find(c->name);

	if (use_mlock == false)
		return;

	if (mc == NULL)
		return;

	sts(":%s MLOCK %ld %s :%s", ME, c->ts, c->name, mychan_get_sts_mlock(mc));
}

static void m_mlock(sourceinfo_t *si, int parc, char *parv[])
{
	channel_t *c;
	mychan_t *mc;
	const char *mlock;

	/* Ignore MLOCK if the server isn't bursting, to avoid 'war' conditions */
	if (si->s->flags & SF_EOB)
		return;

	if (!(c = channel_find(parv[1])))
		return;

	if (!(mc = mychan_find(c->name)))
	{
		/* Unregistered channel. Clear the MLOCK. */
		sts(":%s MLOCK %ld %s :", ME, c->ts, c->name);
		return;
	}

	time_t ts = atol(parv[0]);
	if (ts > c->ts)
		return;

	mlock = mychan_get_sts_mlock(mc);
	if (0 != strcmp(parv[2], mlock))
	{
		/* MLOCK is changing, with the same TS. Bounce back the correct one. */
		sts(":%s MLOCK %ld %s :%s", ME, c->ts, c->name, mlock);
	}
}


static void m_topic(sourceinfo_t *si, int parc, char *parv[])
{
	channel_t *c = channel_find(parv[0]);

	if (c == NULL)
		return;

	handle_topic_from(si, c, si->su->nick, CURRTIME, parv[1]);
}

static void m_tb(sourceinfo_t *si, int parc, char *parv[])
{
	channel_t *c = channel_find(parv[0]);
	time_t ts = atol(parv[1]);

	if (c == NULL)
		return;

	if (c->topic != NULL && c->topicts <= ts)
	{
		slog(LG_DEBUG, "m_tb(): ignoring newer topic on %s", c->name);
		return;
	}

	handle_topic_from(si, c, parc > 3 ? parv[2] : si->s->name, ts, parv[parc - 1]);
}

static void m_etb(sourceinfo_t *si, int parc, char *parv[])
{
	channel_t *c = channel_find(parv[1]);
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

	channelts = atol(parv[0]);
	topicts = atol(parv[2]);
	if (c->topic == NULL || channelts < c->ts || (channelts == c->ts && topicts > c->topicts))
		handle_topic_from(si, c, parv[3], topicts, parv[parc - 1]);
}

static void m_ping(sourceinfo_t *si, int parc, char *parv[])
{
	/* reply to PING's */
	sts(":%s PONG %s %s", ME, me.name, parv[0]);
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
	bool keep_new_modes = true;
	unsigned int userc;
	char *userv[256];
	unsigned int i;
	time_t ts;
	char *p;

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
		 *
		 * if the source does TS6, also remove all bans
		 * note that JOIN does not do this
		 */

		clear_simple_modes(c);
		if (si->s->sid != NULL)
			chanban_clear(c);

		MOWGLI_ITER_FOREACH(n, c->members.head)
		{
			cu = (chanuser_t *)n->data;
			if (cu->user->server == me.me)
			{
				/* it's a service, reop */
				sts(":%s PART %s :Reop", CLIENT_NAME(cu->user), c->name);
				sts(":%s SJOIN %lu %s + :@%s", ME, (unsigned long)ts, c->name, CLIENT_NAME(cu->user));
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
			/* XXX for TS5 we should mark them deopped
			 * if they were opped and drop modes from them
			 * -- jilles */
			chanuser_add(c, p);
		}

	if (c->nummembers == 0 && !(c->modes & ircd->perm_mode))
		channel_delete(c);
}

static void m_join(sourceinfo_t *si, int parc, char *parv[])
{
	/* -> :1JJAAAAAB JOIN 1127474195 #test +tn */
	bool keep_new_modes = true;
	mowgli_node_t *n, *tn;
	channel_t *c;
	chanuser_t *cu;
	time_t ts;

	/* JOIN 0 is really a part from all channels */
	/* be sure to allow joins to TS 0 channels -- jilles */
	if (parv[0][0] == '0' && parc <= 2)
	{
		MOWGLI_ITER_FOREACH_SAFE(n, tn, si->su->channels.head)
		{
			cu = (chanuser_t *)n->data;
			chanuser_delete(cu->chan, si->su);
		}
		return;
	}

	/* :user JOIN ts chan modestr [key or limits] */
	c = channel_find(parv[1]);
	ts = atol(parv[0]);

	if (!c)
	{
		slog(LG_DEBUG, "m_join(): new channel: %s", parv[1]);
		c = channel_add(parv[1], ts, si->su->server);
	}

	if (ts == 0 || c->ts == 0)
	{
		if (c->ts != 0)
			slog(LG_INFO, "m_join(): server %s changing TS on %s from %lu to 0", si->su->server->name, c->name, (unsigned long)c->ts);
		c->ts = 0;
		hook_call_channel_tschange(c);
	}
	else if (ts < c->ts)
	{
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
				sts(":%s PART %s :Reop", CLIENT_NAME(cu->user), c->name);
				sts(":%s SJOIN %lu %s + :@%s", ME, (unsigned long)ts, c->name, CLIENT_NAME(cu->user));
				cu->modes = CSTATUS_OP;
			}
			else
				cu->modes = 0;
		}
		slog(LG_DEBUG, "m_join(): TS changed for %s (%lu -> %lu)", c->name, (unsigned long)c->ts, (unsigned long)ts);
		c->ts = ts;
		hook_call_channel_tschange(c);
	}
	else if (ts > c->ts)
		keep_new_modes = false;

	if (keep_new_modes)
		channel_mode(NULL, c, parc - 2, parv + 2);

	chanuser_add(c, CLIENT_NAME(si->su));
}

static void m_bmask(sourceinfo_t *si, int parc, char *parv[])
{
	unsigned int ac, i;
	char *av[256];
	channel_t *c = channel_find(parv[1]);
	int type;

	/* :1JJ BMASK 1127474361 #services b :*!*@*evil* *!*eviluser1@* */
	if (!c)
	{
		slog(LG_DEBUG, "m_bmask(): got bmask for unknown channel");
		return;
	}

	if (atol(parv[0]) > c->ts)
		return;
	
	type = *parv[2];
	if (!strchr(ircd->ban_like_modes, type))
	{
		slog(LG_DEBUG, "m_bmask(): got unknown type '%c'", type);
		return;
	}

	ac = sjtoken(parv[parc - 1], ' ', av);

	for (i = 0; i < ac; i++)
		chanban_add(c, av[i], type);
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

	/* got the right number of args for an introduction? */
	if (parc == 8)
	{
		s = server_find(parv[6]);
		if (!s)
		{
			slog(LG_DEBUG, "m_nick(): new user on nonexistant server: %s", parv[6]);
			return;
		}

		slog(LG_DEBUG, "m_nick(): new user on `%s': %s", s->name, parv[0]);

		u = user_add(parv[0], parv[4], parv[5], NULL, NULL, NULL, parv[7], s, atoi(parv[2]));
		if (u == NULL)
			return;

		user_mode(u, parv[3]);

		/* If server is not yet EOB we will do this later.
		 * This avoids useless "please identify" -- jilles */
		if (s->flags & SF_EOB)
			handle_nickchange(user_find(parv[0]));
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

		if (user_changenick(si->su, parv[0], atoi(parv[1])))
			return;

		/* It could happen that our PING arrived late and the
		 * server didn't acknowledge EOB yet even though it is
		 * EOB; don't send double notices in that case -- jilles */
		if (si->su->server->flags & SF_EOB)
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

static void m_uid(sourceinfo_t *si, int parc, char *parv[])
{
	server_t *s;
	user_t *u;

	/* got the right number of args for an introduction? */
	if (parc == 9)
	{
		s = si->s;
		slog(LG_DEBUG, "m_uid(): new user on `%s': %s", s->name, parv[0]);

		u = user_add(parv[0], parv[4], parv[5], NULL, parv[6], parv[7], parv[8], s, atoi(parv[2]));
		if (u == NULL)
			return;

		user_mode(u, parv[3]);

		/* If server is not yet EOB we will do this later.
		 * This avoids useless "please identify" -- jilles
		 */
		if (s->flags & SF_EOB)
			handle_nickchange(user_find(parv[0]));
	}
	else
	{
		int i;
		slog(LG_DEBUG, "m_uid(): got UID with wrong number of params");

		for (i = 0; i < parc; i++)
			slog(LG_DEBUG, "m_uid():   parv[%d] = %s", i, parv[i]);
	}
}

static void m_euid(sourceinfo_t *si, int parc, char *parv[])
{
	server_t *s;
	user_t *u;

	/* got the right number of args for an introduction? */
	if (parc >= 11)
	{
		s = si->s;
		slog(LG_DEBUG, "m_euid(): new user on `%s': %s", s->name, parv[0]);

		u = user_add(parv[0],				/* nick */
			parv[4],				/* user */
			*parv[8] != '*' ? parv[8] : parv[5],	/* hostname */
			parv[5],				/* hostname (visible) */
			parv[6],				/* ip */
			parv[7],				/* uid */
			parv[parc - 1],				/* gecos */
			s,					/* object parent (server) */
			atoi(parv[2]));				/* hopcount */
		if (u == NULL)
			return;

		user_mode(u, parv[3]);
		if (*parv[9] != '*')
			handle_burstlogin(u, parv[9], 0);

		/* server_eob() cannot know if a user was introduced
		 * with NICK/UID or EUID and handle_nickchange() must
		 * be called exactly once for each new user -- jilles */
		if (s->flags & SF_EOB)
			handle_nickchange(u);
	}
	else
	{
		int i;
		slog(LG_DEBUG, "m_euid(): got EUID with wrong number of params");

		for (i = 0; i < parc; i++)
			slog(LG_DEBUG, "m_euid():   parv[%d] = %s", i, parv[i]);
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

static void m_tmode(sourceinfo_t *si, int parc, char *parv[])
{
	channel_t *c;

	/* -> :1JJAAAAAB TMODE 1127511579 #new +o 2JJAAAAAB */
	c = channel_find(parv[1]);
	if (c == NULL)
	{
		slog(LG_DEBUG, "m_tmode(): nonexistent channel %s", parv[1]);
		return;
	}

	if (atol(parv[0]) > c->ts)
		return;

	channel_mode(NULL, c, parc - 2, &parv[2]);
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
	s = handle_server(si, parv[0], si->s || !ircd->uses_uid ? NULL : ts6sid, atoi(parv[1]), parv[2]);

	if (s != NULL && s->uplink != me.me)
	{
		/* elicit PONG for EOB detection; pinging uplink is
		 * already done elsewhere -- jilles
		 */
		sts(":%s PING %s %s", ME, me.name, s->name);
	}
}

static void m_sid(sourceinfo_t *si, int parc, char *parv[])
{
	/* -> :1JJ SID file. 2 00F :telnet server */
	server_t *s;

	slog(LG_DEBUG, "m_sid(): new server: %s", parv[0]);
	s = handle_server(si, parv[0], parv[2], atoi(parv[1]), parv[3]);

	if (s != NULL && s->uplink != me.me)
	{
		/* elicit PONG for EOB detection; pinging uplink is
		 * already done elsewhere -- jilles
		 */
		sts(":%s PING %s %s", ME, me.name, s->sid);
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

static void m_pass(sourceinfo_t *si, int parc, char *parv[])
{
	/* TS5: PASS mypassword :TS
	 * TS6: PASS mypassword TS 6 :sid */
	if (strcmp(curr_uplink->pass, parv[0]))
	{
		slog(LG_INFO, "m_pass(): password mismatch from uplink; aborting");
		runflags |= RF_SHUTDOWN;
	}

	if (ircd->uses_uid && parc > 3 && atoi(parv[2]) >= 6)
		strlcpy(ts6sid, parv[3], sizeof(ts6sid));
	else
	{
		if (ircd->uses_uid)
		{
			slog(LG_INFO, "m_pass(): uplink does not support TS6, falling back to TS5");
			ircd->uses_uid = false;
		}
		ts6sid[0] = '\0';
	}
}

static void m_error(sourceinfo_t *si, int parc, char *parv[])
{
	slog(LG_INFO, "m_error(): error from server: %s", parv[0]);
}

static void m_encap(sourceinfo_t *si, int parc, char *parv[])
{
	user_t *u;

	if (match(parv[0], me.name))
		return;
	if (!irccasecmp(parv[1], "LOGIN"))
	{
		/* :jilles ENCAP * LOGIN jilles */
		if (!use_rserv_support)
			return;
		if (parc < 3)
			return;
		u = si->su;
		if (u == NULL)
			return;
		/* We used to throw out LOGINs outside of a burst,
		 * but we can't do that anymore since it's used for
		 * SASL users.
		 * --gxti
		 */
		handle_burstlogin(u, parv[2], 0);
	}
	else if (!irccasecmp(parv[1], "SU"))
	{
		/* :services. ENCAP * SU jilles_ jilles */
		/* :services. ENCAP * SU jilles_ */
		if (!use_rserv_support)
			return;
		if (parc < 3)
			return;
		u = user_find(parv[2]);
		if (u == NULL)
			return;
		if (parc == 3)
			handle_clearlogin(si, u);
		else
			handle_setlogin(si, u, parv[3], 0);
	}
	else if (!irccasecmp(parv[1], "REALHOST"))
	{
		/* :1JJAAAAAC ENCAP * REALHOST localhost.stack.nl */
		if (parc < 3)
			return;
		u = si->su;
		if (u == NULL)
			return;
		strlcpy(u->host, parv[2], HOSTLEN);
	}
	else if (!irccasecmp(parv[1], "CHGHOST"))
	{
		if (parc < 4)
			return;
		u = user_find(parv[2]);
		if (u == NULL)
			return;
		strlcpy(u->vhost, parv[3], HOSTLEN);
		slog(LG_DEBUG, "m_encap(): chghost %s -> %s", u->nick,
				u->vhost);
	}
	else if (!irccasecmp(parv[1], "SASL"))
	{
		/* :08C ENCAP * SASL 08CAAAAAE * S d29vTklOSkFTAGRhdGEgaW4gZmlyc3QgbGluZQ== */
		sasl_message_t smsg;

		if (parc < 6)
			return;

		smsg.uid = parv[2];
		smsg.mode = *parv[4];
		smsg.buf = parv[5];
		hook_call_sasl_input(&smsg);
	}
	else if (!irccasecmp(parv[1], "RSMSG"))
	{
		char buf[512];
		char dest[NICKLEN + HOSTLEN];
		int i;

		if (parc < 4)
			return;
		buf[0] = '\0';
		for (i = 3; i < parc; i++)
		{
			if (i > 3)
				strlcat(buf, " ", sizeof buf);
			strlcat(buf, parv[i], sizeof buf);
		}
		snprintf(dest, sizeof dest, "%s@%s", parv[2], me.name);
		handle_message(si, dest, false, buf);
	}
	else if (!irccasecmp(parv[1], "CERTFP"))
	{
		if (parc < 3)
			return;

		u = si->su;
		if (u == NULL)
			return;

		handle_certfp(si, u, parv[2]);
	}
}

static void m_signon(sourceinfo_t *si, int parc, char *parv[])
{
	user_t *u;

	if((u = user_find(parv[0])) == NULL)
		return;

	/* NICK */
	if (user_changenick(u, parv[0], atoi(parv[3])))
		return;

	handle_nickchange(u); /* If they're logging out, this will bug them about identifying. Or something. */

	/* USER */
	strlcpy(u->user, parv[1], USERLEN);

	/* HOST */
	strlcpy(u->vhost, parv[2], HOSTLEN);

	/* LOGIN */
	if(*parv[4] == '*') /* explicitly unchanged */
		return;
	if (!strcmp(parv[4], "0"))
		handle_clearlogin(si, u);
	else
		handle_setlogin(si, u, parv[4], 0);
}

static void m_capab(sourceinfo_t *si, int parc, char *parv[])
{
	char *p;

	use_euid = false;
	use_rserv_support = false;
	use_tb = false;
	use_eopmod = false;
	use_mlock = false;
	for (p = strtok(parv[0], " "); p != NULL; p = strtok(NULL, " "))
	{
		if (!irccasecmp(p, "EUID"))
		{
			slog(LG_DEBUG, "m_capab(): uplink supports EUID, enabling support.");
			use_euid = true;
		}
		if (!irccasecmp(p, "SERVICES"))
		{
			slog(LG_DEBUG, "m_capab(): uplink has rserv extensions, enabling support.");
			use_rserv_support = true;
		}
		if (!irccasecmp(p, "TB"))
		{
			slog(LG_DEBUG, "m_capab(): uplink does topic bursting, using if appropriate.");
			use_tb = true;
		}
		if (!irccasecmp(p, "EOPMOD"))
		{
			slog(LG_DEBUG, "m_capab(): uplink supports EOPMOD, enabling support.");
			use_eopmod = true;
		}
		if (!irccasecmp(p, "MLOCK"))
		{
			slog(LG_DEBUG, "m_capab(): uplink supports MLOCK, enabling support.");
			use_mlock = true;
		}
	}

	/* Now we know whether or not we should enable services support,
	 * so burst the clients.
	 *       --nenolod
	 */
	services_init();
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

/* Server ended their burst: warn all their users if necessary -- jilles */
static void server_eob(server_t *s)
{
	mowgli_node_t *n;

	MOWGLI_ITER_FOREACH(n, s->userlist.head)
	{
		handle_nickchange((user_t *)n->data);
	}
}

/* Channel drop hook: clear MLOCK when a channel becomes unregistered. */
static void channel_drop(mychan_t *mc)
{
	if (use_mlock == false)
		return;

	/* Don't reset it if the channel doesn't exist. */
	if (!mc->chan)
		return;

	sts(":%s MLOCK %ld %s :", ME, mc->chan->ts, mc->chan->name);
}

static server_t *sid_find(char *name)
{
	char sid[4];
	strlcpy(sid, name, 4);
	return server_find(sid);
}

void _modinit(module_t * m)
{
	/* Symbol relocation voodoo. */
	server_login = &ts6_server_login;
	introduce_nick = &ts6_introduce_nick;
	quit_sts = &ts6_quit_sts;
	wallops_sts = &ts6_wallops_sts;
	join_sts = &ts6_join_sts;
	chan_lowerts = &ts6_chan_lowerts;
	kick = &ts6_kick;
	msg = &ts6_msg;
	msg_global_sts = &ts6_msg_global_sts;
	notice_user_sts = &ts6_notice_user_sts;
	notice_global_sts = &ts6_notice_global_sts;
	notice_channel_sts = &ts6_notice_channel_sts;
	wallchops = &ts6_wallchops;
	numeric_sts = &ts6_numeric_sts;
	kill_id_sts = &ts6_kill_id_sts;
	part_sts = &ts6_part_sts;
	kline_sts = &ts6_kline_sts;
	unkline_sts = &ts6_unkline_sts;
	xline_sts = &ts6_xline_sts;
	unxline_sts = &ts6_unxline_sts;
	qline_sts = &ts6_qline_sts;
	unqline_sts = &ts6_unqline_sts;
	topic_sts = &ts6_topic_sts;
	mode_sts = &ts6_mode_sts;
	ping_sts = &ts6_ping_sts;
	ircd_on_login = &ts6_on_login;
	ircd_on_logout = &ts6_on_logout;
	jupe = &ts6_jupe;
	sethost_sts = &ts6_sethost_sts;
	fnc_sts = &ts6_fnc_sts;
	invite_sts = &ts6_invite_sts;
	holdnick_sts = &ts6_holdnick_sts;
	svslogin_sts = &ts6_svslogin_sts;
	sasl_sts = &ts6_sasl_sts;
	is_valid_host = &ts6_is_valid_host;
	mlock_sts = &ts6_mlock_sts;

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
	pcommand_add("TOPIC", m_topic, 2, MSRC_USER);
	pcommand_add("TB", m_tb, 3, MSRC_SERVER);
	pcommand_add("ETB", m_etb, 5, MSRC_USER | MSRC_SERVER);
	pcommand_add("ENCAP", m_encap, 2, MSRC_USER | MSRC_SERVER);
	pcommand_add("CAPAB", m_capab, 1, MSRC_UNREG);
	pcommand_add("UID", m_uid, 9, MSRC_SERVER);
	pcommand_add("BMASK", m_bmask, 4, MSRC_SERVER);
	pcommand_add("TMODE", m_tmode, 3, MSRC_USER | MSRC_SERVER);
	pcommand_add("SID", m_sid, 4, MSRC_SERVER);
	pcommand_add("CHGHOST", m_chghost, 2, MSRC_USER | MSRC_SERVER);
	pcommand_add("MOTD", m_motd, 1, MSRC_USER);
	pcommand_add("SIGNON", m_signon, 5, MSRC_USER);
	pcommand_add("EUID", m_euid, 11, MSRC_SERVER);
	pcommand_add("MLOCK", m_mlock, 3, MSRC_SERVER);

	hook_add_event("server_eob");
	hook_add_event("channel_drop");
	hook_add_server_eob(server_eob);
	hook_add_channel_drop(channel_drop);

	m->mflags = MODTYPE_CORE;
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
