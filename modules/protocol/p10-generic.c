/*
 * Copyright (c) 2005-2006 William Pitcock, et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains protocol support for P10 ircd's.
 * Some sources used: Run's documentation, beware's description,
 * raw data sent by asuka.
 *
 */

#include "atheme.h"
#include "uplink.h"
#include "pmodule.h"

DECLARE_MODULE_V1("protocol/p10-generic", true, _modinit, NULL, PACKAGE_STRING, "Atheme Development Group <http://www.atheme.org>");

static void check_hidehost(user_t *u);

/* login to our uplink */
static unsigned int p10_server_login(void)
{
	int ret;

	ret = sts("PASS :%s", curr_uplink->pass);
	if (ret == 1)
		return 1;

	me.bursting = true;

	/* SERVER irc.p10.org 1 933022556 947908144 J10 AA]]] :[127.0.0.1] A Undernet Server */
	sts("SERVER %s 1 %lu %lu J10 %s]]] +s6 :%s", me.name, (unsigned long)me.start, (unsigned long)CURRTIME, me.numeric, me.desc);

	services_init();

	sts("%s EB", me.numeric);

	return 0;
}

/* introduce a client */
static void p10_introduce_nick(user_t *u)
{
	const char *umode = user_get_umodestr(u);

	sts("%s N %s 1 %lu %s %s %sk ]]]]]] %s :%s", me.numeric, u->nick, (unsigned long)u->ts, u->user, u->host, umode, u->uid, u->gecos);
}

/* invite a user to a channel */
static void p10_invite_sts(user_t *sender, user_t *target, channel_t *channel)
{
	/* target is a nick, weird eh? -- jilles */
	sts("%s I %s %s", sender->uid, target->nick, channel->name);
}

static void p10_quit_sts(user_t *u, const char *reason)
{
	if (!me.connected)
		return;

	sts("%s Q :%s", u->uid, reason);
}

/* WALLOPS wrapper */
static void p10_wallops_sts(const char *text)
{
	sts("%s WA :%s", me.numeric, text);
}

/* join a channel */
static void p10_join_sts(channel_t *c, user_t *u, bool isnew, char *modes)
{
	/* If the channel doesn't exist, we need to create it. */
	if (isnew)
	{
		sts("%s C %s %lu", u->uid, c->name, (unsigned long)c->ts);
		if (modes[0] && modes[1])
			sts("%s M %s %s", u->uid, c->name, modes);
	}
	else
	{
		sts("%s J %s %lu", u->uid, c->name, (unsigned long)c->ts);
		sts("%s M %s +o %s", me.numeric, c->name, u->uid);
	}
}

static void p10_chan_lowerts(channel_t *c, user_t *u)
{
	slog(LG_DEBUG, "p10_chan_lowerts(): lowering TS for %s to %lu",
			c->name, (unsigned long)c->ts);
	sts("%s B %s %lu %s %s:o", me.numeric, c->name, (unsigned long)c->ts,
			channel_modes(c, true), u->uid);
	chanban_clear(c);
}

/* kicks a user from a channel */
static void p10_kick(user_t *source, channel_t *c, user_t *u, const char *reason)
{
	if (chanuser_find(c, source))
		sts("%s K %s %s :%s", source->uid, c->name, u->uid, reason);
	else
		sts("%s K %s %s :%s", me.numeric, c->name, u->uid, reason);

	chanuser_delete(c, u);
}

/* PRIVMSG wrapper */
static void p10_msg(const char *from, const char *target, const char *fmt, ...)
{
	va_list ap;
	user_t *u = user_find_named(from);
	char buf[BUFSIZE];

	if (!u)
		return;

	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZE, fmt, ap);
	va_end(ap);

	sts("%s P %s :%s", u->uid, target, buf);
}

static void p10_msg_global_sts(user_t *from, const char *mask, const char *text)
{
	mowgli_node_t *n;
	tld_t *tld;

	if (!strcmp(mask, "*"))
	{
		MOWGLI_ITER_FOREACH(n, tldlist.head)
		{
			tld = n->data;
			sts("%s P %s*%s :%s", from ? from->uid : me.numeric, ircd->tldprefix, tld->name, text);
		}
	}
	else
		sts("%s P %s%s :%s", from ? from->uid : me.numeric, ircd->tldprefix, mask, text);
}

