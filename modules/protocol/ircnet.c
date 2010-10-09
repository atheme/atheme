/*
 * Copyright (c) 2005-2006 William Pitcock, et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains protocol support for IRCnet ircd's.
 * Derived mainly from the documentation (or lack thereof)
 * in my protocol bridge.
 *
 */

#include "atheme.h"
#include "uplink.h"
#include "pmodule.h"
#include "protocol/ircnet.h"

DECLARE_MODULE_V1("protocol/ircnet", true, _modinit, NULL, PACKAGE_STRING, "Atheme Development Group <http://www.atheme.org>");

/* *INDENT-OFF* */

ircd_t IRCNet = {
        "ircd 2.11.1p1 or later",       /* IRCd name */
        "$$",                           /* TLD Prefix, used by Global. */
        true,                           /* Whether or not we use IRCNet/TS6 UID */
        false,                          /* Whether or not we use RCOMMAND */
        false,                          /* Whether or not we support channel owners. */
        false,                          /* Whether or not we support channel protection. */
        false,                          /* Whether or not we support halfops. */
	false,				/* Whether or not we use P10 */
	false,				/* Whether or not we use vHosts. */
	0,				/* Oper-only cmodes */
        0,                              /* Integer flag for owner channel flag. */
        0,                              /* Integer flag for protect channel flag. */
        0,                              /* Integer flag for halfops. */
        "+",                            /* Mode we set for owner. */
        "+",                            /* Mode we set for protect. */
        "+",                            /* Mode we set for halfops. */
	PROTOCOL_IRCNET,		/* Protocol type */
	0,                              /* Permanent cmodes */
	0,                              /* Oper-immune cmode */
	"beIR",                         /* Ban-like cmodes */
	'e',                            /* Except mchar */
	'I',                            /* Invex mchar */
	IRCD_CIDR_BANS                  /* Flags */
};

struct cmode_ ircnet_mode_list[] = {
  { 'i', CMODE_INVITE },
  { 'm', CMODE_MOD    },
  { 'n', CMODE_NOEXT  },
  { 'p', CMODE_PRIV   },
  { 's', CMODE_SEC    },
  { 't', CMODE_TOPIC  },
  { '\0', 0 }
};

struct extmode ircnet_ignore_mode_list[] = {
  { '\0', 0 }
};

struct cmode_ ircnet_status_mode_list[] = {
  { 'o', CSTATUS_OP    },
  { 'v', CSTATUS_VOICE },
  { '\0', 0 }
};

struct cmode_ ircnet_prefix_mode_list[] = {
  { '@', CSTATUS_OP    },
  { '+', CSTATUS_VOICE },
  { '\0', 0 }
};

struct cmode_ ircnet_user_mode_list[] = {
  { 'i', UF_INVIS    },
  { 'o', UF_IRCOP    },
  { '\0', 0 }
};

/* *INDENT-ON* */

/* login to our uplink */
static unsigned int ircnet_server_login(void)
{
	int ret;

	ret = sts("PASS %s 0211010000 IRC|aDEFiIJMuw P", curr_uplink->pass);
	if (ret == 1)
		return 1;

	me.bursting = true;

	sts("SERVER %s 1 %s :%s", me.name, me.numeric, me.desc);

	services_init();

	sts(":%s EOB", me.numeric);

	return 0;
}

/* introduce a client */
static void ircnet_introduce_nick(user_t *u)
{
	const char *umode = user_get_umodestr(u);

	sts(":%s UNICK %s %s %s %s 0.0.0.0 %s :%s", me.numeric, u->nick, u->uid, u->user, u->host, umode, u->gecos);
}

/* invite a user to a channel */
static void ircnet_invite_sts(user_t *sender, user_t *target, channel_t *channel)
{
	int joined = 0;

	/* Need to join to invite -- jilles */
	if (!chanuser_find(channel, sender))
	{
		sts(":%s NJOIN %s :@%s", ME, channel->name, CLIENT_NAME(sender));
		joined = 1;
	}
	/* ircnet's UID implementation is incomplete, in many places,
	 * like this one, it does not accept UIDs -- jilles */
	sts(":%s INVITE %s %s", CLIENT_NAME(sender), target->nick, channel->name);
	if (joined)
		sts(":%s PART %s :Invited %s", CLIENT_NAME(sender), channel->name, target->nick);
}

