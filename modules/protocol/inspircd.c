/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2012 Atheme Project (http://atheme.org/)
 *
 * InspIRCd link support
 */

#include <atheme.h>
#include <atheme/protocol/inspircd.h>

#define PROTOCOL_MINIMUM 1202 // we do not support anything older than this
#define PROTOCOL_PREFERRED_STR "1202"

static struct ircd InspIRCd = {
	.ircdname = "InspIRCd",
	.tldprefix = "$",
	.uses_uid = true,
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
	.type = PROTOCOL_INSPIRCD,
	.perm_mode = CMODE_PERM,
	.oimmune_mode = 0,
	.ban_like_modes = "beIgXw",
	.except_mchar = 'e',
	.invex_mchar = 'I',
	.flags = IRCD_CIDR_BANS | IRCD_HOLDNICK,
};

static const struct cmode inspircd_mode_list[] = {
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
  { 'S', CMODE_STRIP	},
  { 'K', CMODE_NOKNOCK	},
  { 'A', CMODE_NOINVITE },
  { 'C', CMODE_NOCTCP	},
  { 'N', CMODE_STICKY	},
  { 'G', CMODE_CENSOR	},
  { 'P', CMODE_PERM	},
  { 'B', CMODE_NOCAPS	},
  { 'z', CMODE_SSLONLY	},
  { 'T', CMODE_NONOTICE },
  { 'u', CMODE_HIDING	},
  { 'Q', CMODE_PEACE	},
  { 'D', CMODE_DELAYJOIN },
  { '\0', 0 }
};

static const struct cmode inspircd_status_mode_list[] = {
  { 'Y', CSTATUS_IMMUNE	 },
  { 'q', CSTATUS_OWNER	 },
  { 'a', CSTATUS_PROTECT },
  { 'o', CSTATUS_OP	 },
  { 'h', CSTATUS_HALFOP  },
  { 'v', CSTATUS_VOICE	 },
  { '\0', 0 }
};

static const struct cmode inspircd_prefix_mode_list[] = {
  { '!', CSTATUS_IMMUNE	 },
  { '~', CSTATUS_OWNER	 },
  { '&', CSTATUS_PROTECT },
  { '@', CSTATUS_OP	 },
  { '%', CSTATUS_HALFOP  },
  { '+', CSTATUS_VOICE	 },
  { '\0', 0 }
};

static const struct cmode inspircd_user_mode_list[] = {
  { 'i', UF_INVIS    },
  { 'o', UF_IRCOP    },
  { 'd', UF_DEAF     },
  { 'k', UF_IMMUNE   },
  { '\0', 0 }
};

static unsigned int max_rejoindelay = 5;

static bool has_hideopermod = false;
static bool has_servicesmod = false;
static bool has_globopsmod = false;
static bool has_chghostmod = false;
static bool has_cbanmod = false;
static bool has_hidechansmod = false;
static bool has_servprotectmod = false;
static bool has_svshold = false;
static bool has_cloakingmod = false;
static bool has_shun = false;
static bool has_svstopic_topiclock = false;
static int has_protocol = 0;

static mowgli_node_t *
inspircd_next_matching_ban(struct channel *c, struct user *u, int type, mowgli_node_t *first)
{
	struct chanban *cb;
	mowgli_node_t *n;
	char hostbuf[NICKLEN + 1 + USERLEN + 1 + HOSTLEN + 1];
	char realbuf[NICKLEN + 1 + USERLEN + 1 + HOSTLEN + 1];
	char ipbuf[NICKLEN + 1 + USERLEN + 1 + HOSTLEN + 1];
	char *p;

	snprintf(hostbuf, sizeof hostbuf, "%s!%s@%s", u->nick, u->user, u->vhost);
	snprintf(realbuf, sizeof realbuf, "%s!%s@%s", u->nick, u->user, u->host);

	// will be nick!user@ if ip unknown, doesn't matter
	snprintf(ipbuf, sizeof ipbuf, "%s!%s@%s", u->nick, u->user, u->ip);

	bool check_realhost = (config_options.masks_through_vhost || u->host == u->vhost);

	MOWGLI_ITER_FOREACH(n, first)
	{
		struct channel *target_c;

		cb = n->data;

		if (cb->type != type)
			continue;

		if (!match(cb->mask, hostbuf))
			return n;
		if (check_realhost && (!match(cb->mask, realbuf) || !match(cb->mask, ipbuf) || !match_cidr(cb->mask, ipbuf)))
			return n;

		if (cb->mask[1] == ':' && strchr("MRUjrm", cb->mask[0]))
		{
			bool matched = false;

			p = cb->mask + 2;
			if (*(p - 1) != ':')
				p = NULL;

			switch (cb->mask[0])
			{
			case 'M':
			case 'R':
				matched = u->myuser != NULL && !(u->myuser->flags & MU_WAITAUTH) && (p == NULL || !match(p, entity(u->myuser)->name));
				break;
			case 'U':
				matched = u->myuser == NULL;
				break;
			case 'j':
				if (p == NULL)
					continue;
				target_c = channel_find(p);
				if (target_c == NULL || (target_c->modes & (CMODE_PRIV | CMODE_SEC)))
					continue;
				matched = chanuser_find(target_c, u) != NULL;
				break;
			case 'r':
				if (p == NULL)
					continue;
				matched = !match(p, u->gecos);
				break;
			case 'm':
				matched = !match(p, hostbuf);
				if (check_realhost && !matched)
					matched = !match(p, realbuf) || !match(p, ipbuf) || !match_cidr(p, ipbuf);
				break;
			default:
				continue;
			}

			if (matched)
				return n;
		}
	}

	return NULL;
}

static bool
inspircd_is_extban(const char *mask)
{
	const size_t mask_len = strlen(mask);

	// e.g R:Test
	if (mask_len < 2 || mask[1] != ':' || strchr(mask, ' '))
		return false;

	return true;
}

static struct server *
sid_find(const char *name)
{
	char sid[4];
	mowgli_strlcpy(sid, name, 4);
	return server_find(sid);
}

static inline void
channel_metadata_sts(struct channel *c, const char *key, const char *value)
{
	sts(":%s METADATA %s %s :%s", ME, c->name, key, value);
}

static bool
check_jointhrottle(const char *value, struct channel *c, struct mychan *mc, struct user *u, struct myuser *mu)
{
	const char *p, *arg2;

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
	if (p - arg2 > 10 || arg2 - value - 1 > 10 || !atoi(value) || !atoi(arg2))
		return false;
	return true;
}

static bool
check_flood(const char *value, struct channel *c, struct mychan *mc, struct user *u, struct myuser *mu)
{
	return check_jointhrottle((*value == '*' ? value + 1 : value), c, mc, u, mu);
}

static bool
check_nickflood(const char *value, struct channel *c, struct mychan *mc, struct user *u, struct myuser *mu)
{
	return check_jointhrottle(value, c, mc, u, mu);
}

static bool
check_history(const char *value, struct channel *c, struct mychan *mc, struct user *u, struct myuser *mu)
{
	return check_jointhrottle(value, c, mc, u, mu);
}

static bool
check_forward(const char *value, struct channel *c, struct mychan *mc, struct user *u, struct myuser *mu)
{
	struct channel *target_c;
	struct mychan *target_mc;

	if (!VALID_GLOBAL_CHANNEL_PFX(value) || strlen(value) > 50)
		return false;
	if (u == NULL && mu == NULL)
		return true;
	target_c = channel_find(value);
	target_mc = mychan_from(target_c);
	if (target_c == NULL && target_mc == NULL)
		return false;
	return true;
}

static bool
check_rejoindelay(const char *value, struct channel *c, struct mychan *mc, struct user *u, struct myuser *mu)
{
	const char *ch = value;

	while (*ch)
	{
		if (!isdigit((unsigned char)*ch))
			return false;
		ch++;
	}

	if (atoi(value) <= 0 || atoi(value) > (int) max_rejoindelay)
	{
		return false;
	}
	else
	{
		return true;
	}
}

static bool
check_delaymsg(const char *value, struct channel *c, struct mychan *mc, struct user *u, struct myuser *mu)
{
	const char *ch = value;

	while (*ch)
	{
		if (!isdigit((unsigned char)*ch))
			return false;
		ch++;
	}

	if (atoi(value) <= 0)
	{
		return false;
	}
	else
	{
		return true;
	}
}

static struct extmode inspircd_ignore_mode_list[] = {
  { 'f', check_flood },
  { 'F', check_nickflood },
  { 'j', check_jointhrottle },
  { 'L', check_forward },
  { 'J', check_rejoindelay },
  { 'd', check_delaymsg },
  { 'H', check_history },
  { '\0', 0 }
};

static void
inspircd_send_fjoin(struct channel *c, struct user *u, char *modes)
{
	sts(":%s FJOIN %s %lu %s :o,%s", me.numeric, c->name, (unsigned long)c->ts, modes, u->uid);
}

static unsigned int
inspircd_server_login(void)
{
	int ret;

	// Check if we have a numeric set. InspIRCd 1.2 protocol requires it. -nenolod
	if (me.numeric == NULL)
	{
		slog(LG_ERROR, "inspircd_server_login(): inspircd requires a unique identifier. set serverinfo::numeric.");
		exit(EXIT_FAILURE);
	}

	// will be determined in CAPAB.
	ircd->uses_owner = false;
	ircd->uses_protect = false;
	ircd->uses_halfops = false;

	ret = sts("CAPAB START " PROTOCOL_PREFERRED_STR);
	if (ret == 1)
		return 1;
	sts("CAPAB CAPABILITIES :PROTOCOL=" PROTOCOL_PREFERRED_STR);
	sts("CAPAB END");
	sts("SERVER %s %s 0 %s :%s", me.name, curr_uplink->send_pass, me.numeric, me.desc);

	me.bursting = true;
	return 0;
}

static void
inspircd_introduce_nick(struct user *u)
{
	// :penguin.omega.org.za UID 497AAAAAB 1188302517 OperServ 127.0.0.1 127.0.0.1 OperServ +s 127.0.0.1 :Operator Server
	const char *umode = user_get_umodestr(u);
	const bool send_oper = (is_ircop(u) && !has_servprotectmod);

	sts(":%s UID %s %lu %s %s %s %s 0.0.0.0 %lu %s%s%s%s :%s", me.numeric, u->uid, (unsigned long)u->ts, u->nick, u->host, u->host, u->user, (unsigned long)u->ts, umode, (send_oper && has_hideopermod) ? "H" : "", has_hidechansmod ? "I" : "", has_servprotectmod ? "k" : "", u->gecos);
	if (send_oper)
		sts(":%s OPERTYPE Service", u->uid);
}

static void
inspircd_quit_sts(struct user *u, const char *reason)
{
	sts(":%s QUIT :%s", u->uid, reason);
}

static void
inspircd_wallops_sts(const char *text)
{
	if (has_globopsmod)
		sts(":%s SNONOTICE g :%s", me.numeric, text);
	else
		sts(":%s SNONOTICE A :%s", me.numeric, text);
}

static void
inspircd_join_sts(struct channel *c, struct user *u, bool isnew, char *modes)
{
	if (!isnew || !modes[0])
		modes = "+";

	inspircd_send_fjoin(c, u, modes);
}

static void
inspircd_chan_lowerts(struct channel *c, struct user *u)
{
	slog(LG_DEBUG, "inspircd_chan_lowerts(): lowering TS for %s to %lu",
		c->name, (unsigned long)c->ts);

	inspircd_send_fjoin(c, u, channel_modes(c, true));
}

static void
inspircd_kick(struct user *source, struct channel *c, struct user *u, const char *reason)
{
	sts(":%s KICK %s %s :%s", source->uid, c->name, u->uid, reason);

	chanuser_delete(c, u);
}

static void ATHEME_FATTR_PRINTF(3, 4)
inspircd_msg(const char *from, const char *target, const char *fmt, ...)
{
	va_list ap;
	char buf[BUFSIZE];
	struct user *user = user_find(target);
	struct user *from_p = user_find(from);

	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZE, fmt, ap);
	va_end(ap);

	sts(":%s PRIVMSG %s :%s", from_p->uid, !VALID_GLOBAL_CHANNEL_PFX(target) ? user->uid : target, buf);
}

