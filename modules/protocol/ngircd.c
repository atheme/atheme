/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2012 William Pitcock <nenolod@dereferenced.org>
 *
 * This file contains protocol support for ngircd.
 */

#include <atheme.h>
#include <atheme/protocol/ngircd.h>

static struct ircd ngIRCd = {
	.ircdname = "ngIRCd",
	.tldprefix = "$",
	.uses_uid = false,
	.uses_rcommand = false,
	.uses_owner = true,
	.uses_protect = true,
	.uses_halfops = true,
	.uses_p10 = false,
	.uses_vhost = true,
	.oper_only_modes = CMODE_OPERONLY | CMODE_PERM,
	.owner_mode = CSTATUS_OWNER,
	.protect_mode = CSTATUS_PROTECT,
	.halfops_mode = CSTATUS_HALFOP,
	.owner_mchar = "+q",
	.protect_mchar = "+a",
	.halfops_mchar = "+h",
	.type = PROTOCOL_NGIRCD,
	.perm_mode = CMODE_PERM,
	.oimmune_mode = 0,
	.ban_like_modes = "beI",
	.except_mchar = 'e',
	.invex_mchar = 'I',
	.flags = 0,
};

static const struct cmode ngircd_mode_list[] = {
  { 'i', CMODE_INVITE	},
  { 'm', CMODE_MOD	},
  { 'n', CMODE_NOEXT	},
  { 'p', CMODE_PRIV	},
  { 's', CMODE_SEC	},
  { 't', CMODE_TOPIC	},
  { 'O', CMODE_OPERONLY },
  { 'R', CMODE_REGONLY	},
  { 'r', CMODE_CHANREG	},
  { 'P', CMODE_PERM	},
  { 'z', CMODE_SSLONLY	},
  { '\0', 0 }
};

static struct extmode ngircd_ignore_mode_list[] = {
  { '\0', 0 }
};

static const struct cmode ngircd_status_mode_list[] = {
  { 'q', CSTATUS_OWNER	 },
  { 'a', CSTATUS_PROTECT },
  { 'o', CSTATUS_OP	 },
  { 'h', CSTATUS_HALFOP  },
  { 'v', CSTATUS_VOICE	 },
  { '\0', 0 }
};

static const struct cmode ngircd_prefix_mode_list[] = {
  { '~', CSTATUS_OWNER	 },
  { '&', CSTATUS_PROTECT },
  { '@', CSTATUS_OP	 },
  { '%', CSTATUS_HALFOP  },
  { '+', CSTATUS_VOICE	 },
  { '\0', 0 }
};

static const struct cmode ngircd_user_mode_list[] = {
  { 'a', UF_AWAY     },
  { 'i', UF_INVIS    },
  { 'o', UF_IRCOP    },
  { 'q', UF_IMMUNE   },
  { '\0', 0 }
};

static unsigned int
ngircd_server_login(void)
{
	int ret;

	ret = sts("PASS %s 0210-IRC+ %s|%s:CLMo", curr_uplink->send_pass, PACKAGE_TARNAME, PACKAGE_VERSION);
	if (ret == 1)
		return 1;

	me.bursting = true;

	sts("SERVER %s 1 :%s", me.name, me.desc);

	services_init();

	return 0;
}

static void
ngircd_introduce_nick(struct user *u)
{
	const char *umode = user_get_umodestr(u);

	sts(":%s NICK %s 1 %s %s 1 %s :%s", me.name, u->nick, u->user, u->host, umode, u->gecos);
}

static void
ngircd_invite_sts(struct user *sender, struct user *target, struct channel *channel)
{
	bool joined = false;

	if (!chanuser_find(channel, sender))
	{
		sts(":%s NJOIN %s :@%s", ME, channel->name, CLIENT_NAME(sender));
		joined = true;
	}

	sts(":%s INVITE %s %s", CLIENT_NAME(sender), CLIENT_NAME(target), channel->name);

	if (joined)
		sts(":%s PART %s :Invited %s", CLIENT_NAME(sender), channel->name, target->nick);
}

static void
ngircd_quit_sts(struct user *u, const char *reason)
{
	sts(":%s QUIT :%s", CLIENT_NAME(u), reason);
}

static void
ngircd_join_sts(struct channel *c, struct user *u, bool isnew, char *modes)
{
	sts(":%s NJOIN %s :@%s", ME, c->name, CLIENT_NAME(u));
	if (isnew && modes[0] && modes[1])
		sts(":%s MODE %s %s", ME, c->name, modes);
}