static void ircnet_quit_sts(user_t *u, const char *reason)
{
	if (!me.connected)
		return;

	sts(":%s QUIT :%s", u->nick, reason);
}

/* WALLOPS wrapper */
static void ircnet_wallops_sts(const char *text)
{
	sts(":%s WALLOPS :%s", me.name, text);
}

/* join a channel */
static void ircnet_join_sts(channel_t *c, user_t *u, bool isnew, char *modes)
{
	sts(":%s NJOIN %s :@%s", me.numeric, c->name, u->uid);
	if (isnew && modes[0] && modes[1])
		sts(":%s MODE %s %s", me.numeric, c->name, modes);
}

/* kicks a user from a channel */
static void ircnet_kick(user_t *source, channel_t *c, user_t *u, const char *reason)
{
	/* sigh server kicks will generate snotes
	 * but let's avoid joining N times for N kicks */
	sts(":%s KICK %s %s :%s", source != NULL && chanuser_find(c, source) ? CLIENT_NAME(source) : ME, c->name, CLIENT_NAME(u), reason);

	chanuser_delete(c, u);
}

/* PRIVMSG wrapper */
static void ircnet_msg(const char *from, const char *target, const char *fmt, ...)
{
	va_list ap;
	char buf[BUFSIZE];

	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZE, fmt, ap);
	va_end(ap);

	sts(":%s PRIVMSG %s :%s", from, target, buf);
}

static void ircnet_msg_global_sts(user_t *from, const char *mask, const char *text)
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
static void ircnet_notice_user_sts(user_t *from, user_t *target, const char *text)
{
	sts(":%s NOTICE %s :%s", from ? CLIENT_NAME(from) : ME, CLIENT_NAME(target), text);
}

static void ircnet_notice_global_sts(user_t *from, const char *mask, const char *text)
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

static void ircnet_notice_channel_sts(user_t *from, channel_t *target, const char *text)
{
	if (from == NULL || chanuser_find(target, from))
		sts(":%s NOTICE %s :%s", from ? CLIENT_NAME(from) : ME, target->name, text);
	else
		sts(":%s NOTICE %s :[%s:%s] %s", ME, target->name, from->nick, target->name, text);
}

/* numeric wrapper */
static void ircnet_numeric_sts(server_t *from, int numeric, user_t *target, const char *fmt, ...)
{
	va_list ap;
	char buf[BUFSIZE];

	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZE, fmt, ap);
	va_end(ap);

	/* if we were to use SID/UID here, the user would see SID/UID :( */
	sts(":%s %d %s %s", from->name, numeric, target->nick, buf);
}

/* KILL wrapper */
static void ircnet_kill_id_sts(user_t *killer, const char *id, const char *reason)
{
	if (killer != NULL)
		sts(":%s KILL %s :%s!%s (%s)", CLIENT_NAME(killer), id, killer->host, killer->nick, reason);
	else
		sts(":%s KILL %s :%s (%s)", ME, id, me.name, reason);
}

/* PART wrapper */
static void ircnet_part_sts(channel_t *c, user_t *u)
{
	sts(":%s PART %s", u->nick, c->name);
}

/* server-to-server KLINE wrapper */
static void ircnet_kline_sts(const char *server, const char *user, const char *host, long duration, const char *reason)
{
	service_t *svs;

	if (!me.connected)
		return;

	/* this won't propagate!
	 * you'll need some bot/service on each server to do that */
	if (irccasecmp(server, me.actual) && cnt.server > 2)
		wallops("Missed a tkline");

	svs = service_find("operserv");
	sts(":%s TKLINE %lds %s@%s :%s", svs != NULL ? CLIENT_NAME(svs->me) : me.actual, duration, user, host, reason);
}

/* server-to-server UNKLINE wrapper */
static void ircnet_unkline_sts(const char *server, const char *user, const char *host)
{
	service_t *svs;

	if (!me.connected)
		return;

	if (irccasecmp(server, me.actual) && cnt.server > 2)
		wallops("Missed an untkline");

	svs = service_find("operserv");
	sts(":%s UNTKLINE %s@%s", svs != NULL ? CLIENT_NAME(svs->me) : me.actual, user, host);
}