static void
inspircd_msg_global_sts(struct user *from, const char *mask, const char *text)
{
	sts(":%s PRIVMSG %s%s :%s", from ? from->uid : me.numeric, ircd->tldprefix, mask, text);
}

static void
inspircd_notice_user_sts(struct user *from, struct user *target, const char *text)
{
	sts(":%s NOTICE %s :%s", from ? from->uid : me.numeric, target->uid, text);
}

static void
inspircd_notice_global_sts(struct user *from, const char *mask, const char *text)
{
	sts(":%s NOTICE %s%s :%s", from ? from->uid : me.numeric, ircd->tldprefix, mask, text);
}

static void
inspircd_notice_channel_sts(struct user *from, struct channel *target, const char *text)
{
	sts(":%s NOTICE %s :%s", from ? from->uid : me.numeric, target->name, text);
}

static void ATHEME_FATTR_PRINTF(4, 5)
inspircd_numeric_sts(struct server *from, int numeric, struct user *target, const char *fmt, ...)
{
	va_list ap;
	char buf[BUFSIZE];

	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZE, fmt, ap);
	va_end(ap);

	sts(":%s PUSH %s ::%s %d %s %s", from->sid, target->uid, from->name, numeric, target->nick, buf);
}

static void
inspircd_kill_id_sts(struct user *killer, const char *id, const char *reason)
{
	if (killer != NULL)
		sts(":%s KILL %s :Killed (%s (%s))", CLIENT_NAME(killer), id, killer->nick, reason);
	else
		sts(":%s KILL %s :Killed (%s (%s))", ME, id, me.name, reason);
}