static void
ngircd_kick(struct user *source, struct channel *c, struct user *u, const char *reason)
{
	sts(":%s KICK %s %s :%s", CLIENT_NAME(source), c->name, CLIENT_NAME(u), reason);

	chanuser_delete(c, u);
}

static void ATHEME_FATTR_PRINTF(3, 4)
ngircd_msg(const char *from, const char *target, const char *fmt, ...)
{
	va_list ap;
	char buf[BUFSIZE];

	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZE, fmt, ap);
	va_end(ap);

	sts(":%s PRIVMSG %s :%s", from, target, buf);
}

static void
ngircd_msg_global_sts(struct user *from, const char *mask, const char *text)
{
	mowgli_node_t *n;
	struct tld *tld;

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

static void
ngircd_notice_user_sts(struct user *from, struct user *target, const char *text)
{
	sts(":%s NOTICE %s :%s", from ? CLIENT_NAME(from) : ME, CLIENT_NAME(target), text);
}

static void
ngircd_notice_global_sts(struct user *from, const char *mask, const char *text)
{
	mowgli_node_t *n;
	struct tld *tld;

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

static void
ngircd_notice_channel_sts(struct user *from, struct channel *target, const char *text)
{
	sts(":%s NOTICE %s :%s", from ? CLIENT_NAME(from) : ME, target->name, text);
}

static void ATHEME_FATTR_PRINTF(4, 5)
ngircd_numeric_sts(struct server *from, int numeric, struct user *target, const char *fmt, ...)
{
	va_list ap;
	char buf[BUFSIZE];

	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZE, fmt, ap);
	va_end(ap);

	sts(":%s %d %s %s", SERVER_NAME(from), numeric, CLIENT_NAME(target), buf);
}

static void
ngircd_kill_id_sts(struct user *killer, const char *id, const char *reason)
{
	if (killer != NULL)
		sts(":%s KILL %s :%s!%s (%s)", CLIENT_NAME(killer), id, killer->host, killer->nick, reason);
	else
		sts(":%s KILL %s :%s (%s)", ME, id, me.name, reason);
}

static void
ngircd_part_sts(struct channel *c, struct user *u)
{
	sts(":%s PART %s", CLIENT_NAME(u), c->name);
}

static void
ngircd_kline_sts(const char *server, const char *user, const char *host, long duration, const char *reason)
{
	sts(":%s GLINE %s@%s %ld :%s", ME, user, host, duration, reason);
}

static void
ngircd_unkline_sts(const char *server, const char *user, const char *host)
{
	// when sent with no extra parameters, it indicates the GLINE should be removed.
	sts(":%s GLINE %s@%s", ME, user, host);
}

static void
ngircd_topic_sts(struct channel *c, struct user *source, const char *setter, time_t ts, time_t prevts, const char *topic)
{
	bool joined = false;

	if (!chanuser_find(c, source))
	{
		sts(":%s NJOIN %s :@%s", ME, c->name, CLIENT_NAME(source));
		joined = true;
	}

	sts(":%s TOPIC %s :%s", CLIENT_NAME(source), c->name, topic);

	if (joined)
		sts(":%s PART %s :Topic set for %s", CLIENT_NAME(source), c->name, setter);
}

static void
ngircd_mode_sts(char *sender, struct channel *target, char *modes)
{
	return_if_fail(sender != NULL);
	return_if_fail(target != NULL);
	return_if_fail(modes != NULL);

	sts(":%s MODE %s %s", sender, target->name, modes);
}

static void
ngircd_ping_sts(void)
{
	sts(":%s PING :%s", me.name, me.name);
}

static void
ngircd_on_login(struct user *u, struct myuser *account, const char *wantedhost)
{
	return_if_fail(u != NULL);

	sts(":%s METADATA %s accountname :%s", me.name, CLIENT_NAME(u), entity(account)->name);

	if (should_reg_umode(u))
		sts(":%s MODE %s +R", CLIENT_NAME(nicksvs.me->me), CLIENT_NAME(u));
}

static bool
ngircd_on_logout(struct user *u, const char *account)
{
	return_val_if_fail(u != NULL, false);

	if (!nicksvs.no_nick_ownership)
		sts(":%s MODE %s -R", CLIENT_NAME(nicksvs.me->me), CLIENT_NAME(u));

	sts(":%s METADATA %s accountname :", me.name, CLIENT_NAME(u));

	return false;
}

static void
ngircd_jupe(const char *server, const char *reason)
{
	static int jupe_ctr = 1;

	server_delete(server);
	sts(":%s SQUIT %s :%s", ME, server, reason);
	sts(":%s SERVER %s 2 %d :%s", ME, server, ++jupe_ctr, reason);
}

static void
ngircd_sethost_sts(struct user *source, struct user *target, const char *host)
{
	if (strcmp(target->host, host))
	{
		sts(":%s METADATA %s cloakhost :%s", me.name, target->nick, host);
		sts(":%s MODE %s +x", me.name, target->nick);

		if (strcmp(host, target->chost))
		{
			strshare_unref(target->chost);
			target->chost = strshare_get(host);
		}
	}
	else
	{
		sts(":%s MODE %s -x", me.name, target->nick);
		sts(":%s METADATA %s cloakhost :", me.name, target->nick);

		strshare_unref(target->chost);
		target->chost = strshare_get(target->host);
	}
}

static void
m_topic(struct sourceinfo *si, int parc, char *parv[])
{
	struct channel *c = channel_find(parv[0]);

	if (!c)
		return;

	handle_topic_from(si, c, si->su != NULL ? si->su->nick : si->s->name, CURRTIME, parv[1]);
}

static void
m_ping(struct sourceinfo *si, int parc, char *parv[])
{
	// reply to PINGs
	sts(":%s PONG %s", ME, parv[0]);
}

static void
m_pong(struct sourceinfo *si, int parc, char *parv[])
{
	handle_eob(si->s);

	me.uplinkpong = CURRTIME;

	// -> :test.projectxero.net PONG test.projectxero.net :shrike.malkier.net
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
	bool realchange;

	if (parc == 7)
	{
		s = server_find(parv[4]);
		if (!s)
		{
			slog(LG_DEBUG, "m_nick(): new user on nonexistent server (token): %s", parv[4]);
			return;
		}

		slog(LG_DEBUG, "m_nick(): new user on `%s': %s", s->name, parv[0]);

		u = user_add(parv[0], parv[2], parv[3], NULL, NULL, NULL, parv[6], s, CURRTIME);
		if (u == NULL)
			return;

		user_mode(u, parv[5]);

		handle_nickchange(u);
	}
	// if it's only 1 then it's a nickname change
	else if (parc == 1)
	{
		if (!si->su)
		{
			slog(LG_DEBUG, "m_nick(): server trying to change nick: %s", si->s != NULL ? si->s->name : "<none>");
			return;
		}

		slog(LG_DEBUG, "m_nick(): nickname change from `%s': %s", si->su->nick, parv[0]);

		realchange = irccasecmp(si->su->nick, parv[0]);

		if (user_changenick(si->su, parv[0], CURRTIME))
			return;

		// fix up +r if necessary -- jilles
		if (realchange && !nicksvs.no_nick_ownership)
		{
			if (should_reg_umode(si->su))
				// changed nick to registered one, reset +r
				sts(":%s MODE %s +R", nicksvs.nick, parv[0]);
			else
				// changed from registered nick, remove +r
				sts(":%s MODE %s -R", nicksvs.nick, parv[0]);
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

static void
m_njoin(struct sourceinfo *si, int parc, char *parv[])
{
	struct channel *c;
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

		// if !/+ channel, we don't want to do anything with it
		if (c == NULL)
			return;

		// Check mode locks
		channel_mode_va(NULL, c, 1, "+");
	}

	userc = sjtoken(parv[parc - 1], ',', userv);

	for (i = 0; i < userc; i++)
		chanuser_add(c, userv[i]);

	if (c->nummembers == 0 && !(c->modes & ircd->perm_mode))
		channel_delete(c);
}

static void
m_chaninfo(struct sourceinfo *si, int parc, char *parv[])
{
	struct channel *c;
	const char *kmode, *lmode;
	bool swap;

	/* Differently from what ngircd does itself, accept all received
	 * modes and topics. This should help somewhat with resolving the
	 * resulting desyncs.
	 */

	c = channel_find(parv[0]);

	if (!c)
	{
		slog(LG_DEBUG, "m_chaninfo(): new channel: %s", parv[0]);

		/* Give channels created during burst an older "TS"
		 * so they won't be deopped -- jilles */
		c = channel_add(parv[0], si->s->flags & SF_EOB ? CURRTIME : CURRTIME - 601, si->s);

		// if !/+ channel, we don't want to do anything with it
		if (c == NULL)
			return;

		/* Hope something will happen, otherwise the channel will
		 * stick around empty.
		 */
	}

	if (parc >= 4)
	{
		// CHANINFO #channel +modes key limit [:topic]
		kmode = strchr(parv[1], 'k');
		lmode = strchr(parv[1], 'l');
		swap = kmode == NULL || (lmode != NULL && kmode > lmode);
		channel_mode_va(NULL, c, 3, parv[1], parv[swap ? 3 : 2],
				parv[swap ? 2 : 3]);
	}
	else
	{
		// CHANINFO #channel +modes [:topic]
		channel_mode_va(NULL, c, 1, parv[1]);
	}

	if (parc == 3 || parc >= 5)
		handle_topic(c, si->s->name, CURRTIME, parv[parc - 1]);
}

static void
m_quit(struct sourceinfo *si, int parc, char *parv[])
{
	slog(LG_DEBUG, "m_quit(): user leaving: %s", si->su->nick);

	// user_delete() takes care of removing channels and so forth
	user_delete(si->su, parv[0]);
}

static void
ngircd_user_mode(struct user *u, const char *modes)
{
	int dir;
	const char *p;

	return_if_fail(u != NULL);

	user_mode(u, modes);
	dir = 0;
	for (p = modes; *p; ++p)
		switch (*p)
		{
			case '-': dir = MTYPE_DEL; break;
			case '+': dir = MTYPE_ADD; break;
			case 'x':
				slog(LG_DEBUG, "user had vhost='%s' chost='%s'", u->vhost, u->chost);
				if (dir == MTYPE_ADD)
				{
					if (strcmp(u->vhost, u->chost))
					{
						strshare_unref(u->vhost);
						u->vhost = strshare_get(u->chost);
					}
				}
				else if (dir == MTYPE_DEL)
				{
					strshare_unref(u->vhost);
					u->vhost = strshare_get(u->host);
				}
				slog(LG_DEBUG, "user got vhost='%s' chost='%s'", u->vhost, u->chost);
				break;
		}
}

static void
m_mode(struct sourceinfo *si, int parc, char *parv[])
{
	if (*parv[0] == '#')
		channel_mode(NULL, channel_find(parv[0]), parc - 1, &parv[1]);
	else if (*parv[0] == '!')
		;
	else
		ngircd_user_mode(user_find(parv[0]), parv[1]);
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
		if (*parv[0] != '!')
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
	s = handle_server(si, parv[0], parc >= 4 ? parv[2] : "1",
			atoi(parv[1]), parv[parc - 1]);

	if (s != NULL && s->uplink != me.me)
	{
		/* elicit PONG for EOB detection; pinging uplink is
		 * already done elsewhere -- jilles
		 */
		sts(":%s PING %s", me.name, s->name);
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
	int chanc;
	char *chanv[256];
	int i;

	// JOIN 0 is really a part from all channels
	if (parv[0][0] == '0')
	{
		MOWGLI_ITER_FOREACH_SAFE(n, tn, si->su->channels.head)
		{
			cu = (struct chanuser *) n->data;
			chanuser_delete(cu->chan, si->su);
		}
	}
	else
	{
		chanc = sjtoken(parv[0], ',', chanv);
		for (i = 0; i < chanc; i++)
		{
			char *st = strchr(chanv[i], 0x07);
			if (st != NULL)
				*st++ = '\0';

			struct channel *c = channel_find(chanv[i]);

			if (!c)
			{
				slog(LG_DEBUG, "m_join(): new channel: %s", chanv[i]);
				c = channel_add(chanv[i], CURRTIME, si->su->server);

				/* Tell the core to check mode locks now,
				 * otherwise it may only happen after the next
				 * mode change.
				 * DreamForge does not allow any redundant modes
				 * so this will not look ugly. -- jilles
				 *
				 * If this is in a burst, a MODE with the
				 * simple modes will follow so we can skip
				 * this. -- jilles */
				if (!me.bursting)
					channel_mode_va(NULL, c, 1, "+");
			}

			chanuser_add(c, si->su->nick);

			if (st != NULL)
			{
				while (*st != '\0')
				{
					switch (*st)
					{
					case 'o':
						channel_mode_va(NULL, c, 2, "+o", si->su->nick);
						break;
					case 'v':
						channel_mode_va(NULL, c, 2, "+v", si->su->nick);
						break;
					default:
						break;
					}
					st++;
				}
			}
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
m_metadata(struct sourceinfo *si, int parc, char *parv[])
{
	struct user *u = user_find(parv[0]);

	return_if_fail(u != NULL);

	if (!strcmp(parv[1], "accountname"))
	{
		if (si->s == u->server && (!(si->s->flags & SF_EOB) ||
					   (u->myuser != NULL &&
					    !irccasecmp(entity(u->myuser)->name, parv[2]))))
			handle_burstlogin(u, parv[2], 0);
		else if (*parv[2])
			handle_setlogin(si, u, parv[2], 0);
		else
			handle_clearlogin(si, u);
	}
	else if (!strcmp(parv[1], "certfp"))
	{
		handle_certfp(si, u, parv[2]);
	}
	else if (!strcmp(parv[1], "cloakhost"))
	{
		strshare_unref(u->chost);
		u->chost = strshare_get(parv[2]);
	}
}

static void
nick_group(struct hook_user_req *hdata)
{
	struct user *u;

	u = hdata->si->su != NULL && !irccasecmp(hdata->si->su->nick, hdata->mn->nick) ? hdata->si->su : user_find_named(hdata->mn->nick);
	if (u != NULL && should_reg_umode(u))
		sts(":%s MODE %s +R", nicksvs.nick, u->nick);
}

static void
nick_ungroup(struct hook_user_req *hdata)
{
	struct user *u;

	u = hdata->si->su != NULL && !irccasecmp(hdata->si->su->nick, hdata->mn->nick) ? hdata->si->su : user_find_named(hdata->mn->nick);
	if (u != NULL && !nicksvs.no_nick_ownership)
		sts(":%s MODE %s -R", nicksvs.nick, u->nick);
}

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "transport/rfc1459")

	server_login = &ngircd_server_login;
	introduce_nick = &ngircd_introduce_nick;
	quit_sts = &ngircd_quit_sts;
	join_sts = &ngircd_join_sts;
	kick = &ngircd_kick;
	msg = &ngircd_msg;
	msg_global_sts = &ngircd_msg_global_sts;
	notice_user_sts = &ngircd_notice_user_sts;
	notice_global_sts = &ngircd_notice_global_sts;
	notice_channel_sts = &ngircd_notice_channel_sts;
	numeric_sts = &ngircd_numeric_sts;
	kill_id_sts = &ngircd_kill_id_sts;
	part_sts = &ngircd_part_sts;
	kline_sts = &ngircd_kline_sts;
	unkline_sts = &ngircd_unkline_sts;
	topic_sts = &ngircd_topic_sts;
	mode_sts = &ngircd_mode_sts;
	ping_sts = &ngircd_ping_sts;
	ircd_on_login = &ngircd_on_login;
	ircd_on_logout = &ngircd_on_logout;
	jupe = &ngircd_jupe;
	sethost_sts = &ngircd_sethost_sts;
	invite_sts = &ngircd_invite_sts;

	mode_list = ngircd_mode_list;
	ignore_mode_list = ngircd_ignore_mode_list;
	status_mode_list = ngircd_status_mode_list;
	prefix_mode_list = ngircd_prefix_mode_list;
	user_mode_list = ngircd_user_mode_list;
	ignore_mode_list_size = ARRAY_SIZE(ngircd_ignore_mode_list);

	ircd = &ngIRCd;

	pcommand_add("PING", m_ping, 1, MSRC_USER | MSRC_SERVER);
	pcommand_add("PONG", m_pong, 1, MSRC_SERVER);
	pcommand_add("PRIVMSG", m_privmsg, 2, MSRC_USER);
	pcommand_add("NOTICE", m_notice, 2, MSRC_UNREG | MSRC_USER | MSRC_SERVER);
	pcommand_add("CHANINFO", m_chaninfo, 2, MSRC_SERVER);
	pcommand_add("NJOIN", m_njoin, 2, MSRC_SERVER);
	pcommand_add("PART", m_part, 1, MSRC_USER);
	pcommand_add("NICK", m_nick, 1, MSRC_USER | MSRC_SERVER);
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
	pcommand_add("TOPIC", m_topic, 2, MSRC_USER | MSRC_SERVER);
	pcommand_add("MOTD", m_motd, 1, MSRC_USER);
	pcommand_add("METADATA", m_metadata, 3, MSRC_SERVER);
	pcommand_add("SQUERY", m_privmsg, 2, MSRC_USER);

	hook_add_nick_group(nick_group);
	hook_add_nick_ungroup(nick_ungroup);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{

}

SIMPLE_DECLARE_MODULE_V1("protocol/ngircd", MODULE_UNLOAD_CAPABILITY_NEVER)
