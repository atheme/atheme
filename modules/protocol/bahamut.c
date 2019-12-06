/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2003-2004 E. Will, et al.
 * Copyright (C) 2005-2007 Atheme Project (http://atheme.org/)
 *
 * This file contains protocol support for bahamut-based ircd.
 */

#include <atheme.h>
#include <atheme/protocol/bahamut.h>

static struct ircd Bahamut = {
	.ircdname = "Bahamut 2.1.x",
	.tldprefix = "$",
	.uses_uid = false,
	.uses_rcommand = false,
	.uses_owner = false,
	.uses_protect = false,
	.uses_halfops = false,
	.uses_p10 = false,
	.uses_vhost = false,
	.oper_only_modes = CMODE_OPERONLY,
	.owner_mode = 0,
	.protect_mode = 0,
	.halfops_mode = 0,
	.owner_mchar = "+",
	.protect_mchar = "+",
	.halfops_mchar = "+",
	.type = PROTOCOL_BAHAMUT,
	.perm_mode = 0,
	.oimmune_mode = 0,
	.ban_like_modes = "beI",
	.except_mchar = 'e',
	.invex_mchar = 'I',
	.flags = IRCD_HOLDNICK,
};

static const struct cmode bahamut_mode_list[] = {
  { 'i', CMODE_INVITE	},
  { 'm', CMODE_MOD	},
  { 'n', CMODE_NOEXT	},
  { 'p', CMODE_PRIV	},
  { 's', CMODE_SEC	},
  { 't', CMODE_TOPIC	},
  { 'c', CMODE_NOCOLOR	},
  { 'M', CMODE_MODREG	},
  { 'R', CMODE_REGONLY	},
  { 'O', CMODE_OPERONLY },
  { 'r', CMODE_CHANREG	},
  { 'A', CMODE_HIDING  },
  { 'P', CMODE_PRIVACY  },
  { '\0', 0 }
};

static const struct cmode bahamut_status_mode_list[] = {
  { 'o', CSTATUS_OP    },
  { 'v', CSTATUS_VOICE },
  { '\0', 0 }
};

static const struct cmode bahamut_prefix_mode_list[] = {
  { '@', CSTATUS_OP    },
  { '+', CSTATUS_VOICE },
  { '\0', 0 }
};

static const struct cmode bahamut_user_mode_list[] = {
  { 'A', UF_ADMIN    },
  { 'i', UF_INVIS    },
  { 'o', UF_IRCOP    },
  { '\0', 0 }
};

static bool use_nickipstr = false;

static bool
check_jointhrottle(const char *value, struct channel *c, struct mychan *mc, struct user *u, struct myuser *mu)
{
	const char *p, *arg2;
	int num, timeslice, v;

	if (!strcmp(value, "0") && u == NULL && mu == NULL)
		return true;

	p = value;
	arg2 = NULL;

	while (*p != '\0')
	{
		if (*p == ':')
		{
			if (arg2 != NULL)
				return false;
			arg2 = p + 1;
		}
		else if (!isdigit((unsigned char)*p))
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

static struct extmode bahamut_ignore_mode_list[] = {
  { 'j', check_jointhrottle },
  { '\0', 0 }
};

static unsigned int
bahamut_server_login(void)
{
	int ret;

	ret = sts("PASS %s :TS", curr_uplink->send_pass);
	if (ret == 1)
		return 1;

	me.bursting = true;

	sts("CAPAB SSJOIN NOQUIT BURST ZIP NICKIP TSMODE NICKIPSTR");
	sts("SERVER %s 1 :%s", me.name, me.desc);
	sts("SVINFO 5 3 0 :%lu", (unsigned long)CURRTIME);

	return 0;
}

static void
bahamut_introduce_nick(struct user *u)
{
	const char *umode = user_get_umodestr(u);

	if (use_nickipstr)
		sts("NICK %s 1 %lu %s %s %s %s 0 0.0.0.0 :%s", u->nick, (unsigned long)u->ts, umode, u->user, u->host, me.name, u->gecos);
	else
		sts("NICK %s 1 %lu %s %s %s %s 0 0 :%s", u->nick, (unsigned long)u->ts, umode, u->user, u->host, me.name, u->gecos);
}

static void
bahamut_invite_sts(struct user *sender, struct user *target, struct channel *channel)
{
	sts(":%s INVITE %s %s", sender->nick, target->nick, channel->name);
}

static void
bahamut_quit_sts(struct user *u, const char *reason)
{
	sts(":%s QUIT :%s", u->nick, reason);
}

static void
bahamut_wallops_sts(const char *text)
{
	sts(":%s GLOBOPS :%s", me.name, text);
}

static void
bahamut_join_sts(struct channel *c, struct user *u, bool isnew, char *modes)
{
	if (isnew)
		sts(":%s SJOIN %lu %s %s :@%s", me.name, (unsigned long)c->ts,
				c->name, modes, u->nick);
	else
		sts(":%s SJOIN %lu %s + :@%s", me.name, (unsigned long)c->ts,
				c->name, u->nick);
}

static void
bahamut_chan_lowerts(struct channel *c, struct user *u)
{
	slog(LG_DEBUG, "bahamut_chan_lowerts(): lowering TS for %s to %lu",
			c->name, (unsigned long)c->ts);
	sts(":%s SJOIN %lu %s %s :@%s", me.name, (unsigned long)c->ts, c->name,
				channel_modes(c, true), u->nick);
	chanban_clear(c);

	// Don't destroy keeptopic info, I'll admit this is ugly -- jilles
	sfree(c->topic);
	sfree(c->topic_setter);
	c->topic = NULL;
	c->topic_setter = NULL;
	c->topicts = 0;
}

static void
bahamut_kick(struct user *source, struct channel *c, struct user *u, const char *reason)
{
	sts(":%s KICK %s %s :%s", source->nick, c->name, u->nick, reason);

	chanuser_delete(c, u);
}

static void ATHEME_FATTR_PRINTF(3, 4)
bahamut_msg(const char *from, const char *target, const char *fmt, ...)
{
	va_list ap;
	char buf[BUFSIZE];

	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZE, fmt, ap);
	va_end(ap);

	sts(":%s PRIVMSG %s :%s", from, target, buf);
}

static void
bahamut_msg_global_sts(struct user *from, const char *mask, const char *text)
{
	mowgli_node_t *n;
	struct tld *tld;
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

static void
bahamut_notice_user_sts(struct user *from, struct user *target, const char *text)
{
	sts(":%s NOTICE %s :%s", from ? from->nick : me.name, target->nick, text);
}

static void
bahamut_notice_global_sts(struct user *from, const char *mask, const char *text)
{
	mowgli_node_t *n;
	struct tld *tld;

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

static void
bahamut_notice_channel_sts(struct user *from, struct channel *target, const char *text)
{
	sts(":%s NOTICE %s :%s", from ? from->nick : me.name, target->name, text);
}

static void
bahamut_wallchops(struct user *sender, struct channel *channel, const char *message)
{
	sts(":%s NOTICE @%s :%s", sender->nick, channel->name, message);
}

static void ATHEME_FATTR_PRINTF(4, 5)
bahamut_numeric_sts(struct server *from, int numeric, struct user *target, const char *fmt, ...)
{
	va_list ap;
	char buf[BUFSIZE];

	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZE, fmt, ap);
	va_end(ap);

	sts(":%s %d %s %s", from->name, numeric, target->nick, buf);
}

static void
bahamut_kill_id_sts(struct user *killer, const char *id, const char *reason)
{
	if (killer != NULL)
		sts(":%s KILL %s :%s!%s (%s)", killer->nick, id, killer->host, killer->nick, reason);
	else
		sts(":%s KILL %s :%s (%s)", me.name, id, me.name, reason);
}

static void
bahamut_part_sts(struct channel *c, struct user *u)
{
	sts(":%s PART %s", u->nick, c->name);
}

static void
bahamut_kline_sts(const char *server, const char *user, const char *host, long duration, const char *reason)
{
	struct service *svs;

	svs = service_find("operserv");
	sts(":%s AKILL %s %s %ld %s %lu :%s", me.name, host, user, duration, svs != NULL ? svs->nick : me.name, (unsigned long)CURRTIME, reason);
}

static void
bahamut_unkline_sts(const char *server, const char *user, const char *host)
{
	sts(":%s RAKILL %s %s", me.name, host, user);
}

static void
bahamut_topic_sts(struct channel *c, struct user *source, const char *setter, time_t ts, time_t prevts, const char *topic)
{
	return_if_fail(c != NULL);

	sts(":%s TOPIC %s %s %lu :%s", source->nick, c->name, setter, (unsigned long)ts, topic);
}

static void
bahamut_mode_sts(char *sender, struct channel *target, char *modes)
{
	return_if_fail(sender != NULL);
	return_if_fail(target != NULL);
	return_if_fail(modes != NULL);

	sts(":%s MODE %s %lu %s", sender, target->name, (unsigned long)target->ts, modes);
}

static void
bahamut_ping_sts(void)
{
	sts("PING :%s", me.name);
}

static void
bahamut_on_login(struct user *u, struct myuser *account, const char *wantedhost)
{
	return_if_fail(u != NULL);

	/* Can only do this for nickserv, and can only record identified
	 * state if logged in to correct nick, sorry -- jilles
	 */
	if (should_reg_umode(u))
		sts(":%s SVSMODE %s +rd %lu", nicksvs.nick, u->nick, (unsigned long)CURRTIME);
}

static bool
bahamut_on_logout(struct user *u, const char *account)
{
	return_val_if_fail(u != NULL, false);

	if (!nicksvs.no_nick_ownership)
		sts(":%s SVSMODE %s -r+d %lu", nicksvs.nick, u->nick, (unsigned long)CURRTIME);
	return false;
}

static void
bahamut_jupe(const char *server, const char *reason)
{
	struct server *s;

	sts(":%s SQUIT %s :%s", me.name, server, reason);
	s = server_find(server);
	/* If the server is not directly connected to our uplink, we
	 * need to wait for its uplink to process the SQUIT :(
	 * -- jilles */
	if (s != NULL && s->uplink != NULL && s->uplink->uplink != me.me)
		s->flags |= SF_JUPE_PENDING;
	else
		sts(":%s SERVER %s 2 :%s", me.name, server, reason);
}

static void
bahamut_fnc_sts(struct user *source, struct user *u, const char *newnick, int type)
{
	sts(":%s SVSNICK %s %s %lu", source->nick, u->nick, newnick,
			(unsigned long)(CURRTIME - SECONDS_PER_MINUTE));
}

static void
bahamut_holdnick_sts(struct user *source, int duration, const char *nick, struct myuser *mu)
{
	sts(":%s SVSHOLD %s %d :Reserved by %s for nickname owner (%s)",
			source->nick, nick, duration, source->nick,
			mu ? entity(mu)->name : nick);
}

static void
m_topic(struct sourceinfo *si, int parc, char *parv[])
{
	struct channel *c = channel_find(parv[0]);

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

static void
m_ping(struct sourceinfo *si, int parc, char *parv[])
{
	// reply to PINGs
	sts(":%s PONG %s %s", me.name, me.name, parv[0]);
}

static void
m_pong(struct sourceinfo *si, int parc, char *parv[])
{
	struct server *s;

	// someone replied to our PING
	if (!parv[0])
		return;
	s = server_find(parv[0]);
	if (s == NULL)
		return;

	// Postpone EOB for our uplink until topic burst is also done
	if (s->uplink != me.me)
		handle_eob(s);

	if (s != si->s)
		return;

	me.uplinkpong = CURRTIME;

	// -> :test.projectxero.net PONG test.projectxero.net :shrike.malkier.net
}

static void
m_burst(struct sourceinfo *si, int parc, char *parv[])
{
	struct server *s;

	// Ignore "BURST" at start of burst
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

static void
m_privmsg(struct sourceinfo *si, int parc, char *parv[])
{
	if (parc != 2)
		return;

	handle_message(si, parv[0], false, parv[1]);
}

static void
m_notice(struct sourceinfo *si, int parc, char *parv[])
{
	if (parc != 2)
		return;

	handle_message(si, parv[0], true, parv[1]);
}

static void
m_sjoin(struct sourceinfo *si, int parc, char *parv[])
{
	/*
	 *  -> :proteus.malkier.net SJOIN 1073516550 #shrike +tn :@sycobuny @+rakaur
	 *	also:
	 *  -> :nenolod_ SJOIN 1117334567 #chat
	 */

	struct channel *c;
	bool keep_new_modes = true;
	unsigned int userc;
	char *userv[256];
	unsigned int i;
	time_t ts;
	char *p;

	if (parc >= 4 && si->s != NULL)
	{
		// :origin SJOIN ts chan modestr [key or limits] :users
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
			struct chanuser *cu;
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
				cu = (struct chanuser *)n->data;
				if (cu->user->server == me.me)
				{
					// it's a service, reop
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

static void
m_part(struct sourceinfo *si, int parc, char *parv[])
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

static void
m_nick(struct sourceinfo *si, int parc, char *parv[])
{
	struct server *s;
	struct user *u;
	char ipstring[64];
	bool realchange;

	// -> NICK jilles 1 1136143909 +oi ~jilles 192.168.1.5 jaguar.test 0 3232235781 :Jilles Tjoelker
	if (parc == 10)
	{
		s = server_find(parv[6]);
		if (!s)
		{
			slog(LG_DEBUG, "m_nick(): new user on nonexistent server: %s", parv[6]);
			return;
		}

		slog(LG_DEBUG, "m_nick(): new user on `%s': %s", s->name, parv[0]);

		if (!use_nickipstr)
		{
			struct in_addr ip;

			ip.s_addr = ntohl(strtoul(parv[8], NULL, 10));
			ipstring[0] = '\0';
			if (!inet_ntop(AF_INET, &ip, ipstring, sizeof ipstring))
				ipstring[0] = '\0';
		}
		else
			mowgli_strlcpy(ipstring, parv[8], sizeof ipstring);

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

	// if it's only 2 then it's a nickname change
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

		// fix up +r if necessary -- jilles
		if (realchange && should_reg_umode(si->su))
			// changed nick to registered one, reset +r
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

static void
m_quit(struct sourceinfo *si, int parc, char *parv[])
{
	slog(LG_DEBUG, "m_quit(): user leaving: %s", si->su->nick);

	// user_delete() takes care of removing channels and so forth
	user_delete(si->su, parv[0]);
}

static void
m_mode(struct sourceinfo *si, int parc, char *parv[])
{
	struct channel *c;

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

static void
m_kick(struct sourceinfo *si, int parc, char *parv[])
{
	struct user *u = user_find(parv[1]);
	struct channel *c = channel_find(parv[0]);

	// -> :rakaur KICK #shrike rintaun :test
	slog(LG_DEBUG, "m_kick(): user was kicked: %s -> %s", parv[1], parv[0]);

	if (!u)
	{
		slog(LG_DEBUG, "m_kick(): got kick for nonexistent user %s", parv[1]);
		return;
	}

	if (!c)
	{
		slog(LG_DEBUG, "m_kick(): got kick in nonexistent channel: %s", parv[0]);
		return;
	}

	if (!chanuser_find(c, u))
	{
		slog(LG_DEBUG, "m_kick(): got kick for %s not in %s", u->nick, c->name);
		return;
	}

	chanuser_delete(c, u);

	// if they kicked us, let's rejoin
	if (is_internal_client(u))
	{
		slog(LG_DEBUG, "m_kick(): %s got kicked from %s; rejoining", u->nick, parv[0]);
		join(parv[0], u->nick);
	}
}

static void
m_kill(struct sourceinfo *si, int parc, char *parv[])
{
	handle_kill(si, parv[0], parc > 1 ? parv[1] : "<No reason given>");
}

static void
m_squit(struct sourceinfo *si, int parc, char *parv[])
{
	slog(LG_DEBUG, "m_squit(): server leaving: %s from %s", parv[0], parv[1]);
	server_delete(parv[0]);
}

static void
m_server(struct sourceinfo *si, int parc, char *parv[])
{
	struct server *s;

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

static void
m_stats(struct sourceinfo *si, int parc, char *parv[])
{
	handle_stats(si->su, parv[0][0]);
}

static void
m_admin(struct sourceinfo *si, int parc, char *parv[])
{
	handle_admin(si->su);
}

static void
m_version(struct sourceinfo *si, int parc, char *parv[])
{
	handle_version(si->su);
}

static void
m_info(struct sourceinfo *si, int parc, char *parv[])
{
	handle_info(si->su);
}

static void
m_whois(struct sourceinfo *si, int parc, char *parv[])
{
	handle_whois(si->su, parv[1]);
}

static void
m_trace(struct sourceinfo *si, int parc, char *parv[])
{
	handle_trace(si->su, parv[0], parc >= 2 ? parv[1] : NULL);
}

static void
m_away(struct sourceinfo *si, int parc, char *parv[])
{
	handle_away(si->su, parc >= 1 ? parv[0] : NULL);
}

static void
m_join(struct sourceinfo *si, int parc, char *parv[])
{
	struct chanuser *cu;
	mowgli_node_t *n, *tn;

	// JOIN 0 is really a part from all channels
	if (parv[0][0] == '0')
	{
		MOWGLI_ITER_FOREACH_SAFE(n, tn, si->su->channels.head)
		{
			cu = (struct chanuser *)n->data;
			chanuser_delete(cu->chan, si->su);
		}
	}
}

static void
m_pass(struct sourceinfo *si, int parc, char *parv[])
{
	if (curr_uplink->receive_pass != NULL &&
	    strcmp(curr_uplink->receive_pass, parv[0]))
	{
		slog(LG_INFO, "m_pass(): password mismatch from uplink; aborting");
		runflags |= RF_SHUTDOWN;
	}
}

static void
m_error(struct sourceinfo *si, int parc, char *parv[])
{
	slog(LG_INFO, "m_error(): error from server: %s", parv[0]);
}

static void
m_motd(struct sourceinfo *si, int parc, char *parv[])
{
	handle_motd(si->su);
}

static void
m_capab(struct sourceinfo *si, int parc, char *parv[])
{
	int i;

	use_nickipstr = false;

	for (i = 0; i < parc; i++)
	{
		if (!irccasecmp(parv[i], "NICKIPSTR"))
		{
			slog(LG_DEBUG, "m_capab(): uplink supports string-based IP addresses, enabling support.");
			use_nickipstr = true;
		}
	}

	// now burst services with NICKIP set correctly.
	services_init();
}

static void
nick_group(struct hook_user_req *hdata)
{
	struct user *u;

	u = hdata->si->su != NULL && !irccasecmp(hdata->si->su->nick, hdata->mn->nick) ? hdata->si->su : user_find_named(hdata->mn->nick);
	if (u != NULL && should_reg_umode(u))
		sts(":%s SVSMODE %s +rd %lu", nicksvs.nick, u->nick, (unsigned long)CURRTIME);
}

static void
nick_ungroup(struct hook_user_req *hdata)
{
	struct user *u;

	u = hdata->si->su != NULL && !irccasecmp(hdata->si->su->nick, hdata->mn->nick) ? hdata->si->su : user_find_named(hdata->mn->nick);
	if (u != NULL && !nicksvs.no_nick_ownership)
		sts(":%s SVSMODE %s -r+d %lu", nicksvs.nick, u->nick, (unsigned long)CURRTIME);
}

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "transport/rfc1459")

	server_login = &bahamut_server_login;
	introduce_nick = &bahamut_introduce_nick;
	quit_sts = &bahamut_quit_sts;
	wallops_sts = &bahamut_wallops_sts;
	join_sts = &bahamut_join_sts;
	chan_lowerts = &bahamut_chan_lowerts;
	kick = &bahamut_kick;
	msg = &bahamut_msg;
	msg_global_sts = &bahamut_msg_global_sts;
	notice_user_sts = &bahamut_notice_user_sts;
	notice_global_sts = &bahamut_notice_global_sts;
	notice_channel_sts = &bahamut_notice_channel_sts;
	wallchops = &bahamut_wallchops;
	numeric_sts = &bahamut_numeric_sts;
	kill_id_sts = &bahamut_kill_id_sts;
	part_sts = &bahamut_part_sts;
	kline_sts = &bahamut_kline_sts;
	unkline_sts = &bahamut_unkline_sts;
	topic_sts = &bahamut_topic_sts;
	mode_sts = &bahamut_mode_sts;
	ping_sts = &bahamut_ping_sts;
	ircd_on_login = &bahamut_on_login;
	ircd_on_logout = &bahamut_on_logout;
	jupe = &bahamut_jupe;
	fnc_sts = &bahamut_fnc_sts;
	invite_sts = &bahamut_invite_sts;
	holdnick_sts = &bahamut_holdnick_sts;

	mode_list = bahamut_mode_list;
	ignore_mode_list = bahamut_ignore_mode_list;
	status_mode_list = bahamut_status_mode_list;
	prefix_mode_list = bahamut_prefix_mode_list;
	user_mode_list = bahamut_user_mode_list;
	ignore_mode_list_size = ARRAY_SIZE(bahamut_ignore_mode_list);

	ircd = &Bahamut;

	pcommand_add("CAPAB", m_capab, 0, MSRC_UNREG);
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

	hook_add_nick_group(nick_group);
	hook_add_nick_ungroup(nick_ungroup);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{

}

SIMPLE_DECLARE_MODULE_V1("protocol/bahamut", MODULE_UNLOAD_CAPABILITY_NEVER)