static void
inspircd_part_sts(struct channel *c, struct user *u)
{
	sts(":%s PART %s :Leaving", u->uid, c->name);
}

static void
inspircd_kline_sts(const char *server, const char *user, const char *host, long duration, const char *reason)
{
	struct service *svs;

	// :services-dev.chatspike.net ADDLINE G test@test.com Brain 1133994664 0 :You are banned from this network
	svs = service_find("operserv");
	sts(":%s ADDLINE G %s@%s %s %lu %ld :%s", me.numeric, user, host, svs != NULL ? svs->nick : me.name, (unsigned long)CURRTIME, duration, reason);
}

static void
inspircd_unkline_sts(const char *server, const char *user, const char *host)
{
	struct service *svs;

	svs = service_find("operserv");
	sts(":%s DELLINE G %s@%s", svs != NULL ? svs->me->uid : ME, user, host);
}

static void
inspircd_qline_sts(const char *server, const char *name, long duration, const char *reason)
{
	struct service *svs;

	svs = service_find("operserv");

	if (!VALID_GLOBAL_CHANNEL_PFX(name))
	{
		sts(":%s ADDLINE Q %s %s %lu %ld :%s", me.numeric, name, svs != NULL ? svs->nick : me.name, (unsigned long)CURRTIME, duration, reason);
		return;
	}

	if (has_cbanmod)
		sts(":%s CBAN %s %ld :%s", svs != NULL ? svs->me->uid : ME, name, duration, reason);
	else
		slog(LG_INFO, "SQLINE: Could not set SQLINE on \2%s\2 due to m_cban not being loaded in inspircd.", name);
}

static void
inspircd_unqline_sts(const char *server, const char *name)
{
	if (!VALID_GLOBAL_CHANNEL_PFX(name))
	{
		sts(":%s DELLINE Q %s", ME, name);
		return;
	}

	if (has_cbanmod)
		sts(":%s CBAN %s", ME, name);
	else
		slog(LG_INFO, "SQLINE: Could not remove SQLINE on \2%s\2 due to m_cban not being loaded in inspircd.", name);
}

static void
inspircd_dline_sts(const char *server, const char *host, long duration, const char *reason)
{
	struct service *svs;

	svs = service_find("operserv");
	sts(":%s ADDLINE Z %s %s %lu %ld :%s", me.numeric, host, svs != NULL ? svs->nick : me.name, (unsigned long)CURRTIME, duration, reason);
}

static void
inspircd_undline_sts(const char *server, const char *host)
{
	struct service *svs;

	svs = service_find("operserv");
	sts(":%s DELLINE Z %s", svs != NULL ? svs->me->uid : ME, host);
}

static void
inspircd_topic_sts(struct channel *c, struct user *source, const char *setter, time_t ts, time_t prevts, const char *topic)
{
	return_if_fail(c != NULL);

	// if we have SVSTOPIC, then we don't have to deal with any of the below, so don't bother.
	if (has_svstopic_topiclock)
	{
		sts(":%s SVSTOPIC %s %lu %s :%s", ME, c->name, (unsigned long)ts, setter, topic);
		return;
	}

	/* If possible, try to use FTOPIC
	 * Note that because TOPIC does not contain topicTS, it may be
	 * off a few seconds on other servers, hence the 60 seconds here.
	 * -- jilles */

	// Restoring old topic
	if (ts > prevts + SECONDS_PER_MINUTE || prevts == 0)
	{
		sts(":%s FTOPIC %s %lu %s :%s", source->uid, c->name, (unsigned long)ts, setter, topic);
		return;
	}
	// Tweaking a topic
	else if (ts == prevts)
	{
		ts -= SECONDS_PER_MINUTE;
		sts(":%s FTOPIC %s %lu %s :%s", source->uid, c->name, (unsigned long)ts, setter, topic);
		c->topicts = ts;
		return;
	}
	sts(":%s TOPIC %s :%s", source->uid, c->name, topic);
	c->topicts = CURRTIME;
}

static void
inspircd_mode_sts(char *sender, struct channel *target, char *modes)
{
	struct user *sender_p;

	return_if_fail(sender != NULL);
	return_if_fail(target != NULL);
	return_if_fail(modes != NULL);

	sender_p = user_find(sender);

	return_if_fail(sender_p != NULL);

	sts(":%s FMODE %s %lu %s", sender_p->uid, target->name, (unsigned long)target->ts, modes);
}

static void
inspircd_ping_sts(void)
{
	// XXX this is annoying, struct uplink contains no sid or link to struct server
	struct server *u = server_find(curr_uplink->name);

	if (!u)
		return;

	sts(":%s PING %s :%s", me.numeric, me.numeric, u->sid);
}

static void
inspircd_on_login(struct user *u, struct myuser *mu, const char *wantedhost)
{
	sts(":%s METADATA %s accountname :%s", me.numeric, u->uid, entity(mu)->name);
}

static bool
inspircd_on_logout(struct user *u, const char *account)
{
	sts(":%s METADATA %s accountname :", me.numeric, u->uid);
	return false;
}

