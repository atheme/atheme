/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Copyright (c) 2005-2006 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains protocol support for shadowircd-based ircd.
 *
 * $Id: shadowircd.c 8301 2007-05-20 13:22:15Z jilles $
 */

#include "atheme.h"
#include "uplink.h"
#include "pmodule.h"
#include "protocol/shadowircd.h"

DECLARE_MODULE_V1("protocol/shadowircd", TRUE, _modinit, NULL, "$Id: shadowircd.c 8301 2007-05-20 13:22:15Z jilles $", "Atheme Development Group <http://www.atheme.org>");

/* *INDENT-OFF* */

ircd_t ShadowIRCd = {
        "ShadowIRCd 3.3/3.6 family",	/* IRCd name */
        "$$",                           /* TLD Prefix, used by Global. */
        FALSE,                          /* Whether or not we use IRCNet/TS6 UID */
        FALSE,                          /* Whether or not we use RCOMMAND */
        TRUE,                           /* Whether or not we support channel owners. */
        TRUE,                           /* Whether or not we support channel protection. */
        TRUE,                           /* Whether or not we support halfops. */
	FALSE,				/* Whether or not we use P10 */
	TRUE,				/* Whether or not we use vHosts. */
	CMODE_OPERONLY | CMODE_PERM,	/* Oper-only cmodes */
        CMODE_PROTECT,                  /* Integer flag for owner channel flag. */
        CMODE_PROTECT,                  /* Integer flag for protect channel flag. */
        CMODE_HALFOP,                   /* Integer flag for halfops. */
        "+u",                           /* Mode we set for owner. */
        "+u",                           /* Mode we set for protect. */
        "+h",                           /* Mode we set for halfops. */
	PROTOCOL_SHADOWIRCD,		/* Protocol type */
	CMODE_PERM,                     /* Permanent cmodes */
	"beIqd",                        /* Ban-like cmodes */
	'e',                            /* Except mchar */
	'I',                            /* Invex mchar */
	0                               /* Flags */
};

struct cmode_ shadowircd_mode_list[] = {
  { 'i', CMODE_INVITE },
  { 'm', CMODE_MOD    },
  { 'n', CMODE_NOEXT  },
  { 'p', CMODE_PRIV   },
  { 's', CMODE_SEC    },
  { 't', CMODE_TOPIC  },
  { 'c', CMODE_NOCOLOR },
  { 'M', CMODE_MODREG },
  { 'E', CMODE_REGONLY },
  { 'O', CMODE_OPERONLY },
  { 'P', CMODE_PEACE },
  { 'R', CMODE_PERM   },
  { 'S', CMODE_STRIP },
  { 'K', CMODE_NOKNOCK },
  { 'N', CMODE_STICKY },
  { '\0', 0 }
};

struct extmode shadowircd_ignore_mode_list[] = {
  { '\0', 0 }
};

struct cmode_ shadowircd_status_mode_list[] = {
  { 'u', CMODE_PROTECT },
  { 'o', CMODE_OP      },
  { 'h', CMODE_HALFOP  },
  { 'v', CMODE_VOICE   },
  { '\0', 0 }
};

struct cmode_ shadowircd_prefix_mode_list[] = {
  { '!', CMODE_PROTECT },
  { '@', CMODE_OP      },
  { '%', CMODE_HALFOP  },
  { '+', CMODE_VOICE   },
  { '\0', 0 }
};

/* *INDENT-ON* */

/* login to our uplink */
static unsigned int shadowircd_server_login(void)
{
	int ret;

	ret = sts("PASS %s :TS", curr_uplink->pass);
	if (ret == 1)
		return 1;

	me.bursting = TRUE;

	sts("CAPAB :QS EX IE KLN UNKLN ENCAP SERVICES");
	sts("SERVER %s 1 :%s", me.name, me.desc);
	sts("SVINFO 5 3 0 :%ld", CURRTIME);

	return 0;
}

/* introduce a client */
static void shadowircd_introduce_nick(user_t *u)
{
	sts("NICK %s 1 %ld +%sS %s %s %s :%s", u->nick, u->ts, "io", u->user, u->host, me.name, u->gecos);
}