/* NOTICE wrapper */
static void p10_notice_user_sts(user_t *from, user_t *target, const char *text)
{
	sts("%s O %s :%s", from ? from->uid : me.numeric, target->uid, text);
}

static void p10_notice_global_sts(user_t *from, const char *mask, const char *text)
{
	mowgli_node_t *n;
	tld_t *tld;

	if (!strcmp(mask, "*"))
	{
		MOWGLI_ITER_FOREACH(n, tldlist.head)
		{
			tld = n->data;
			sts("%s O %s*%s :%s", from ? from->uid : me.numeric, ircd->tldprefix, tld->name, text);
		}
	}
	else
		sts("%s O %s%s :%s", from ? from->uid : me.numeric, ircd->tldprefix, mask, text);
}

static void p10_notice_channel_sts(user_t *from, channel_t *target, const char *text)
{
	if (from == NULL || chanuser_find(target, from))
		sts("%s O %s :%s", from ? from->uid : me.numeric, target->name, text);
	else
		sts("%s O %s :[%s:%s] %s", me.numeric, target->name, from->nick, target->name, text);
}

static void p10_wallchops(user_t *sender, channel_t *channel, const char *message)
{
	sts("%s WC %s :%s", sender->uid, channel->name, message);
}

/* numeric wrapper */
static void p10_numeric_sts(server_t *from, int numeric, user_t *target, const char *fmt, ...)
{
	va_list ap;
	char buf[BUFSIZE];

	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZE, fmt, ap);
	va_end(ap);

	sts("%s %d %s %s", from->sid, numeric, target->uid, buf);
}

/* KILL wrapper */
static void p10_kill_id_sts(user_t *killer, const char *id, const char *reason)
{
	if (killer != NULL)
		sts("%s D %s :%s!%s (%s)", killer->uid, id, killer->host, killer->nick, reason);
	else
		sts("%s D %s :%s (%s)", me.numeric, id, me.name, reason);
}

/* PART wrapper */
static void p10_part_sts(channel_t *c, user_t *u)
{
	sts("%s L %s", u->uid, c->name);
}

/* server-to-server KLINE wrapper */
static void p10_kline_sts(const char *server, const char *user, const char *host, long duration, const char *reason)
{
	if (!me.connected)
		return;

	/* hold permanent akills for four weeks -- jilles */
	sts("%s GL * +%s@%s %ld :%s", me.numeric, user, host, duration > 0 ? duration : 2419200, reason);
}

/* server-to-server UNKLINE wrapper */
static void p10_unkline_sts(const char *server, const char *user, const char *host)
{
	if (!me.connected)
		return;

	sts("%s GL * -%s@%s", me.numeric, user, host);
}

static void p10_xline_sts(const char *server, const char *realname, long duration, const char *reason)
{
	if (!me.connected)
		return;

	/* hold permanent sglines for four weeks -- jilles */
	sts("%s GL * +$R%s %ld :%s", me.numeric, realname, duration > 0 ? duration : 2419200, reason);
}

static void p10_unxline_sts(const char *server, const char *realname)
{
	if (!me.connected)
		return;

	sts("%s GL * -$R%s", me.numeric, realname);
}

static void p10_qline_sts(const char *server, const char *name, long duration, const char *reason)
{
	if (!me.connected)
		return;

	if (*name != '#' && *name != '&')
	{
		slog(LG_INFO, "SQLINE: Could not set SQLINE on \2%s\2, not supported by ircu.", name);
		return;
	}

	/* hold permanent sqlines for four weeks -- jilles */
	sts("%s GL * +%s %ld :%s", me.numeric, name, duration > 0 ? duration : 2419200, reason);
}

static void p10_unqline_sts(const char *server, const char *name)
{
	if (!me.connected)
		return;

	if (*name != '#' && *name != '&')
	{
		slog(LG_INFO, "SQLINE: Could not remove SQLINE on \2%s\2, not supported by ircu.", name);
		return;
	}

	sts("%s GL * -%s", me.numeric, name);
}