static void
inspircd_jupe(const char *server, const char *reason)
{
	struct service *svs;
	static char sid[3+1];
	int i;
	struct server *s;

	svs = service_find("operserv");

	s = server_find(server);
	if (s != NULL)
	{
		// We need to wait for the RSQUIT to be processed -- jilles
		sts(":%s RSQUIT :%s", svs != NULL ? svs->me->uid : ME, server);
		s->flags |= SF_JUPE_PENDING;
		return;
	}

	// Remove any previous jupe first
	sts(":%s SQUIT %s :%s", me.numeric, server, reason);

	// dirty dirty make up some sid
	if (sid[0] == '\0')
		mowgli_strlcpy(sid, me.numeric, sizeof sid);
	do
	{
		i = 2;
		for (;;)
		{
			if (sid[i] == 'Z')
			{
				sid[i] = '0';
				i--;

				// eek, no more sids
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

	sts(":%s SERVER %s * 1 %s :%s", me.numeric, server, sid, reason);
}

static void
inspircd_sethost_sts(struct user *source, struct user *target, const char *host)
{
	if (has_chghostmod)
	{
		sts(":%s CHGHOST %s %s", source->uid, target->uid, host);

		if (has_cloakingmod && !irccasecmp(target->host, host))
			sts(":%s MODE %s +x", source->uid, target->uid);
	}
	else
		slog(LG_INFO, "VHOST: Could not set \2%s\2 due to m_chghost not being loaded in inspircd.", host);
}

static void
inspircd_fnc_sts(struct user *source, struct user *u, const char *newnick, int type)
{
	// svsnick can only be sent by a server
	sts(":%s SVSNICK %s %s %lu", me.numeric, u->uid, newnick,
		(unsigned long)(CURRTIME - SECONDS_PER_MINUTE));
}

static void
inspircd_invite_sts(struct user *sender, struct user *target, struct channel *channel)
{
	sts(":%s INVITE %s %s", sender->uid, target->uid, channel->name);
}

static void
inspircd_holdnick_sts(struct user *source, int duration, const char *nick, struct myuser *account)
{
	struct service *svs;

	svs = service_find("operserv");
	if (duration == 0)
	{
		if (has_svshold)
			// remove SVSHOLD
			sts(":%s SVSHOLD %s", source->uid, nick);
		else
			sts(":%s QLINE %s", svs != NULL ? svs->me->uid : ME, nick);
	}
	else
	{
		if (has_svshold)
			sts(":%s SVSHOLD %s %d :Registered nickname.", source->uid, nick, duration);
		else
			sts(":%s ADDLINE Q %s %s %lu %d :%s", me.numeric, nick, svs != NULL ? svs->nick : me.name, (unsigned long)CURRTIME, duration, "Nickname Enforcer");
	}
}

static void
inspircd_svslogin_sts(const char *target, const char *nick, const char *user, const char *host, struct myuser *account)
{
	sts(":%s METADATA %s accountname :%s", me.numeric, target, entity(account)->name);

	// If host is a "*" that means no change requested
	if ((has_chghostmod) && (strcmp(host, "*")))
		sts(":%s ENCAP %c%c%c CHGHOST %s %s", me.numeric, target[0], target[1], target[2], target, host);
}

static void
inspircd_sasl_sts(const char *target, char mode, const char *data)
{
	struct service *svs;
	struct server *s = sid_find(target);

	return_if_fail(s != NULL);

	svs = service_find("saslserv");
	if (svs == NULL)
		return;

	sts(":%s ENCAP %s SASL %s %s %c %s", ME, s->sid, svs->me->uid, target, mode, data);
}

static void
inspircd_sasl_mechlist_sts(const char *mechlist)
{
	sts(":%s METADATA * saslmechlist :%s", ME, mechlist);
}

static void
inspircd_quarantine_sts(struct user *source, struct user *victim, long duration, const char *reason)
{
	if (has_shun)
		sts(":%s ADDLINE SHUN *@%s %s %lu %ld :%s", me.numeric, victim->host, source->nick, (unsigned long) CURRTIME, duration, reason);
}

static void
inspircd_mlock_sts(struct channel *c)
{
	struct mychan *mc = mychan_from(c);

	if (mc == NULL)
		return;

	channel_metadata_sts(c, "mlock", mychan_get_sts_mlock(mc));
}

static void
inspircd_topiclock_sts(struct channel *c)
{
	struct mychan *mc = mychan_from(c);
	if (mc == NULL || !has_svstopic_topiclock)
		return;

	channel_metadata_sts(c, "topiclock", (mc->flags & MC_TOPICLOCK ? "1" : ""));
}

static void
m_topic(struct sourceinfo *si, int parc, char *parv[])
{
	struct channel *c = channel_find(parv[0]);

	if (!c)
		return;

	handle_topic_from(si, c, si->su->nick, time(NULL), parv[1]);
}

static void
m_ftopic(struct sourceinfo *si, int parc, char *parv[])
{
	struct channel *c = channel_find(parv[0]);
	time_t ts = atol(parv[1]);

	if (!c)
		return;

	if (c->topic != NULL && c->topicts >= ts)
	{
		slog(LG_DEBUG, "m_ftopic(): ignoring older topic on %s", c->name);
		return;
	}

	handle_topic_from(si, c, parv[2], ts, parv[3]);
}

static void
m_ping(struct sourceinfo *si, int parc, char *parv[])
{
	// reply to PINGs
	if (parc == 1)
		sts(":%s PONG %s", me.numeric, parv[0]);
	else if (parc == 2)
		sts(":%s PONG %s :%s", me.numeric, parv[1], parv[0]);
}

static void
m_pong(struct sourceinfo *si, int parc, char *parv[])
{
	struct server *s;

	if (!parv[1])
		return;
	s = server_find(parv[0]);
	if (!s || s == me.me)
	{
		TAINT_ON(s = server_find(parv[1]), "inspircd bug #90 causes possible state desync -- upgrade your software");
		if (!s || s == me.me)
			return;
	}

	handle_eob(s);

	me.uplinkpong = CURRTIME;

	// if pong source isn't origin, this isn't a complete burst. --nenolod
	if (s != si->s)
		return;

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
	if (!me.recvsvr)
	{
		slog(LG_ERROR, "m_notice(): received NOTICE from uplink which is in unregistered state.");
		slog(LG_ERROR, "m_notice(): this probably means that you are linking to a client port instead");
		slog(LG_ERROR, "m_notice(): of a server port on your inspircd server.");
		slog(LG_ERROR, "m_notice(): atheme is giving up now.  please correct your configuration and try again.");
		exit(EXIT_FAILURE);
	}

	if (parc != 2)
		return;

	handle_message(si, parv[0], true, parv[1]);
}

static void
map_a_prefix(char prefix, char* prefixandnick, unsigned int *nlen)
{
	size_t j, k;

	// does this char match a known prefix?
	for (j = 0; status_mode_list[j].mode; j++)
	{
		// yup. add it to the 'final' combination (@%w00t)
		if (prefix == status_mode_list[j].mode)
		{
			for (k = 0; prefix_mode_list[k].mode; k++)
			{
				if (status_mode_list[j].value == prefix_mode_list[k].value)
				{
					prefixandnick[*nlen] = prefix_mode_list[k].mode;
					(*nlen)++;
					return;
				}
			}
		}
	}
}

static void
m_fjoin(struct sourceinfo *si, int parc, char *parv[])
{
	// :08X FJOIN #flaps 1234 +nt vh,0F8XXXXN ,08XGH75C ,001CCCC3 aq,00ABBBB1
	struct channel *c;
	unsigned int userc;
	unsigned int i;
	unsigned int nlen;
	bool prefix = true;
	bool keep_new_modes = true;
	char *userv[256];
	char prefixandnick[51];
	time_t ts;

	c = channel_find(parv[0]);
	ts = atol(parv[1]);

	if (!c)
	{
		slog(LG_DEBUG, "m_fjoin(): new channel: %s", parv[0]);
		c = channel_add(parv[0], ts, si->s);
		return_if_fail(c != NULL);
	}

	if (ts < c->ts)
	{
		struct chanuser *cu;
		mowgli_node_t *n;

		/* the TS changed.  a TS change requires us to do
		 * bugger all except update the TS, because in InspIRCd
		 * remote servers enforce the TS change - Brain
		 *
		 * This is no longer the case with 1.1, we need to remove modes etc
		 * as well as lowering the channel ts. Do both. -- w00t
		 */
		clear_simple_modes(c);
		chanban_clear(c);

		// Also reop services, and remove status from others.
		MOWGLI_ITER_FOREACH(n, c->members.head)
		{
			cu = (struct chanuser *)n->data;
			if (cu->user->server == me.me)
			{
				// it's a service, reop
				sts(":%s FMODE %s %lu +o %s", me.numeric, c->name, (unsigned long)ts, cu->user->uid);
				cu->modes = CSTATUS_OP;
			}
			else
				cu->modes = 0;
		}

		c->ts = ts;
		hook_call_channel_tschange(c);
	}
	else if (ts > c->ts)
	{
		keep_new_modes = false; // ignore statuses
	}

	/* ok, here's the difference from 1.0 -> 1.1:
	 * 1.0 sent p[3] and up as individual users, prefixed with their 'highest' prefix, @, % or +
	 * in 1.1, this is more complex. All prefixes are sent, with the additional caveat that modules
	 * can add their own prefixes (dangerous!) - therefore, don't just chanuser_add(), split the prefix
	 * out and ignore unknown prefixes (probably the safest option). --w00t
	 */
	userc = sjtoken(parv[parc - 1], ' ', userv);

	if (keep_new_modes)
	{
		channel_mode(NULL, c, parc - 3, parv + 2);
	}

	// loop over all the users in this fjoin
	for (i = 0; i < userc; i++)
	{
		nlen = 0;
		prefix = true;

		slog(LG_DEBUG, "m_fjoin(): processing user: %s", userv[i]);

		/* ok, now look at the chars in the nick.. we have something like "@%,w00t", but need @%w00t.. and
		 * we also want to ignore unknown prefixes.. loop through the chars
		 */
		for (; *userv[i]; userv[i]++)
		{
			map_a_prefix(*userv[i], prefixandnick, &nlen);

			// it's not a known prefix char, have we reached the end of the prefixes?
			if (*userv[i] == ',')
			{
				// yup, skip over the comma
				userv[i]++;

				// if we're ignoring status (keep_new_modes is false) then just add them to chan here...
				if (keep_new_modes == false)
				{
					// This ignores the @%, and just adds 'w00t' to the chan
					chanuser_add(c, userv[i]);
				}
				else
				{
					// else, we do care about their prefixes.. add '@%w00t' to the chan
					mowgli_strlcpy(prefixandnick + nlen, userv[i], sizeof(prefixandnick) - nlen);
					chanuser_add(c, prefixandnick);
				}

				// added them.. break out of this loop, which will move us to the next user
				break;
			}
			else
			{
				// unknown prefix char
			}
		}

		// go to the next user
	}

	if (c->nummembers == 0 && !(c->modes & ircd->perm_mode))
		channel_delete(c);
}

static void
m_part(struct sourceinfo *si, int parc, char *parv[])
{
	slog(LG_DEBUG, "m_part(): user left channel: %s -> %s", si->su->nick, parv[0]);
	chanuser_delete(channel_find(parv[0]), si->su);
}

static void
m_uid(struct sourceinfo *si, int parc, char *parv[])
{
	struct user *u;

	/* -> :751 UID 751AAAAAA 1220196319 Brain brainwave.brainbox.cc netadmin.chatspike.net brain 192.168.1.10 1220196324 +Siosw +ACKNOQcdfgklnoqtx :Craig Edwards
	 *                 0          1       2             3                     4              5         6          7         8           9*               10*
	 *
	 * note: you can't rely on realname being p[10], it's actually p[parc - 1].
	 * reason being that mode params may exist in p[9]+, or not at all.
	 */
	slog(LG_DEBUG, "m_uid(): new user on `%s': %s", si->s->name, parv[2]);

	//            nick,    user,    host,    vhost,    ip,      uid,        gecos,    server,       ts
	u = user_add(parv[2], parv[5], parv[3], parv[4], parv[6], parv[0], parv[parc - 1], si->s, atol(parv[1]));

	if (u == NULL)
		return;

	user_mode(u, parv[8]);

	// If server is not yet EOB we will do this later. This avoids useless "please identify" -- jilles
	if (si->s->flags & SF_EOB)
		handle_nickchange(u);
}

static void
m_nick(struct sourceinfo *si, int parc, char *parv[])
{
	slog(LG_DEBUG, "m_nick(): nickname change from `%s': %s", si->su->nick, parv[0]);

	if (user_changenick(si->su, parv[0], atoi(parv[1])))
		return;

	/* It could happen that our PING arrived late and the
	 * server didn't acknowledge EOB yet even though it is
	 * EOB; don't send double notices in that case -- jilles */
	if (si->su->server->flags & SF_EOB)
		handle_nickchange(si->su);
}

static void
m_quit(struct sourceinfo *si, int parc, char *parv[])
{
	slog(LG_DEBUG, "m_quit(): user leaving: %s", si->su->nick);

	// user_delete() takes care of removing channels and so forth
	user_delete(si->su, parv[0]);
}

static void
inspircd_user_mode(struct user *u, const char *modes)
{
	int dir;
	const char *p;

	return_if_fail(u != NULL);

	user_mode(u, modes);
	dir = 0;
	for (p = modes; *p; ++p)
		switch (*p)
		{
			case '-':
				dir = MTYPE_DEL;
				break;
			case '+':
				dir = MTYPE_ADD;
				break;
			case 'x':
				/* If +x is set then the users vhost is set to their cloaked host
				 * Note that -x etc works OK here, InspIRCd is nice enough to tell us
				 * everytime a users host changes. - Adam
				 */
				if (dir == MTYPE_ADD)
				{
					/* It is possible for the users vhost to not be their cloaked host after +x.
					 * This only occurs when a user is introduced after a netmerge with their
					 * vhost instead of their cloaked host. - Adam
					 */
					if (strcmp(u->vhost, u->chost))
					{
						strshare_unref(u->chost);
						u->chost = strshare_get(u->vhost);
					}
				}
				break;
		}
}

static void
m_mode(struct sourceinfo *si, int parc, char *parv[])
{
	if (*parv[0] == '#')
		channel_mode(NULL, channel_find(parv[0]), parc - 1, &parv[1]);
	else
		inspircd_user_mode(user_find(parv[0]), parv[1]);
}

static void
m_fmode(struct sourceinfo *si, int parc, char *parv[])
{
	struct channel *c;
	time_t ts;

	// :server.moo FMODE #blarp tshere +ntsklLg keymoo 1337 secks
	if (*parv[0] == '#')
	{
		c = channel_find(parv[0]);
		if (c == NULL)
		{
			slog(LG_DEBUG, "m_fmode(): nonexistent channel: %s", parv[0]);
			return;
		}
		ts = atoi(parv[1]);
		if (ts > c->ts)
		{
			return;
		}
		else if (ts < c->ts)
			slog(LG_DEBUG, "m_fmode(): %s %s: incoming TS %lu is older than our TS %lu, possible desync", parv[0], parv[2], (unsigned long)ts, (unsigned long)c->ts);
		channel_mode(NULL, c, parc - 2, &parv[2]);
	}
	else
		inspircd_user_mode(user_find(parv[0]), parv[2]);
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
	slog(LG_DEBUG, "m_squit(): server leaving: %s", parv[0]);
	server_delete(parv[0]);
}

static void
m_server(struct sourceinfo *si, int parc, char *parv[])
{
	char ver[BUFSIZE];

	slog(LG_DEBUG, "m_server(): new server: %s", parv[0]);

	if (si->s == NULL)
	{
		sts(":%s BURST", me.numeric);
		get_version_string(ver, sizeof(ver));
		sts(":%s VERSION :%s", me.numeric, ver);
		services_init();
		sts(":%s ENDBURST", me.numeric);
	}

	handle_server(si, parv[0], parv[3], atoi(parv[2]), parv[4]);
}

static inline void
solicit_pongs(struct server *s)
{
	mowgli_node_t *n;

	sts(":%s PING %s %s", me.numeric, me.numeric, s->sid);

	MOWGLI_ITER_FOREACH(n, s->children.head)
		solicit_pongs(n->data);
}

static void
m_endburst(struct sourceinfo *si, int parc, char *parv[])
{
	solicit_pongs(si->s);
}

static void
m_stats(struct sourceinfo *si, int parc, char *parv[])
{
	handle_stats(si->su, parv[0][0]);
}

static void
m_motd(struct sourceinfo *si, int parc, char *parv[])
{
	handle_motd(si->su);
}

static void
m_admin(struct sourceinfo *si, int parc, char *parv[])
{
	handle_admin(si->su);
}

static void
m_away(struct sourceinfo *si, int parc, char *parv[])
{
	handle_away(si->su, parc >= 1 ? parv[parc-1] : NULL);
}

static void
m_join(struct sourceinfo *si, int parc, char *parv[])
{
	struct channel *c;

	c = channel_find(parv[0]);
	if (!c)
	{
		slog(LG_DEBUG, "m_join(): new channel: %s (modes lost)", parv[0]);
		c = channel_add(parv[0], parc > 1 ? atol(parv[1]) : CURRTIME, si->su->server);
		return_if_fail(c != NULL);
		channel_mode_va(NULL, c, 1, "+");
	}
	chanuser_add(c, si->su->nick);
}

static void
m_save(struct sourceinfo *si, int parc, char *parv[])
{
	struct user *u = user_find(parv[0]);
	if (!u)
		return;

	if (u->ts != atoi(parv[1]))
	{
		slog(LG_DEBUG, "m_save(): ignoring SAVE message for %s, TS doesn't match (%lu != %s)", u->nick, (unsigned long)u->ts, parv[1]);
		return;
	}

	if (!strcmp(u->nick, u->uid))
	{
		slog(LG_DEBUG, "m_save(): ignoring noop SAVE message for %s", u->nick);
		return;
	}

	if (is_internal_client(u))
	{
		slog(LG_INFO, "m_save(): service %s got hit, changing back", u->nick);
		sts(":%s NICK %s %lu", u->uid, u->nick, (unsigned long) u->ts);
		// XXX services wars
	}
	else
	{
		slog(LG_DEBUG, "m_save(): nickname change for `%s': %s", u->nick, u->uid);

		if (user_changenick(u, u->uid, 0))
			return;

		handle_nickchange(u);
	}
}

static void
m_error(struct sourceinfo *si, int parc, char *parv[])
{
	slog(LG_INFO, "m_error(): error from server: %s", parv[0]);
}

static void
m_idle(struct sourceinfo *si, int parc, char *parv[])
{
	if (parc == 1 && si->su != NULL)
	{
		sts(":%s IDLE %s 0 0", parv[0], si->su->uid);
	}
	else
	{
		slog(LG_INFO, "m_idle(): Received an IDLE response but we didn't WHOIS anybody!");
	}
}

static void
m_opertype(struct sourceinfo *si, int parc, char *parv[])
{
	/* Hope this works.. InspIRCd OPERTYPE means user is an oper, mark them so
	 * Later, we may want to look at saving their OPERTYPE for informational
	 * purposes, or not. --w00t
	 */
	user_mode(si->su, "+o");
}

static void
m_fident(struct sourceinfo *si, int parc, char *parv[])
{
	strshare_unref(si->su->user);
	si->su->user = strshare_get(parv[0]);
}

static void
m_fhost(struct sourceinfo *si, int parc, char *parv[])
{
	strshare_unref(si->su->vhost);
	si->su->vhost = strshare_get(parv[0]);
}

static void
m_encap(struct sourceinfo *si, int parc, char *parv[])
{
	if (!irccasecmp(parv[1], "SASL"))
	{
		// :08C ENCAP * SASL 08CAAAAAE * S d29vTklOSkFTAGRhdGEgaW4gZmlyc3QgbGluZQ==
		struct sasl_message smsg;

		if (parc < 6)
			return;

		(void) memset(&smsg, 0x00, sizeof smsg);

		smsg.uid = parv[2];
		smsg.mode = *parv[4];
		smsg.parc = parc - 5;
		smsg.server = si->s;

		if (smsg.parc > SASL_MESSAGE_MAXPARA)
		{
			(void) slog(LG_ERROR, "%s: received SASL command with %d parameters",
			                      MOWGLI_FUNC_NAME, smsg.parc);

			smsg.parc = SASL_MESSAGE_MAXPARA;
		}

		(void) memcpy(smsg.parv, &parv[5], smsg.parc * sizeof(char *));

		hook_call_sasl_input(&smsg);
	}
}

static inline void
verify_mlock(struct channel *c, time_t ts, const char *their_mlock)
{
	const char *mlock_str;
	struct mychan *mc;

	mc = mychan_from(c);
	if (mc == NULL)
		return;

	if (ts != 0 && ts != c->ts)
		return mlock_sts(c);

	// bounce MLOCK change if it doesn't match what we say it is
	mlock_str = mychan_get_sts_mlock(mc);
	if (strcmp(mlock_str, their_mlock))
		return mlock_sts(c);
}

static void
verify_topiclock(struct channel *c, bool state)
{
	bool mystate;
	struct mychan *mc = mychan_from(c);
	if (!mc)
		return;

	mystate = mc->flags & MC_TOPICLOCK;
	if (state != mystate)
		inspircd_topiclock_sts(c);
}

/* :<source server> METADATA <channel|user> <key> :<value>
 * The sole piece of metadata we're interested in is 'accountname', set by Services,
 * and kept by ircd.
 *
 * :services.barafranca METADATA w00t accountname :w00t
 * :services.barafranca METADATA #moo 1274712456 mlock :nt
 */
static void
m_metadata(struct sourceinfo *si, int parc, char *parv[])
{
	struct user *u;
	struct channel *c;
	time_t ts;
	char *certfp;

	if (parc > 3)
	{
		c = channel_find(parv[0]);
		ts = atoi(parv[1]);

		if (!irccasecmp(parv[2], "mlock"))
			verify_mlock(c, ts, parv[3]);
	}

	if (!irccasecmp(parv[1], "accountname"))
	{
		// find user
		u = user_find(parv[0]);

		if (u == NULL)
			return;

		if (parv[2][0] == '\0')
			handle_clearlogin(si, u);
		else
			handle_burstlogin(u, parv[2], 0);
	}
	else if (!irccasecmp(parv[1], "ssl_cert"))
	{
		// :419 METADATA 419AAAAAB ssl_cert :vTrse abcdef0123456789abcdef CN=danieldg
		// :419 METADATA 419AAAAAC ssl_cert :vtrsE Error message for certificate
		char *fpstr, *end;
		size_t len;

		u = user_find(parv[0]);

		if (u == NULL)
			return;

		fpstr = strchr(parv[2], ' ');
		if (!fpstr++)
			return;
		// first, check for lack of valid certificate
		end = strchr(parv[2], 'E');
		if (end && end < fpstr)
			return;
		end = strchr(fpstr, ' ');

		len = end ? (unsigned int)(end - fpstr) : strlen(fpstr);

		certfp = smalloc(len + 1);
		memcpy(certfp, fpstr, len);

		handle_certfp(si, u, certfp);
		sfree(certfp);
	}
	else if (!irccasecmp(parv[1], "mlock"))
	{
		c = channel_find(parv[0]);
		verify_mlock(c, 0, parv[2]);
	}
	else if (!irccasecmp(parv[1], "topiclock"))
	{
		bool state = (!strcmp(parv[2], "1"));
		c = channel_find(parv[0]);
		if (c)
			verify_topiclock(c, state);
	}
}

/* RSQUIT:
 *  remote/request squit
 *  when squitting a remote server, inspircd sends RSQUIT along the tree until it reaches the server that has
 *  the server to be squit as a local connection, which should then close it's connection and send SQUIT back
 *  to the rest of the network.
 */
static void
m_rsquit(struct sourceinfo *si, int parc, char *parv[])
{
	sts(":%s SQUIT %s :Jupe removed by %s", me.numeric, parv[0], si->su->nick);
}

static void
channel_drop(struct mychan *mc)
{
	if (mc->chan == NULL)
		return;

	channel_metadata_sts(mc->chan, "mlock", "");
	channel_metadata_sts(mc->chan, "topiclock", "");
}

static void
m_capab(struct sourceinfo *si, int parc, char *parv[])
{
	int i, varc;
	char *varv[256];

	if (strcasecmp(parv[0], "START") == 0)
	{
		// reset all our previously received CAPAB stuff
		has_hideopermod = false;
		has_servicesmod = false;
		has_globopsmod = false;
		has_chghostmod = false;
		has_cbanmod = false;
		has_hidechansmod = false;
		has_servprotectmod = false;
		has_svshold = false;
		has_shun = false;
		has_svstopic_topiclock = false;
		has_protocol = 0;

		/* InspIRCd 2.0 and newer sends the protocol version in CAPAB START,
		 * if there is none sent then we can be sure it's an unsupported version.
		 */
		if (parc > 1)
			has_protocol = atoi(parv[1]);
		if (has_protocol == 1203 || has_protocol == 1204)
		{
			slog(LG_ERROR, "m_capab(): InspIRCd 2.1 beta is not supported.");
			exit(EXIT_FAILURE);
		}
		else if (has_protocol < PROTOCOL_MINIMUM)
		{
			slog(LG_ERROR, "m_capab(): remote protocol version too old (%d). you may need another protocol module or a newer inspircd. exiting.", has_protocol);
			exit(EXIT_FAILURE);
		}
	}
	else if (strcasecmp(parv[0], "CAPABILITIES") == 0 && parc > 1)
	{
		varc = sjtoken(parv[1], ' ', varv);
		for (i = 0; i < varc; i++)
		{
			if(!strncmp(varv[i], "PREFIX=", 7))
			{
				if (strstr(varv[i] + 7, "q"))
				{
					ircd->uses_owner = true;
				}
				if (strstr(varv[i] + 7, "a"))
				{
					ircd->uses_protect = true;
				}
				if (strstr(varv[i] + 7, "h"))
				{
					ircd->uses_halfops = true;
				}
			}
			else if (!strcmp(varv[i], "GLOBOPS=1"))
			{
				has_globopsmod = true;
			}
			// XXX check/store CHANMAX/IDENTMAX
		}
	}
	else if ((strcasecmp(parv[0], "MODULES") == 0 || strcasecmp(parv[0], "MODSUPPORT") == 0) && parc > 1)
	{
		char *it = NULL;

		if (strstr(parv[1], "m_services_account.so"))
		{
			has_servicesmod = true;
		}
		if (strstr(parv[1], "m_cloaking.so"))
		{
			has_cloakingmod = true;
		}
		if (strstr(parv[1], "m_globops.so"))
		{
			has_globopsmod = true;
		}
		if (strstr(parv[1], "m_chghost.so"))
		{
			has_chghostmod = true;
		}
		if (strstr(parv[1], "m_cban.so"))
		{
			has_cbanmod = true;
		}
		if (strstr(parv[1], "m_svshold.so"))
		{
			has_svshold = true;
		}
		if (strstr(parv[1], "m_shun.so"))
		{
			has_shun = true;
		}
		if (strstr(parv[1], "m_topiclock.so"))
		{
			has_svstopic_topiclock = true;
		}
		if ((it = strstr(parv[1], "m_kicknorejoin.so")) != NULL)
		{
			it = strchr(it, '=');
			if (it)
				max_rejoindelay = atoi(it + 1);
		}
		TAINT_ON(strstr(parv[1], "m_invisible.so") != NULL, "invisible (m_invisible) is not presently supported correctly in atheme, and won't be due to ethical obligations");
	}
	else if (strcasecmp(parv[0], "USERMODES") == 0 && parc > 1)
	{
		varc = sjtoken(parv[1], ' ', varv);
		for (i = 0; i < varc; i++)
		{
			if (!strcmp(varv[i], "hidechans=I"))
				has_hidechansmod = true;
			else if (!strcmp(varv[i], "hideoper=H"))
				has_hideopermod = true;
			else if (!strcmp(varv[i], "servprotect=k"))
				has_servprotectmod = true;
		}
	}
	else if (strcasecmp(parv[0], "END") == 0)
	{
		if (has_servicesmod == false)
		{
			slog(LG_ERROR, "m_capab(): you didn't load m_services_account into inspircd. atheme support requires this module. exiting.");
			exit(EXIT_FAILURE);
		}

		if (has_chghostmod == false)
		{
			slog(LG_DEBUG, "m_capab(): you didn't load m_chghost into inspircd. vhost setting will not work.");
		}

		if (has_cbanmod == false)
		{
			slog(LG_DEBUG, "m_capab(): you didn't load m_cban into inspircd. sqlines on channels will not work.");
		}

		if (has_svshold == false)
		{
			slog(LG_INFO, "m_capab(): you didn't load m_svshold into inspircd. nickname enforcers will not work.");
		}
	}
}

static void
server_eob(struct server *s)
{
	mowgli_node_t *n;

	MOWGLI_ITER_FOREACH(n, s->userlist.head)
	{
		handle_nickchange((struct user *)n->data);
	}
}

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "transport/rfc1459")
	MODULE_TRY_REQUEST_DEPENDENCY(m, "protocol/base36uid")

	next_matching_ban = &inspircd_next_matching_ban;
	server_login = &inspircd_server_login;
	introduce_nick = &inspircd_introduce_nick;
	quit_sts = &inspircd_quit_sts;
	wallops_sts = &inspircd_wallops_sts;
	join_sts = &inspircd_join_sts;
	chan_lowerts = &inspircd_chan_lowerts;
	kick = &inspircd_kick;
	msg = &inspircd_msg;
	msg_global_sts = &inspircd_msg_global_sts;
	notice_user_sts = &inspircd_notice_user_sts;
	notice_global_sts = &inspircd_notice_global_sts;
	notice_channel_sts = &inspircd_notice_channel_sts;
	numeric_sts = &inspircd_numeric_sts;
	kill_id_sts = &inspircd_kill_id_sts;
	part_sts = &inspircd_part_sts;
	kline_sts = &inspircd_kline_sts;
	unkline_sts = &inspircd_unkline_sts;
	qline_sts = &inspircd_qline_sts;
	unqline_sts = &inspircd_unqline_sts;
	topic_sts = &inspircd_topic_sts;
	mode_sts = &inspircd_mode_sts;
	ping_sts = &inspircd_ping_sts;
	ircd_on_login = &inspircd_on_login;
	ircd_on_logout = &inspircd_on_logout;
	jupe = &inspircd_jupe;
	sethost_sts = &inspircd_sethost_sts;
	fnc_sts = &inspircd_fnc_sts;
	invite_sts = &inspircd_invite_sts;
	holdnick_sts = &inspircd_holdnick_sts;
	svslogin_sts = &inspircd_svslogin_sts;
	sasl_sts = &inspircd_sasl_sts;
	sasl_mechlist_sts = &inspircd_sasl_mechlist_sts;
	quarantine_sts = &inspircd_quarantine_sts;
	mlock_sts = &inspircd_mlock_sts;
	topiclock_sts = &inspircd_topiclock_sts;
	is_extban = &inspircd_is_extban;
	dline_sts = &inspircd_dline_sts;
	undline_sts = &inspircd_undline_sts;

	mode_list = inspircd_mode_list;
	ignore_mode_list = inspircd_ignore_mode_list;
	status_mode_list = inspircd_status_mode_list;
	prefix_mode_list = inspircd_prefix_mode_list;
	user_mode_list = inspircd_user_mode_list;
	ignore_mode_list_size = ARRAY_SIZE(inspircd_ignore_mode_list);

	ircd = &InspIRCd;

	pcommand_add("PING", m_ping, 1, MSRC_USER | MSRC_SERVER);
	pcommand_add("PONG", m_pong, 1, MSRC_SERVER);
	pcommand_add("PRIVMSG", m_privmsg, 2, MSRC_USER | MSRC_SERVER);
	pcommand_add("NOTICE", m_notice, 2, MSRC_USER | MSRC_SERVER | MSRC_UNREG);
	pcommand_add("FJOIN", m_fjoin, 3, MSRC_SERVER);
	pcommand_add("PART", m_part, 1, MSRC_USER);
	pcommand_add("NICK", m_nick, 2, MSRC_USER);
	pcommand_add("UID", m_uid, 10, MSRC_SERVER);
	pcommand_add("QUIT", m_quit, 1, MSRC_USER);
	pcommand_add("MODE", m_mode, 2, MSRC_USER | MSRC_SERVER);
	pcommand_add("FMODE", m_fmode, 3, MSRC_USER | MSRC_SERVER);
	pcommand_add("KICK", m_kick, 2, MSRC_USER | MSRC_SERVER);
	pcommand_add("KILL", m_kill, 1, MSRC_USER | MSRC_SERVER);
	pcommand_add("SAVE", m_save, 2, MSRC_SERVER);
	pcommand_add("SQUIT", m_squit, 1, MSRC_USER | MSRC_SERVER);
	pcommand_add("RSQUIT", m_rsquit, 1, MSRC_USER);
	pcommand_add("SERVER", m_server, 4, MSRC_UNREG | MSRC_SERVER);
	pcommand_add("STATS", m_stats, 2, MSRC_USER);
	pcommand_add("MOTD", m_motd, 1, MSRC_USER);
	pcommand_add("ADMIN", m_admin, 1, MSRC_USER);
	pcommand_add("FTOPIC", m_ftopic, 4, MSRC_SERVER);
	pcommand_add("JOIN", m_join, 1, MSRC_USER);
	pcommand_add("ERROR", m_error, 1, MSRC_UNREG | MSRC_SERVER);
	pcommand_add("TOPIC", m_topic, 2, MSRC_USER);
	pcommand_add("FIDENT", m_fident, 1, MSRC_USER);
	pcommand_add("FHOST", m_fhost, 1, MSRC_USER);
	pcommand_add("IDLE", m_idle, 1, MSRC_USER);
	pcommand_add("AWAY", m_away, 0, MSRC_USER);
	pcommand_add("OPERTYPE", m_opertype, 1, MSRC_USER);
	pcommand_add("METADATA", m_metadata, 3, MSRC_SERVER);
	pcommand_add("CAPAB", m_capab, 1, MSRC_UNREG | MSRC_SERVER);
	pcommand_add("ENCAP", m_encap, 2, MSRC_USER | MSRC_SERVER);
	pcommand_add("ENDBURST", m_endburst, 0, MSRC_SERVER);

	hook_add_server_eob(server_eob);
	hook_add_channel_drop(channel_drop);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{

}

SIMPLE_DECLARE_MODULE_V1("protocol/inspircd", MODULE_UNLOAD_CAPABILITY_NEVER)