/* invite a user to a channel */
static void shadowircd_invite_sts(user_t *sender, user_t *target, channel_t *channel)
{
	sts(":%s INVITE %s %s", sender->nick, target->nick, channel->name);
}

static void shadowircd_quit_sts(user_t *u, const char *reason)
{
	if (!me.connected)
		return;

	sts(":%s QUIT :%s", u->nick, reason);
}

/* WALLOPS wrapper */
static void shadowircd_wallops_sts(const char *text)
{
	sts(":%s WALLOPS :%s", chansvs.nick, text);
}

/* join a channel */
static void shadowircd_join_sts(channel_t *c, user_t *u, boolean_t isnew, char *modes)
{
	if (isnew)
		sts(":%s SJOIN %ld %s %s :@%s", me.name, c->ts, c->name,
				modes, u->nick);
	else
		sts(":%s SJOIN %ld %s + :@%s", me.name, c->ts, c->name,
				u->nick);
}

/* kicks a user from a channel */
static void shadowircd_kick(char *from, char *channel, char *to, char *reason)
{
	channel_t *chan = channel_find(channel);
	user_t *user = user_find(to);

	if (!chan || !user)
		return;

	sts(":%s KICK %s %s :%s", from, channel, to, reason);

	chanuser_delete(chan, user);
}

/* PRIVMSG wrapper */
static void shadowircd_msg(const char *from, const char *target, const char *fmt, ...)
{
	va_list ap;
	char buf[BUFSIZE];

	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZE, fmt, ap);
	va_end(ap);

	sts(":%s PRIVMSG %s :%s", from, target, buf);
}

/* NOTICE wrapper */
static void shadowircd_notice_user_sts(user_t *from, user_t *target, const char *text)
{
	sts(":%s NOTICE %s :%s", from ? from->nick : me.name, target->nick, text);
}

static void shadowircd_notice_global_sts(user_t *from, const char *mask, const char *text)
{
	sts(":%s NOTICE %s%s :%s", from ? from->nick : me.name, ircd->tldprefix, mask, text);
}

static void shadowircd_notice_channel_sts(user_t *from, channel_t *target, const char *text)
{
	sts(":%s NOTICE %s :%s", from ? from->nick : me.name, target->name, text);
}

static void shadowircd_wallchops(user_t *sender, channel_t *channel, const char *message)
{
	if (chanuser_find(channel, sender))
		sts(":%s NOTICE @%s :%s", CLIENT_NAME(sender), channel->name,
				message);
	else /* do not join for this, everyone would see -- jilles */
		generic_wallchops(sender, channel, message);
}

/* numeric wrapper */
static void shadowircd_numeric_sts(char *from, int numeric, char *target, char *fmt, ...)
{
	va_list ap;
	char buf[BUFSIZE];

	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZE, fmt, ap);
	va_end(ap);

	sts(":%s %d %s %s", from, numeric, target, buf);
}

/* KILL wrapper */
static void shadowircd_kill_id_sts(user_t *killer, const char *id, const char *reason)
{
	if (killer != NULL)
		sts(":%s KILL %s :%s!%s (%s)", killer->nick, id, killer->host, killer->nick, reason);
	else
		sts(":%s KILL %s :%s (%s)", me.name, id, me.name, reason);
}

/* PART wrapper */
static void shadowircd_part_sts(channel_t *c, user_t *u)
{
	sts(":%s PART %s", u->nick, c->name);
}

/* server-to-server KLINE wrapper */
static void shadowircd_kline_sts(char *server, char *user, char *host, long duration, char *reason)
{
	if (!me.connected)
		return;

	sts(":%s KLINE %s %ld %s %s :%s", me.name, server, duration, user, host, reason);
}

/* server-to-server UNKLINE wrapper */
static void shadowircd_unkline_sts(char *server, char *user, char *host)
{
	if (!me.connected)
		return;

	sts(":%s UNKLINE %s %s %s", opersvs.nick, server, user, host);
}

/* topic wrapper */
static void shadowircd_topic_sts(channel_t *c, const char *setter, time_t ts, time_t prevts, const char *topic)
{
	if (!me.connected || !c)
		return;

	sts(":%s TOPIC %s :%s", chansvs.nick, c->name, topic);
}