/* topic wrapper */
static void ircnet_topic_sts(channel_t *c, user_t *source, const char *setter, time_t ts, time_t prevts, const char *topic)
{
	int joined = 0;

	if (!me.connected || !c)
		return;

	/* Need to join to set topic -- jilles */
	if (!chanuser_find(c, source))
	{
		sts(":%s NJOIN %s :@%s", ME, c->name, CLIENT_NAME(source));
		joined = 1;
	}
	sts(":%s TOPIC %s :%s", CLIENT_NAME(source), c->name, topic);
	if (joined)
		sts(":%s PART %s :Topic set for %s",
				CLIENT_NAME(source), c->name, setter);
}

/* mode wrapper */
static void ircnet_mode_sts(char *sender, channel_t *target, char *modes)
{
	user_t *u;

	if (!me.connected)
		return;

	u = user_find(sender);

	/* send it from the server if that service isn't on channel
	 * -- jilles */
	sts(":%s MODE %s %s", chanuser_find(target, u) ? CLIENT_NAME(u) : ME, target->name, modes);
}

/* ping wrapper */
static void ircnet_ping_sts(void)
{
	if (!me.connected)
		return;

	sts("PING :%s", me.name);
}

/* protocol-specific stuff to do on login */
static void ircnet_on_login(user_t *u, myuser_t *account, const char *wantedhost)
{
	/* nothing to do on ratbox */
	return;
}

/* protocol-specific stuff to do on login */
static bool ircnet_on_logout(user_t *u, const char *account)
{
	/* nothing to do on ratbox */
	return false;
}

static void ircnet_jupe(const char *server, const char *reason)
{
	service_t *svs;
	static char sid[4+1];
	int i;
	server_t *s;

	if (!me.connected)
		return;

	svs = service_find("operserv");
	sts(":%s SQUIT %s :%s", svs != NULL ? CLIENT_NAME(svs->me) : me.actual, server, reason);

	s = server_find(server);
	/* We need to wait for the SQUIT to be processed -- jilles */
	if (s != NULL)
	{
		s->flags |= SF_JUPE_PENDING;
		return;
	}

	/* dirty dirty make up some sid */
	if (sid[0] == '\0')
		strlcpy(sid, me.numeric, sizeof sid);
	do
	{
		i = 3;
		for (;;)
		{
			if (sid[i] == 'Z')
			{
				sid[i] = '0';
				i--;
				/* eek, no more sids */
				if (i < 0)
					return;
				continue;
			}
			else if (sid[i] == '9')
				sid[i] = 'A';
			else sid[i]++;
			break;
		}
	} while (server_find(sid));

	sts(":%s SERVER %s 2 %s 0211010000 :%s", me.name, server, sid, reason);
}

static void m_topic(sourceinfo_t *si, int parc, char *parv[])
{
	channel_t *c = channel_find(parv[0]);

	if (!c)
		return;

	handle_topic_from(si, c, si->su->nick, CURRTIME, parv[1]);
}

static void m_ping(sourceinfo_t *si, int parc, char *parv[])
{
	/* reply to PING's */
	sts(":%s PONG %s %s", me.name, me.name, parv[0]);
}

static void m_pong(sourceinfo_t *si, int parc, char *parv[])
{
	/* someone replied to our PING */
	if ((!parv[0]) || (strcasecmp(me.actual, parv[0])))
		return;

	me.uplinkpong = CURRTIME;

	/* -> :test.projectxero.net PONG test.projectxero.net :shrike.malkier.net */
}