/* topic wrapper */
static void p10_topic_sts(channel_t *c, user_t *source, const char *setter, time_t ts, time_t prevts, const char *topic)
{
	if (!me.connected || !c)
		return;

	if (ts > prevts || prevts == 0)
		sts("%s T %s %lu %lu :%s", source->uid, c->name, (unsigned long)c->ts, (unsigned long)ts, topic);
	else
	{
		ts = CURRTIME;
		if (ts < prevts)
			ts = prevts + 1;
		sts("%s T %s %lu %lu :%s", source->uid, c->name, (unsigned long)c->ts, (unsigned long)ts, topic);
		c->topicts = ts;
	}
}

/* mode wrapper */
static void p10_mode_sts(char *sender, channel_t *target, char *modes)
{
	user_t *fptr = user_find_named(sender);

	if (!fptr)
		return;

	if (chanuser_find(target, fptr))
		sts("%s M %s %s", fptr->uid, target->name, modes);
	else
		sts("%s M %s %s", me.numeric, target->name, modes);
}

/* ping wrapper */
static void p10_ping_sts(void)
{
	if (!me.connected)
		return;

	sts("%s G !%lu %s %lu", me.numeric, (unsigned long)CURRTIME, me.name, (unsigned long)CURRTIME);
}

/* protocol-specific stuff to do on login */
static void p10_on_login(user_t *u, myuser_t *mu, const char *wantedhost)
{
	if (!me.connected || u == NULL)
		return;

	sts("%s AC %s %s %lu", me.numeric, u->uid, entity(mu)->name,
			(unsigned long)mu->registered);
	check_hidehost(u);
}

/* P10 does not support logout, so kill the user
 * we can't keep track of which logins are stale and which aren't -- jilles */
static bool p10_on_logout(user_t *u, const char *account)
{
	if (!me.connected || u == NULL)
		return false;

	if (u != NULL)
	{
		kill_user(NULL, u, "Forcing logout %s -> %s", u->nick, account);
		return true;
	}
	else
		return false;
}

static void p10_jupe(const char *server, const char *reason)
{
	server_t *s;

	if (!me.connected)
		return;

	/* hold it for a day (arbitrary) -- jilles */
	/* get rid of local deactivation too */
	s = server_find(server);
	if (s != NULL && s->uplink != NULL)
		sts("%s JU %s +%s %d %lu :%s", me.numeric, s->uplink->sid, server, 86400, (unsigned long)CURRTIME, reason);
	sts("%s JU * +%s %d %lu :%s", me.numeric, server, 86400, (unsigned long)CURRTIME, reason);
}

static void m_topic(sourceinfo_t *si, int parc, char *parv[])
{
	channel_t *c = channel_find(parv[0]);
	char *source;
	time_t ts = 0;

	if (!c)
		return;

	if (si->s != NULL)
		source = si->s->name;
	else
		source = si->su->nick;

	if (parc > 2)
		ts = atoi(parv[parc - 2]);
	if (ts == 0)
		ts = CURRTIME;
	else if (c->topic != NULL && ts < c->topicts)
		return;
	handle_topic_from(si, c, source, ts, parv[parc - 1]);
}

/* AB G !1119920789.573932 services.atheme.org 1119920789.573932 */
static void m_ping(sourceinfo_t *si, int parc, char *parv[])
{
	/* reply to PING's */
	if (si->su != NULL)
		sts("%s Z %s %s", me.numeric, me.numeric, si->su->nick);
	else if (parv[0][0] != '!')
		sts("%s Z %s %s", me.numeric, me.numeric, si->s->name);
	else
		sts("%s Z %s %s", me.numeric, me.numeric, parv[0]);
}