/* mode wrapper */
static void shadowircd_mode_sts(char *sender, channel_t *target, char *modes)
{
	if (!me.connected)
		return;

	sts(":%s MODE %s %s", sender, target->name, modes);
}

/* ping wrapper */
static void shadowircd_ping_sts(void)
{
	if (!me.connected)
		return;

	sts("PING :%s", me.name);
}

/* protocol-specific stuff to do on login */
static void shadowircd_on_login(char *origin, char *user, char *wantedhost)
{
	user_t *u = user_find(origin);

	if (!me.connected || u == NULL)
		return;

	/* Can only do this for nickserv, and can only record identified
	 * state if logged in to correct nick, sorry -- jilles
	 */
	if (should_reg_umode(u))
		sts(":%s MODE %s +e", me.name, origin);
}

/* protocol-specific stuff to do on login */
static boolean_t shadowircd_on_logout(char *origin, char *user, char *wantedhost)
{
	if (!me.connected)
		return FALSE;

	if (!nicksvs.no_nick_ownership)
		sts(":%s MODE %s -e", me.name, origin);

	return FALSE;
}

static void shadowircd_jupe(const char *server, const char *reason)
{
	if (!me.connected)
		return;

	server_delete(server);
	sts(":%s SQUIT %s :%s", opersvs.nick, server, reason);
	sts(":%s SERVER %s 2 :%s", me.name, server, reason);
}

static void m_topic(sourceinfo_t *si, int parc, char *parv[])
{
	channel_t *c = channel_find(parv[0]);

	if (c == NULL)
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
			cu->modes = 0;
		}

		slog(LG_DEBUG, "m_sjoin(): TS changed for %s (%ld -> %ld)", c->name, c->ts, ts);

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

		if (user_changenick(si->su, parv[0], atoi(parv[1])))
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
		slog(LG_INFO, "m_pass(): password mismatch from uplink; aborting");
		runflags |= RF_SHUTDOWN;
	}
}

static void m_error(sourceinfo_t *si, int parc, char *parv[])
{
	slog(LG_INFO, "m_error(): error from server: %s", parv[0]);
}

static void m_capab(sourceinfo_t *si, int parc, char *parv[])
{
	services_init();
}

/* :shadowircd36.qa.atheme.org SVSCLOAK nenolod moogles.are.fun */
static void m_svscloak(sourceinfo_t *si, int parc, char *parv[])
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
	server_login = &shadowircd_server_login;
	introduce_nick = &shadowircd_introduce_nick;
	quit_sts = &shadowircd_quit_sts;
	wallops_sts = &shadowircd_wallops_sts;
	join_sts = &shadowircd_join_sts;
	kick = &shadowircd_kick;
	msg = &shadowircd_msg;
	notice_user_sts = &shadowircd_notice_user_sts;
	notice_global_sts = &shadowircd_notice_global_sts;
	notice_channel_sts = &shadowircd_notice_channel_sts;
	wallchops = &shadowircd_wallchops;
	numeric_sts = &shadowircd_numeric_sts;
	kill_id_sts = &shadowircd_kill_id_sts;
	part_sts = &shadowircd_part_sts;
	kline_sts = &shadowircd_kline_sts;
	unkline_sts = &shadowircd_unkline_sts;
	topic_sts = &shadowircd_topic_sts;
	mode_sts = &shadowircd_mode_sts;
	ping_sts = &shadowircd_ping_sts;
	ircd_on_login = &shadowircd_on_login;
	ircd_on_logout = &shadowircd_on_logout;
	jupe = &shadowircd_jupe;
	invite_sts = &shadowircd_invite_sts;

	mode_list = shadowircd_mode_list;
	ignore_mode_list = shadowircd_ignore_mode_list;
	status_mode_list = shadowircd_status_mode_list;
	prefix_mode_list = shadowircd_prefix_mode_list;

	ircd = &ShadowIRCd;

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
	pcommand_add("TOPIC", m_topic, 2, MSRC_USER);
	pcommand_add("CAPAB", m_capab, 1, MSRC_UNREG);
	pcommand_add("SVSCLOAK", m_svscloak, 2, MSRC_USER | MSRC_SERVER);
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