static void m_eob(sourceinfo_t *si, int parc, char *parv[])
{
	server_t *serv;
	char sidbuf[4+1], *p;

	handle_eob(si->s);
	if (parc >= 1)
	{
		sidbuf[4] = '\0';
		p = parv[0];
		while (p[0] && p[1] && p[2] && p[3])
		{
			memcpy(sidbuf, p, 4);
			serv = server_find(sidbuf);
			handle_eob(serv);
			if (p[4] != ',')
				break;
			p += 5;
		}
	}

	if (me.bursting)
	{
		sts(":%s EOBACK", me.numeric);
#ifdef HAVE_GETTIMEOFDAY
		e_time(burstime, &burstime);

		slog(LG_INFO, "m_eob(): finished synching with uplink (%d %s)", (tv2ms(&burstime) > 1000) ? (tv2ms(&burstime) / 1000) : tv2ms(&burstime), (tv2ms(&burstime) > 1000) ? "s" : "ms");

		wallops("Finished synchronizing with network in %d %s.", (tv2ms(&burstime) > 1000) ? (tv2ms(&burstime) / 1000) : tv2ms(&burstime), (tv2ms(&burstime) > 1000) ? "s" : "ms");
#else
		slog(LG_INFO, "m_eob(): finished synching with uplink");
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

static void m_njoin(sourceinfo_t *si, int parc, char *parv[])
{
	channel_t *c;
	unsigned int userc;
	char *userv[256];
	unsigned int i;

	c = channel_find(parv[0]);

	if (!c)
	{
		slog(LG_DEBUG, "m_njoin(): new channel: %s", parv[0]);
		/* Give channels created during burst an older "TS"
		 * so they won't be deopped -- jilles */
		c = channel_add(parv[0], si->s->flags & SF_EOB ? CURRTIME : CURRTIME - 601, si->s);
		/* if !/+ channel, we don't want to do anything with it */
		if (c == NULL)
			return;
		/* Check mode locks */
		channel_mode_va(NULL, c, 1, "+");
	}

	userc = sjtoken(parv[parc - 1], ',', userv);

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
	user_t *u;

	/* got the right number of args for an introduction? */
	if (parc == 7)
	{
		slog(LG_DEBUG, "m_nick(): new user on `%s': %s", si->s->name, parv[0]);

		u = user_add(parv[0], parv[2], parv[3], NULL, parv[4], parv[1], parv[6], si->s, 0);
		if (u == NULL)
			return;

		user_mode(u, parv[5]);
		if (strchr(parv[5], 'a'))
			handle_away(u, "Gone");

		handle_nickchange(u);
	}

	/* if it's only 1 then it's a nickname change */
	else if (parc == 1)
	{
                if (!si->su)
                {       
                        slog(LG_DEBUG, "m_nick(): server trying to change nick: %s", si->s != NULL ? si->s->name : "<none>");
                        return;
                }

		slog(LG_DEBUG, "m_nick(): nickname change from `%s': %s", si->su->nick, parv[0]);

		if (user_changenick(si->su, parv[0], 0))
			return;

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

static void m_save(sourceinfo_t *si, int parc, char *parv[])
{
	user_t *u;

	u = user_find(parv[0]);
	if (u == NULL)
		return;
	if (!strcmp(u->nick, u->uid))
	{
		slog(LG_DEBUG, "m_save(): ignoring noop SAVE message for %s", u->nick);
		return;
	}
	if (is_internal_client(u))
	{
		slog(LG_INFO, "m_save(): service %s got hit, changing back", u->nick);
		sts(":%s NICK %s", u->uid, u->nick);
		/* XXX services wars */
	}
	else
	{
		slog(LG_DEBUG, "m_save(): nickname change for `%s': %s", u->nick, u->uid);

		if (user_changenick(u, u->uid, 0))
			return;

		handle_nickchange(u);
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
	/* The following is hackish, but it works, because user MODE
	 * is not used in bursts and users are not allowed to change away
	 * status using MODE.
	 * -- jilles */
	if (*parv[0] == '#')
		channel_mode(NULL, channel_find(parv[0]), parc - 1, &parv[1]);
	else if (!strcmp(parv[1], "-a"))
		handle_away(user_find(parv[0]), NULL);
	else if (!strcmp(parv[1], "+a"))
		handle_away(user_find(parv[0]), "Gone");
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
	if (server_find(parv[0]))
		server_delete(parv[0]);
	else if (si->su != NULL)
	{
		/* XXX we don't have a list of jupes, so let's just
		 * assume it is one if we don't know it */
		slog(LG_INFO, "m_squit(): accepting SQUIT for jupe %s from %s", parv[0], si->su->nick);
		sts(":%s WALLOPS :Received SQUIT %s from %s (%s)", me.numeric, parv[0], si->su->nick, parv[1]);
		sts(":%s SQUIT %s :%s", me.numeric, parv[0], parv[1]);
	}
}

static void m_server(sourceinfo_t *si, int parc, char *parv[])
{
	slog(LG_DEBUG, "m_server(): new server: %s", parv[0]);
	handle_server(si, parv[0], parv[2], atoi(parv[1]), parv[parc - 1]);
}

static void m_smask(sourceinfo_t *si, int parc, char *parv[])
{
	slog(LG_DEBUG, "m_smask(): new masked server: %s (%s)",
			si->s->name, parv[0]);
	handle_server(si, NULL, parv[0], si->s->hops + 1, si->s->desc);
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

void _modinit(module_t * m)
{
	/* Symbol relocation voodoo. */
	server_login = &ircnet_server_login;
	introduce_nick = &ircnet_introduce_nick;
	quit_sts = &ircnet_quit_sts;
	wallops_sts = &ircnet_wallops_sts;
	join_sts = &ircnet_join_sts;
	kick = &ircnet_kick;
	msg = &ircnet_msg;
	msg_global_sts = &ircnet_msg_global_sts;
	notice_user_sts = &ircnet_notice_user_sts;
	notice_global_sts = &ircnet_notice_global_sts;
	notice_channel_sts = &ircnet_notice_channel_sts;
	/* no wallchops, ircnet ircd does not support this */
	numeric_sts = &ircnet_numeric_sts;
	kill_id_sts = &ircnet_kill_id_sts;
	part_sts = &ircnet_part_sts;
	kline_sts = &ircnet_kline_sts;
	unkline_sts = &ircnet_unkline_sts;
	topic_sts = &ircnet_topic_sts;
	mode_sts = &ircnet_mode_sts;
	ping_sts = &ircnet_ping_sts;
	ircd_on_login = &ircnet_on_login;
	ircd_on_logout = &ircnet_on_logout;
	jupe = &ircnet_jupe;
	invite_sts = &ircnet_invite_sts;

	mode_list = ircnet_mode_list;
	ignore_mode_list = ircnet_ignore_mode_list;
	status_mode_list = ircnet_status_mode_list;
	prefix_mode_list = ircnet_prefix_mode_list;
	user_mode_list = ircnet_user_mode_list;
	ignore_mode_list_size = ARRAY_SIZE(ircnet_ignore_mode_list);

	ircd = &IRCNet;

	pcommand_add("PING", m_ping, 1, MSRC_USER | MSRC_SERVER);
	pcommand_add("PONG", m_pong, 1, MSRC_SERVER);
	pcommand_add("EOB", m_eob, 0, MSRC_SERVER);
	pcommand_add("PRIVMSG", m_privmsg, 2, MSRC_USER);
	pcommand_add("NOTICE", m_notice, 2, MSRC_UNREG | MSRC_USER | MSRC_SERVER);
	pcommand_add("NJOIN", m_njoin, 2, MSRC_SERVER);
	pcommand_add("PART", m_part, 1, MSRC_USER);
	pcommand_add("NICK", m_nick, 1, MSRC_USER);
	pcommand_add("UNICK", m_nick, 7, MSRC_SERVER);
	pcommand_add("SAVE", m_save, 1, MSRC_SERVER);
	pcommand_add("QUIT", m_quit, 1, MSRC_USER);
	pcommand_add("MODE", m_mode, 2, MSRC_USER | MSRC_SERVER);
	pcommand_add("KICK", m_kick, 2, MSRC_USER | MSRC_SERVER);
	pcommand_add("KILL", m_kill, 1, MSRC_USER | MSRC_SERVER);
	pcommand_add("SQUIT", m_squit, 1, MSRC_USER | MSRC_SERVER);
	pcommand_add("SERVER", m_server, 4, MSRC_UNREG | MSRC_SERVER);
	pcommand_add("SMASK", m_smask, 2, MSRC_SERVER);
	pcommand_add("STATS", m_stats, 2, MSRC_USER);
	pcommand_add("ADMIN", m_admin, 1, MSRC_USER);
	pcommand_add("VERSION", m_version, 1, MSRC_USER);
	pcommand_add("INFO", m_info, 1, MSRC_USER);
	pcommand_add("WHOIS", m_whois, 2, MSRC_USER);
	pcommand_add("TRACE", m_trace, 1, MSRC_USER);
	pcommand_add("JOIN", m_join, 1, MSRC_USER);
	pcommand_add("PASS", m_pass, 1, MSRC_UNREG);
	pcommand_add("ERROR", m_error, 1, MSRC_UNREG | MSRC_SERVER);
	pcommand_add("TOPIC", m_topic, 2, MSRC_USER);
	pcommand_add("MOTD", m_motd, 1, MSRC_USER);

	m->mflags = MODTYPE_CORE;

	pmodule_loaded = true;
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