static void m_pong(sourceinfo_t *si, int parc, char *parv[])
{
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

static void m_create(sourceinfo_t *si, int parc, char *parv[])
{
	char buf[BUFSIZE];
	int chanc;
	char *chanv[256];
	int i;
	time_t ts;

	chanc = sjtoken(parv[0], ',', chanv);

	for (i = 0; i < chanc; i++)
	{
		channel_t *c = channel_add(chanv[i], ts = atoi(parv[1]), si->su->server);

		/* Tell the core to check mode locks now,
		 * otherwise it may only happen after the next
		 * mode change.
		 * P10 does not allow any redundant modes
		 * so this will not look ugly. -- jilles */
		channel_mode_va(NULL, c, 1, "+");

		if (ts <= c->ts)
		{
			buf[0] = '@';
			buf[1] = '\0';
		}
		else
			buf[0] = '\0';

		strlcat(buf, si->su->uid, BUFSIZE);

		chanuser_add(c, buf);
	}
}

static void m_join(sourceinfo_t *si, int parc, char *parv[])
{
	int chanc;
	char *chanv[256];
	int i;
	mowgli_node_t *n, *tn;
	chanuser_t *cu;

	/* JOIN 0 is really a part from all channels */
	if (!strcmp(parv[0], "0"))
	{
		MOWGLI_ITER_FOREACH_SAFE(n, tn, si->su->channels.head)
		{
			cu = (chanuser_t *) n->data;
			chanuser_delete(cu->chan, si->su);
		}
		return;
	}
	if (parc < 2)
		return;

	chanc = sjtoken(parv[0], ',', chanv);

	for (i = 0; i < chanc; i++)
	{
		channel_t *c = channel_find(chanv[i]);

		if (!c)
		{
			c = channel_add(chanv[i], atoi(parv[1]), si->su->server);
			channel_mode_va(NULL, c, 1, "+");
		}

		chanuser_add(c, si->su->uid);
	}
}

static void m_burst(sourceinfo_t *si, int parc, char *parv[])
{
	channel_t *c;
	unsigned int modec;
	char *modev[16];
	unsigned int userc;
	char *userv[256];
	unsigned int i;
	int j;
	char prefix[16];
	char newnick[16+NICKLEN];
	char *p;
	time_t ts;
	bool keep_new_modes = true;

	/* S BURST <channel> <ts> [parameters]
	 * parameters can be:
	 * +<simple mode>
	 * %<bans separated with spaces>
	 * <nicks>
	 */
	ts = atoi(parv[1]);

	c = channel_find(parv[0]);

	if (c == NULL)
	{
		slog(LG_DEBUG, "m_burst(): new channel: %s", parv[0]);
		c = channel_add(parv[0], ts, si->s);
	}
	if (ts < c->ts)
	{
		chanuser_t *cu;
		mowgli_node_t *n;

		clear_simple_modes(c);
		chanban_clear(c);
		handle_topic_from(si, c, "", 0, "");
		MOWGLI_ITER_FOREACH(n, c->members.head)
		{
			cu = (chanuser_t *)n->data;
			if (cu->user->server == me.me)
			{
				/* it's a service, reop */
				sts("%s M %s +o %s", me.numeric, c->name, CLIENT_NAME(cu->user));
				cu->modes = CSTATUS_OP;
			}
			else
				cu->modes = 0;
		}

		slog(LG_DEBUG, "m_burst(): TS changed for %s (%lu -> %lu)", c->name, (unsigned long)c->ts, (unsigned long)ts);
		c->ts = ts;
		hook_call_channel_tschange(c);
	}
	else if (ts > c->ts)
		keep_new_modes = false;
	if (parc < 3 || parv[2][0] != '+')
	{
		/* Tell the core to check mode locks now,
		 * otherwise it may only happen after the next
		 * mode change. -- jilles */
		channel_mode_va(NULL, c, 1, "+");
	}

	j = 2;
	while (j < parc)
	{
		if (parv[j][0] == '+')
		{
			modec = 0;
			modev[modec++] = parv[j++];
			if (strchr(modev[0], 'k') && j < parc)
				modev[modec++] = parv[j++];
			if (strchr(modev[0], 'l') && j < parc)
				modev[modec++] = parv[j++];
			if (keep_new_modes)
				channel_mode(NULL, c, modec, modev);
		}
		else if (parv[j][0] == '%')
		{
			userc = sjtoken(parv[j++] + 1, ' ', userv);
			if (keep_new_modes)
				for (i = 0; i < userc; i++)
					chanban_add(c, userv[i], 'b');
		}
		else
		{
			userc = sjtoken(parv[j++], ',', userv);

			prefix[0] = '\0';
			for (i = 0; i < userc; i++)
			{
				p = strchr(userv[i], ':');
				if (p != NULL)
				{
					*p = '\0';
					prefix[0] = '\0';
					prefix[1] = '\0';
					prefix[2] = '\0';
					p++;
					if (keep_new_modes)
						while (*p)
						{
							if (*p == 'o' || (*p >= '0' && *p <= '9' && !prefix[0]))
								prefix[prefix[0] ? 1 : 0] = '@';
							else if (*p == 'v')
								prefix[prefix[0] ? 1 : 0] = '+';
							p++;
						}
				}
				strlcpy(newnick, prefix, sizeof newnick);
				strlcat(newnick, userv[i], sizeof newnick);
				chanuser_add(c, newnick);
			}
		}
	}

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
	char ipstring[HOSTIPLEN];
	char *p;

	/* got the right number of args for an introduction? */
	if (parc >= 8)
	{
		/* -> AB N jilles 1 1137687480 jilles jaguar.test +oiwgrx jilles B]AAAB ABAAE :Jilles Tjoelker */
		/* -> AB N test4 1 1137690148 jilles jaguar.test +iw B]AAAB ABAAG :Jilles Tjoelker */
		slog(LG_DEBUG, "m_nick(): new user on `%s': %s", si->s->name, parv[0]);

		decode_p10_ip(parv[parc - 3], ipstring);
		u = user_add(parv[0], parv[3], parv[4], NULL, ipstring, parv[parc - 2], parv[parc - 1], si->s, atoi(parv[2]));
		if (u == NULL)
			return;

		if (parv[5][0] == '+')
		{
			user_mode(u, parv[5]);
			if (strchr(parv[5], 'r'))
			{
				p = strchr(parv[6], ':');
				if (p != NULL)
					*p++ = '\0';
				handle_burstlogin(u, parv[6], p ? atol(p) : 0);
				/* killed to force logout? */
				if (user_find(parv[parc - 2]) == NULL)
					return;
			}
			if (strchr(parv[5], 'x'))
			{
				u->flags |= UF_HIDEHOSTREQ;
				/* this must be after setting the account name */
				check_hidehost(u);
			}
		}

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
		slog(LG_DEBUG, "m_nick(): got NICK with wrong (%d) number of params", parc);

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
	user_t *u;
	channel_t *c;
	int i;
	char *p;
	int dir = MTYPE_ADD;
	time_t ts;

	if (*parv[0] == '#')
	{
		c = channel_find(parv[0]);
		if (c == NULL)
			return;
		i = 2;
		for (p = parv[1]; *p != '\0'; p++)
		{
			switch (*p)
			{
				case '+': dir = MTYPE_ADD; break;
				case '-': dir = MTYPE_DEL; break;
				case 'l':
					  if (dir == MTYPE_DEL)
						  break;
					  /* FALLTHROUGH */
				case 'b': case 'k': case 'o': case 'v':
					  i++;
			}
		}
		if (i < parc && (ts = atoi(parv[i])) != 0)
		{
			if (ts > c->ts)
			{
				slog(LG_DEBUG, "m_mode(): ignoring mode on %s (%lu > %lu)", c->name, (unsigned long)ts, (unsigned long)c->ts);
				return;
			}
		}
		channel_mode(NULL, c, parc - 1, &parv[1]);
	}
	else
	{
		/* Yes this is a nick and not a UID -- jilles */
		u = user_find_named(parv[0]);
		if (u == NULL)
		{
			slog(LG_DEBUG, "m_mode(): user mode for unknown user %s", parv[0]);
			return;
		}
		user_mode(u, parv[1]);
		if (strchr(parv[1], 'x'))
		{
			u->flags |= UF_HIDEHOSTREQ;
			check_hidehost(u);
		}
	}
}

static void m_clearmode(sourceinfo_t *si, int parc, char *parv[])
{
	channel_t *chan;
	char *p, c;
	mowgli_node_t *n;
	chanuser_t *cu;
	int i;

	/* -> ABAAA CM # b */
	/* Note: this is an IRCop command, do not enforce mode locks. */
	chan = channel_find(parv[0]);
	if (chan == NULL)
	{
		slog(LG_DEBUG, "m_clearmode(): unknown channel %s", parv[0]);
		return;
	}
	p = parv[1];
	while ((c = *p++))
	{
		if (c == 'b')
			chanban_clear(chan);
		else if (c == 'k')
		{
			if (chan->key)
				free(chan->key);
			chan->key = NULL;
		}
		else if (c == 'l')
			chan->limit = 0;
		else if (c == 'o')
		{
			MOWGLI_ITER_FOREACH(n, chan->members.head)
			{
				cu = (chanuser_t *)n->data;
				if (cu->user->server == me.me)
				{
					/* it's a service, reop */
					sts("%s M %s +o %s", me.numeric,
							chan->name,
							cu->user->uid);
				}
				else
					cu->modes &= ~CSTATUS_OP;
			}
		}
		else if (c == 'v')
		{
			MOWGLI_ITER_FOREACH(n, chan->members.head)
			{
				cu = (chanuser_t *)n->data;
				cu->modes &= ~CSTATUS_VOICE;
			}
		}
		else
			for (i = 0; mode_list[i].mode != '\0'; i++)
			{
				if (c == mode_list[i].mode)
					chan->modes &= ~mode_list[i].value;
			}
	}
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

/* SERVER ircu.devel.atheme.org 1 1119902586 1119908830 J10 ABAP] + :lets lol */
static void m_server(sourceinfo_t *si, int parc, char *parv[])
{
	server_t *s;

	/* We dont care about the max connections. */
	parv[5][2] = '\0';

	slog(LG_DEBUG, "m_server(): new server: %s, id %s, %s",
			parv[0], parv[5],
			parv[4][0] == 'P' ? "eob" : "bursting");
	s = handle_server(si, parv[0], parv[5], atoi(parv[1]), parv[7]);

	/* SF_EOB may only be set when we have all users on the server.
	 * so store the fact that they are EOB in another flag.
	 * handle_eob() will set SF_EOB when the uplink has finished bursting.
	 * -- jilles */
	if (s != NULL && parv[4][0] == 'P')
		s->flags |= SF_EOB2;
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

static void m_motd(sourceinfo_t *si, int parc, char *parv[])
{
	handle_motd(si->su);
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

static void m_eos(sourceinfo_t *si, int parc, char *parv[])
{
	handle_eob(si->s);

	/* acknowledge a local END_OF_BURST */
	if (si->s->uplink == me.me)
		sts("%s EA", me.numeric);
}

static void m_account(sourceinfo_t *si, int parc, char *parv[])
{
	user_t *u;

	u = user_find(parv[0]);
	if (u == NULL)
		return;
	handle_setlogin(si, u, parv[1], parc > 2 ? atol(parv[2]) : 0);
}

static void check_hidehost(user_t *u)
{
	static bool warned = false;

	/* do they qualify? */
	if (!(u->flags & UF_HIDEHOSTREQ) || u->myuser == NULL || (u->myuser->flags & MU_WAITAUTH))
		return;
	/* don't use this if they have some other kind of vhost */
	if (strcmp(u->host, u->vhost))
	{
		slog(LG_DEBUG, "check_hidehost(): +x overruled by other vhost for %s", u->nick);
		return;
	}
	if (me.hidehostsuffix == NULL)
	{
		if (!warned)
		{
			wallops("Misconfiguration: serverinfo::hidehostsuffix not set");
			warned = true;
		}
		return;
	}
	snprintf(u->vhost, sizeof u->vhost, "%s.%s", entity(u->myuser)->name,
			me.hidehostsuffix);
	slog(LG_DEBUG, "check_hidehost(): %s -> %s", u->nick, u->vhost);
}

void _modinit(module_t * m)
{
	/* Symbol relocation voodoo. */
	server_login = &p10_server_login;
	introduce_nick = &p10_introduce_nick;
	quit_sts = &p10_quit_sts;
	wallops_sts = &p10_wallops_sts;
	join_sts = &p10_join_sts;
	chan_lowerts = &p10_chan_lowerts;
	kick = &p10_kick;
	msg = &p10_msg;
	msg_global_sts = &p10_msg_global_sts;
	notice_user_sts = &p10_notice_user_sts;
	notice_global_sts = &p10_notice_global_sts;
	notice_channel_sts = &p10_notice_channel_sts;
	wallchops = &p10_wallchops;
	numeric_sts = &p10_numeric_sts;
	kill_id_sts = &p10_kill_id_sts;
	part_sts = &p10_part_sts;
	kline_sts = &p10_kline_sts;
	unkline_sts = &p10_unkline_sts;
	xline_sts = &p10_xline_sts;
	unxline_sts = &p10_unxline_sts;
	qline_sts = &p10_qline_sts;
	unqline_sts = &p10_unqline_sts;
	topic_sts = &p10_topic_sts;
	mode_sts = &p10_mode_sts;
	ping_sts = &p10_ping_sts;
	ircd_on_login = &p10_on_login;
	ircd_on_logout = &p10_on_logout;
	jupe = &p10_jupe;
	invite_sts = &p10_invite_sts;

	parse = &p10_parse;

	pcommand_add("G", m_ping, 1, MSRC_USER | MSRC_SERVER);
	pcommand_add("Z", m_pong, 1, MSRC_SERVER);
	pcommand_add("P", m_privmsg, 2, MSRC_USER);
	pcommand_add("O", m_notice, 2, MSRC_USER | MSRC_SERVER);
	pcommand_add("NOTICE", m_notice, 2, MSRC_UNREG);
	pcommand_add("C", m_create, 1, MSRC_USER);
	pcommand_add("J", m_join, 1, MSRC_USER);
	pcommand_add("EB", m_eos, 0, MSRC_SERVER);
	pcommand_add("B", m_burst, 2, MSRC_SERVER);
	pcommand_add("L", m_part, 1, MSRC_USER);
	pcommand_add("N", m_nick, 2, MSRC_USER | MSRC_SERVER);
	pcommand_add("Q", m_quit, 1, MSRC_USER);
	pcommand_add("M", m_mode, 2, MSRC_USER | MSRC_SERVER);
	pcommand_add("OM", m_mode, 2, MSRC_USER); /* OPMODE, treat as MODE */
	pcommand_add("CM", m_clearmode, 2, MSRC_USER);
	pcommand_add("K", m_kick, 2, MSRC_USER | MSRC_SERVER);
	pcommand_add("OK", m_kick, 2, MSRC_USER); /* OPKICK, treat as KICK */
	pcommand_add("D", m_kill, 1, MSRC_USER | MSRC_SERVER);
	pcommand_add("SQ", m_squit, 1, MSRC_USER | MSRC_SERVER);
	pcommand_add("S", m_server, 8, MSRC_SERVER);
	pcommand_add("SERVER", m_server, 8, MSRC_UNREG);
	pcommand_add("R", m_stats, 2, MSRC_USER);
	pcommand_add("AD", m_admin, 1, MSRC_USER);
	pcommand_add("V", m_version, 1, MSRC_USER);
	pcommand_add("F", m_info, 1, MSRC_USER);
	pcommand_add("W", m_whois, 2, MSRC_USER);
	pcommand_add("TR", m_trace, 1, MSRC_USER);
	pcommand_add("A", m_away, 0, MSRC_USER);
	pcommand_add("PASS", m_pass, 1, MSRC_UNREG);
	pcommand_add("Y", m_error, 1, MSRC_UNREG | MSRC_SERVER);
	pcommand_add("ERROR", m_error, 1, MSRC_UNREG | MSRC_SERVER);
	pcommand_add("T", m_topic, 2, MSRC_USER | MSRC_SERVER);
	pcommand_add("MO", m_motd, 1, MSRC_USER);
	pcommand_add("AC", m_account, 2, MSRC_SERVER);

	m->mflags = MODTYPE_CORE;
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
