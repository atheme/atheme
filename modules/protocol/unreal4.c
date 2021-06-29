/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2003-2004 E. Will, et al.
 * Copyright (C) 2005-2006 Atheme Project (http://atheme.org/)
 * Copyright (C) 2014-2015 Xtheme Development Group (https://xtheme.org/)
 * Copyright (C) 2016 Atheme Development Group (https://atheme.github.io/)
 *
 * This file contains protocol support for bahamut-based ircd.
 */

#include <atheme.h>
#include <atheme/protocol/unreal.h>

#define VALID_FLOOD_CHAR(c)	((c == 'c') || (c == 'j') || (c == 'k') || (c == 'm') || (c == 'n') || (c == 't'))
#define VALID_ACTION_CHAR(c)	((c == 'm') || (c == 'M') || (c == 'C') || (c == 'R') || (c == 'i') || (c == 'K') \
				 || (c == 'N') || (c == 'b'))

static struct ircd Unreal = {
	.ircdname = "UnrealIRCd 4 or later",
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
	.type = PROTOCOL_UNREAL,
	.perm_mode = CMODE_PERM,
	.oimmune_mode = 0,
	.ban_like_modes = "beI",
	.except_mchar = 'e',
	.invex_mchar = 'I',
	.flags = IRCD_HOLDNICK | IRCD_SASL_USE_PUID,
};

static const struct cmode unreal_mode_list[] = {
  { 'i', CMODE_INVITE	},
  { 'm', CMODE_MOD	},
  { 'n', CMODE_NOEXT	},
  { 'p', CMODE_PRIV	},
  { 's', CMODE_SEC	},
  { 't', CMODE_TOPIC	},
  { 'c', CMODE_NOCOLOR	},
  { 'D', CMODE_DELAYJOIN	},
  { 'M', CMODE_MODREG	},
  { 'R', CMODE_REGONLY	},
  { 'O', CMODE_OPERONLY },
  { 'Q', CMODE_PEACE	},
  { 'S', CMODE_STRIP	},
  { 'K', CMODE_NOKNOCK	},
  { 'V', CMODE_NOINVITE },
  { 'C', CMODE_NOCTCP	},
  { 'z', CMODE_SSLONLY	},
  { 'N', CMODE_STICKY	},
  { 'G', CMODE_CENSOR	},
  { 'r', CMODE_CHANREG	},
  { 'P', CMODE_PERM	},
  { 'T', CMODE_NONOTICE	},
  { '\0', 0 }
};

static const struct cmode unreal_status_mode_list[] = {
  { 'q', CSTATUS_OWNER	 },
  { 'a', CSTATUS_PROTECT },
  { 'o', CSTATUS_OP	 },
  { 'h', CSTATUS_HALFOP  },
  { 'v', CSTATUS_VOICE	 },
  { '\0', 0 }
};

static const struct cmode unreal_prefix_mode_list[] = {
  { '*', CSTATUS_OWNER	 },
  { '~', CSTATUS_PROTECT },
  { '@', CSTATUS_OP	 },
  { '%', CSTATUS_HALFOP  },
  { '+', CSTATUS_VOICE	 },
  { '\0', 0 }
};

static const struct cmode unreal_user_mode_list[] = {
  { 'A', UF_ADMIN    },
  { 'i', UF_INVIS    },
  { 'o', UF_IRCOP    },
  { 'd', UF_DEAF     },
  { '\0', 0 }
};

static bool has_protoctl = false;
static bool use_esvid = false;
static bool use_mlock = false;
static char ts6sid[3 + 1] = "";

// +f 3:1 or +f *3:1 (which is like +f [3t]:1 or +f [3t#b]:1)
static inline bool
check_flood_old(const char *value, struct channel *c, struct mychan *mc, struct user *u, struct myuser *mu)
{
	bool found_colon = false;

	return_val_if_fail(value != NULL, false);

	// x:y is 3 bytes long, so that is the minimum length of a +f parameter.
	if (strlen(value) < 3)
		return false;

	// skip past the * if present
	if (*value == '*')
		value++;

	// check to make sure all bytes are numbers, allowing for one colon
	while (*value != '\0')
	{
		if (*value == ':' && !found_colon)
			found_colon = true;
		else if (!isdigit((unsigned char)*value))
			return false;

		value++;
	}

	// we have to have a colon in order for it to be valid
	if (!found_colon)
		return false;

	return true;
}

/* +f *X:Y	 (handled by check_flood_old)
 * +f X:Y	 (handled by check_flood_old)
 *
 * +f [<number><letter>(#<letter>)(,...)]
 */
static bool
check_flood(const char *value, struct channel *c, struct mychan *mc, struct user *u, struct myuser *mu)
{
	char evalbuf[BUFSIZE], *ep, *p;

	if (*value != '[')
		return check_flood_old(value, c, mc, u, mu);

	// copy this to a local buffer for evaluation
	mowgli_strlcpy(evalbuf, value, sizeof evalbuf);
	ep = evalbuf + 1;

	// check that the parameter ends with a ]
	if ((p = strchr(ep, ']')) == NULL)
		return false;

	// we have a ], blast it away and check for a colon.
	*p = '\0';
	if (*(p + 1) != ':')
		return false;

	for (p = strtok(ep, ","); p != NULL; p = strtok(NULL, ","))
	{
		while (isdigit((unsigned char)*p))
			p++;

		if (!VALID_FLOOD_CHAR(*p))
			return false;

		*p = '\0';
		p++;

		if (*p != '\0')
		{
			if (*p == '#')
			{
				p++;

				if (!VALID_ACTION_CHAR(*p))
					return false;

				continue;
			}

			// not valid, needs to be # or nothing
			return false;
		}
	}

	return true;
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

static struct extmode unreal_ignore_mode_list[] = {
  { 'f', check_flood },
  { 'L', check_forward },
  { '\0', 0 }
};

static mowgli_node_t *
unreal_next_matching_ban(struct channel *c, struct user *u, int type, mowgli_node_t *first)
{
	struct chanban *cb;
	mowgli_node_t *n;
	char hostbuf[NICKLEN + 1 + USERLEN + 1 + HOSTLEN + 1];
	char realbuf[NICKLEN + 1 + USERLEN + 1 + HOSTLEN + 1];
	char ipbuf[NICKLEN + 1 + USERLEN + 1 + HOSTLEN + 1];
	char *p;
	bool matched;
	int exttype;
	struct channel *target_c;

	snprintf(hostbuf, sizeof hostbuf, "%s!%s@%s", u->nick, u->user, u->vhost);
	snprintf(realbuf, sizeof realbuf, "%s!%s@%s", u->nick, u->user, u->host);

	// will be nick!user@ if ip unknown, doesn't matter
	snprintf(ipbuf, sizeof ipbuf, "%s!%s@%s", u->nick, u->user, u->ip);

	bool check_realhost = (config_options.masks_through_vhost || u->host == u->vhost);

	MOWGLI_ITER_FOREACH(n, first)
	{
		cb = n->data;

		if (cb->type != type)
			continue;

		if (!match(cb->mask, hostbuf))
			return n;
		if (check_realhost && (!match(cb->mask, realbuf) || !match(cb->mask, ipbuf)))
			return n;

		if (cb->mask[0] == '~')
		{
			p = cb->mask + 1;
			exttype = *p++;

			if (exttype == '\0')
				continue;

			// check parameter
			if (*p++ != ':')
				p = NULL;

			switch (exttype)
			{
				case 'a':
					matched = u->myuser != NULL && !(u->myuser->flags & MU_WAITAUTH) && (p == NULL || !match(p, entity(u->myuser)->name));
					break;
				case 'c':
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
				case 'R':
					matched = should_reg_umode(u);
					break;
				case 'q':
					matched = !match(p, hostbuf);
					if (check_realhost && !matched)
						matched = !match(p, ipbuf);
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

static unsigned int
unreal_server_login(void)
{
	int ret;

	ret = sts("PASS %s", curr_uplink->send_pass);
	if (ret == 1)
		return 1;

	me.bursting = true;
	has_protoctl = false;

	sts("PROTOCTL NICKv2 VHP NICKIP UMODE2 SJOIN SJOIN2 SJ3 NOQUIT TKLEXT ESVID MLOCK");
	sts("PROTOCTL EAUTH=%s", me.name);
	sts("PROTOCTL SID=%s", me.numeric);
	sts("SERVER %s 1 :%s", me.name, me.desc);

	sts(":%s EOS", ME);

	return 0;
}

static void
unreal_introduce_nick(struct user *u)
{
	const char *umode = user_get_umodestr(u);

	if (!ircd->uses_uid)
		sts("NICK %s 1 %lu %s %s %s 0 %sS * :%s", u->nick, (unsigned long)u->ts, u->user, u->host, me.name, umode, u->gecos);
	else
		sts(":%s UID %s 1 %lu %s %s %s * %sS * * * :%s", ME, u->nick, (unsigned long)u->ts, u->user, u->host, u->uid, umode, u->gecos);
}

static void
unreal_invite_sts(struct user *sender, struct user *target, struct channel *channel)
{
	sts(":%s INVITE %s %s", CLIENT_NAME(sender), CLIENT_NAME(target), channel->name);
}

static void
unreal_quit_sts(struct user *u, const char *reason)
{
	sts(":%s QUIT :%s", CLIENT_NAME(u), reason);
}

static void
unreal_wallops_sts(const char *text)
{
	sts(":%s GLOBOPS :%s", ME, text);
}

static void
unreal_join_sts(struct channel *c, struct user *u, bool isnew, char *modes)
{
	if (isnew)
		sts(":%s SJOIN %lu %s %s :@%s", ME, (unsigned long)c->ts,
				c->name, modes, CLIENT_NAME(u));
	else
		sts(":%s SJOIN %lu %s + :@%s", ME, (unsigned long)c->ts,
				c->name, CLIENT_NAME(u));
}

static void
unreal_chan_lowerts(struct channel *c, struct user *u)
{
	slog(LG_DEBUG, "unreal_chan_lowerts(): lowering TS for %s to %lu",
			c->name, (unsigned long)c->ts);
	sts(":%s SJOIN %lu %s %s :@%s", ME, (unsigned long)c->ts, c->name,
			channel_modes(c, true), CLIENT_NAME(u));
}

static void
unreal_kick(struct user *source, struct channel *c, struct user *u, const char *reason)
{
	sts(":%s KICK %s %s :%s", source->nick, c->name, u->nick, reason);

	chanuser_delete(c, u);
}

static void ATHEME_FATTR_PRINTF(3, 4)
unreal_msg(const char *from, const char *target, const char *fmt, ...)
{
	va_list ap;
	char buf[BUFSIZE];

	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZE, fmt, ap);
	va_end(ap);

	sts(":%s PRIVMSG %s :%s", from, target, buf);
}

static void
unreal_msg_global_sts(struct user *from, const char *mask, const char *text)
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
unreal_notice_user_sts(struct user *from, struct user *target, const char *text)
{
	sts(":%s NOTICE %s :%s", from ? CLIENT_NAME(from) : ME, CLIENT_NAME(target), text);
}

static void
unreal_notice_global_sts(struct user *from, const char *mask, const char *text)
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
unreal_notice_channel_sts(struct user *from, struct channel *target, const char *text)
{
	sts(":%s NOTICE %s :%s", from ? CLIENT_NAME(from) : ME, target->name, text);
}

static void ATHEME_FATTR_PRINTF(4, 5)
unreal_numeric_sts(struct server *from, int numeric, struct user *target, const char *fmt, ...)
{
	va_list ap;
	char buf[BUFSIZE];

	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZE, fmt, ap);
	va_end(ap);

	sts(":%s %d %s %s", SERVER_NAME(from), numeric, CLIENT_NAME(target), buf);
}

static void
unreal_kill_id_sts(struct user *killer, const char *id, const char *reason)
{
	if (killer != NULL)
	{
		if (nicksvs.me != NULL && killer == nicksvs.me->me)
		{
			sts(":%s SVSKILL %s :Killed (%s (%s))",
					killer->nick, id,
					killer->nick, reason);
			/* We still send KILL too, for two reasons:
			 * 1. SVSKILL does not do nick chase as KILL does,
			 *    so the below KILL ensures the user is removed
			 *    everywhere if kill and nick change cross.
			 *    If this happens between two ircds, unrealircd's
			 *    unknown prefix kills (nick(?) <- someserver)
			 *    will fix it up, but we do not kill unknown
			 *    prefixes.
			 * 2. SVSKILL is ignored if we are not U:lined.
			 *
			 * If the SVSKILL is effective, the KILL will be
			 * dropped.
			 */
		}
		sts(":%s KILL %s :%s!%s (%s)", CLIENT_NAME(killer), id, killer->host, killer->nick, reason);
	}
	else
		sts(":%s KILL %s :%s (%s)", ME, id, me.name, reason);
}

static void
unreal_part_sts(struct channel *c, struct user *u)
{
	sts(":%s PART %s", CLIENT_NAME(u), c->name);
}

static void
unreal_kline_sts(const char *server, const char *user, const char *host, long duration, const char *reason)
{
	struct service *svs;

	svs = service_find("operserv");
	sts(":%s TKL + G %s %s %s %lu %lu :%s", ME, user, host, svs != NULL ? svs->nick : me.name, (unsigned long)(duration > 0 ? CURRTIME + duration : 0), (unsigned long)CURRTIME, reason);
}

static void
unreal_unkline_sts(const char *server, const char *user, const char *host)
{
	struct service *svs;

	svs = service_find("operserv");
	sts(":%s TKL - G %s %s %s", ME, user, host, svs != NULL ? svs->nick : me.name);
}

static void
unreal_xline_sts(const char *server, const char *realname, long duration, const char *reason)
{
	char escapedreason[512], *p;

	if (duration > 0)
	{
		slog(LG_INFO, "SGLINE: Could not set temporary SGLINE on \2%s\2, not supported by unrealircd.", realname);
		return;
	}

	mowgli_strlcpy(escapedreason, reason, sizeof escapedreason);
	for (p = escapedreason; *p != '\0'; p++)
		if (*p == ' ')
			*p = '_';
	if (*escapedreason == ':')
		*escapedreason = ';';

	sts(":%s SVSNLINE + %s :%s", ME, escapedreason, realname);
}

static void
unreal_unxline_sts(const char *server, const char *realname)
{
	sts(":%s SVSNLINE - :%s", ME, realname);
}

static void
unreal_qline_sts(const char *server, const char *name, long duration, const char *reason)
{
	struct service *svs;

	if (*name == '#' || *name == '&')
	{
		slog(LG_INFO, "SQLINE: Could not set SQLINE on \2%s\2, not supported by unrealircd.", name);
		return;
	}

	svs = service_find("operserv");
	sts(":%s TKL + Q * %s %s %lu %lu :%s", ME, name, svs != NULL ? svs->nick : me.name, (unsigned long)(duration > 0 ? CURRTIME + duration : 0), (unsigned long)CURRTIME, reason);
}

static void
unreal_unqline_sts(const char *server, const char *name)
{
	struct service *svs;

	svs = service_find("operserv");
	sts(":%s TKL - Q * %s %s", ME, name, svs != NULL ? svs->nick : me.name);
}

static void
unreal_topic_sts(struct channel *c, struct user *source, const char *setter, time_t ts, time_t prevts, const char *topic)
{
	return_if_fail(c != NULL);
	return_if_fail(source != NULL);

	sts(":%s TOPIC %s %s %lu :%s", source->nick, c->name, setter, (unsigned long)ts, topic);
}

static void
unreal_mode_sts(char *sender, struct channel *target, char *modes)
{
	return_if_fail(sender != NULL);
	return_if_fail(target != NULL);
	return_if_fail(modes != NULL);

	sts(":%s MODE %s %s", sender, target->name, modes);
}

static void
unreal_ping_sts(void)
{
	sts("PING :%s", ME);
}

static void
unreal_on_login(struct user *u, struct myuser *account, const char *wantedhost)
{
	return_if_fail(u != NULL);
	return_if_fail(account != NULL);

	if (!use_esvid)
	{
		/* Can only do this for nickserv, and can only record identified
		 * state if logged in to correct nick, sorry -- jilles
		 *
		 * imo, we should be using SVS2MODE to show the modechange here and on logout --w00t */
		if (should_reg_umode(u))
			sts(":%s SVS2MODE %s +rd %lu", nicksvs.nick, u->nick, (unsigned long)u->ts);

		return;
	}

	if (should_reg_umode(u))
		sts(":%s SVS2MODE %s +rd %s", nicksvs.nick, u->nick, entity(account)->name);
	else
		sts(":%s SVS2MODE %s +d %s", nicksvs.nick, u->nick, entity(account)->name);
}

static bool
unreal_on_logout(struct user *u, const char *account)
{
	return_val_if_fail(u != NULL, false);

	if (use_esvid || !nicksvs.no_nick_ownership)
		sts(":%s SVS2MODE %s -r+d 0", nicksvs.nick, u->nick);

	return false;
}

static void
unreal_jupe(const char *server, const char *reason)
{
	struct service *svs;
	static char sid[3+1];
	int i;

	server_delete(server);

	svs = service_find("operserv");
	sts(":%s SQUIT %s :%s", svs != NULL ? svs->nick : ME, server, reason);

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

	sts(":%s SID %s 2 %s :%s", ME, server, sid, reason);
}

static void
unreal_sethost_sts(struct user *source, struct user *target, const char *host)
{
	sts(":%s CHGHOST %s :%s", source->nick, target->nick, host);

	if (irccasecmp(target->host, host))
		numeric_sts(me.me, 396, target, "%s :is now your hidden host (set by %s)", host, source->nick);
	else
	{
		numeric_sts(me.me, 396, target, "%s :hostname reset by %s", host, source->nick);
		sts(":%s SVS2MODE %s +x", source->nick, target->nick);
	}
}

static void
unreal_fnc_sts(struct user *source, struct user *u, const char *newnick, int type)
{
	sts(":%s SVSNICK %s %s %lu", ME, CLIENT_NAME(u), newnick,
			(unsigned long)(CURRTIME - SECONDS_PER_MINUTE));
}

static void
unreal_holdnick_sts(struct user *source, int duration, const char *nick, struct myuser *mu)
{
	if (duration > 0)
		sts(":%s TKL + Q H %s %s %lu %lu :Reserved by %s for nickname owner (%s)",
				ME, nick, source->nick,
				(unsigned long)(CURRTIME + duration),
				(unsigned long)CURRTIME,
				source->nick,
				mu ? entity(mu)->name : nick);
	else
		sts(":%s TKL - Q H %s %s", ME, nick, source->nick);
}

static void
unreal_quarantine_sts(struct user *source, struct user *victim, long duration, const char *reason)
{
	sts(":%s SHUN +*@%s %ld :%s", source->nick, victim->host, duration, reason);
}

static void
unreal_sasl_sts(const char *target, char mode, const char *data)
{
	char servermask[BUFSIZE], *p;
	struct service *saslserv;

	saslserv = service_find("saslserv");
	if (saslserv == NULL)
		return;

	// extract the servername from the target.
	mowgli_strlcpy(servermask, target, sizeof servermask);
	p = strchr(servermask, '!');
	if (p != NULL)
		*p = '\0';

	sts(":%s SASL %s %s %c %s", saslserv->me->nick, servermask, target, mode, data);
}

static void
unreal_sasl_mechlist_sts(const char *mechlist)
{
	sts("MD client %s saslmechlist :%s", ME, mechlist);
}

static void
unreal_svslogin_sts(const char *target, const char *nick, const char *user, const char *host, struct myuser *account)
{
	char servermask[BUFSIZE], *p;
	struct service *saslserv;

	saslserv = service_find("saslserv");
	if (saslserv == NULL)
		return;

	// extract the servername from the target.
	mowgli_strlcpy(servermask, target, sizeof servermask);
	p = strchr(servermask, '!');
	if (p != NULL)
		*p = '\0';

	sts(":%s SVSLOGIN %s %s %s", saslserv->me->nick, servermask, target, entity(account)->name);
}

static void
unreal_mlock_sts(struct channel *c)
{
	struct mychan *mc = mychan_from(c);

	if (use_mlock == false)
		return;

	if (mc == NULL)
		return;

	sts(":%s MLOCK %lu %s :%s", ME, (unsigned long)c->ts, c->name,
				    mychan_get_sts_mlock(mc));
}

static void
m_mlock(struct sourceinfo *si, int parc, char *parv[])
{
	struct channel *c;
	struct mychan *mc;
	const char *mlock;

	// Ignore MLOCK if the server isn't bursting, to avoid 'war' conditions
	if (si->s->flags & SF_EOB)
		return;

	if (!(c = channel_find(parv[1])))
		return;

	if (!(mc = mychan_from(c)))
	{
		// Unregistered channel. Clear the MLOCK.
		sts(":%s MLOCK %lu %s :", ME, (unsigned long)c->ts, c->name);
		return;
	}

	time_t ts = atol(parv[0]);
	if (ts > c->ts)
		return;

	mlock = mychan_get_sts_mlock(mc);
	if (0 != strcmp(parv[2], mlock))
	{
		// MLOCK is changing, with the same TS. Bounce back the correct one.
		sts(":%s MLOCK %lu %s :%s", ME, (unsigned long)c->ts, c->name, mlock);
	}
}

static void
m_sasl(struct sourceinfo *si, int parc, char *parv[])
{
	struct sasl_message smsg;

	// :irc.loldongs.eu SASL * irc.goatse.cx!42 S d29vTklOSkFTAGRhdGEgaW4gZmlyc3QgbGluZQ==
	if (parc < 4)
		return;

	(void) memset(&smsg, 0x00, sizeof smsg);

	smsg.uid = parv[1];
	smsg.mode = *parv[2];
	smsg.parc = parc - 3;
	smsg.server = si->s;

	if (smsg.parc > SASL_MESSAGE_MAXPARA)
	{
		(void) slog(LG_ERROR, "%s: received SASL command with %d parameters", MOWGLI_FUNC_NAME, smsg.parc);
		smsg.parc = SASL_MESSAGE_MAXPARA;
	}

	(void) memcpy(smsg.parv, &parv[3], smsg.parc * sizeof(char *));

	hook_call_sasl_input(&smsg);
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
	sts(":%s PONG %s %s", ME, me.name, parv[0]);
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
	handle_eob(s);

	if (s != si->s)
		return;

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
remove_our_modes(struct channel *c)
{
	/* the TS changed.  a TS change requires the following things
	 * to be done to the channel:  reset all modes to nothing, remove
	 * all status modes on known users on the channel (including ours),
	 * and set the new TS.
	 */
	struct chanuser *cu;
	mowgli_node_t *n;

	clear_simple_modes(c);

	MOWGLI_ITER_FOREACH(n, c->members.head)
	{
		cu = (struct chanuser *)n->data;
		cu->modes = 0;
	}
}

static void
m_sjoin(struct sourceinfo *si, int parc, char *parv[])
{
	/*  -> :proteus.malkier.net SJOIN 1073516550 #shrike +tn :@sycobuny @+rakaur
	 *	also:
	 *  -> :nenolod_ SJOIN 1117334567 #chat
	 *	also:
	 *  -> SJOIN 1117334567 #chat :@nenolod
	 */

	struct channel *c;
	unsigned int userc;
	char *userv[256];
	unsigned int i;
	time_t ts;

	if (parc >= 4)
	{
		// :origin SJOIN ts chan modestr [key or limits] :users
		c = channel_find(parv[1]);
		ts = atol(parv[0]);

		if (!c)
		{
			slog(LG_DEBUG, "m_sjoin(): new channel: %s", parv[1]);
			c = channel_add(parv[1], ts, si->s);
		}

		if (ts < c->ts)
		{
			remove_our_modes(c);
			slog(LG_DEBUG, "m_sjoin(): TS changed for %s (%lu -> %lu)", c->name, (unsigned long)c->ts, (unsigned long)ts);
			c->ts = ts;
			hook_call_channel_tschange(c);
		}

		channel_mode(NULL, c, parc - 3, parv + 2);
		userc = sjtoken(parv[parc - 1], ' ', userv);

		for (i = 0; i < userc; i++)
			if (*userv[i] == '&')	// channel ban
				chanban_add(c, userv[i] + 1, 'b');
			else if (*userv[i] == '"')	// exception
				chanban_add(c, userv[i] + 1, 'e');
			else if (*userv[i] == '\'')	// invex
				chanban_add(c, userv[i] + 1, 'I');
			else
				chanuser_add(c, userv[i]);
	}
	else if (parc == 3)
	{
		// :origin SJOIN ts chan :users
		c = channel_find(parv[1]);
		ts = atol(parv[0]);

		if (!c)
		{
			slog(LG_DEBUG, "m_sjoin(): new channel: %s (modes lost)", parv[1]);
			c = channel_add(parv[1], ts, si->s);
		}

		if (ts < c->ts)
		{
			remove_our_modes(c);
			slog(LG_DEBUG, "m_sjoin(): TS changed for %s (%lu -> %lu)", c->name, (unsigned long)c->ts, (unsigned long)ts);
			c->ts = ts;
			hook_call_channel_tschange(c);
		}

		channel_mode_va(NULL, c, 1, "+");

		userc = sjtoken(parv[parc - 1], ' ', userv);

		for (i = 0; i < userc; i++)
			if (*userv[i] == '&')	// channel ban
				chanban_add(c, userv[i] + 1, 'b');
			else if (*userv[i] == '"')	// exception
				chanban_add(c, userv[i] + 1, 'e');
			else if (*userv[i] == '\'')	// invex
				chanban_add(c, userv[i] + 1, 'I');
			else
				chanuser_add(c, userv[i]);
	}
	else if (parc == 2)
	{
		c = channel_find(parv[1]);
		ts = atol(parv[0]);
		if (!c)
		{
			slog(LG_DEBUG, "m_sjoin(): new channel: %s (modes lost)", parv[1]);
			c = channel_add(parv[1], ts, si->su->server);
		}

		if (ts < c->ts)
		{
			remove_our_modes(c);
			slog(LG_DEBUG, "m_sjoin(): TS changed for %s (%lu -> %lu)", c->name, (unsigned long)c->ts, (unsigned long)ts);
			c->ts = ts;

			// XXX lost modes! -- XXX - pardon? why do we worry about this? TS reset requires modes reset..
			hook_call_channel_tschange(c);
		}

		channel_mode_va(NULL, c, 1, "+");

		chanuser_add(c, si->su->nick);
	}
	else
		return;

	if (c->nummembers == 0 && !(c->modes & ircd->perm_mode))
		channel_delete(c);
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
m_uid(struct sourceinfo *si, int parc, char *parv[])
{
	struct server *s;
	struct user *u;
	bool realchange;
	const char *vhost;
	const char *ipb64;
	char ipstring[64];
	int af;
	size_t iplen;
	char ipdata[16];

	if (parc == 12)
	{
		s = si->s;
		if (!s)
		{
			slog(LG_DEBUG, "m_uid(): new user on nonexistent server: %s", parv[0]);
			return;
		}

		slog(LG_DEBUG, "m_uid(): new user on `%s': %s", s->name, si->s->name);

		vhost = strcmp(parv[8], "*") ? parv[8] : NULL;
		iplen = 0;
		if (parc == 11 && strcmp(parv[parc - 2], "*"))
		{
			ipb64 = parv[parc - 2];
			af = AF_INET;
			if (strlen(ipb64) == 8)
			{
				iplen = 4;
				if (base64_decode(ipb64, ipdata, iplen) != iplen)
					iplen = 0;
				af = AF_INET;
			}
			else if (strlen(ipb64) == 24)
			{
				iplen = 16;
				if (base64_decode(ipb64, ipdata, iplen) != iplen)
					iplen = 0;
				af = AF_INET6;
			}
			if (iplen != 0)
				if (!inet_ntop(af, ipdata, ipstring, sizeof ipstring))
					iplen = 0;
		}
		u = user_add(parv[0], parv[3], parv[4], vhost, iplen != 0 ? ipstring : NULL, parv[5], parv[parc - 1], s, atoi(parv[2]));
		if (u == NULL)
			return;

		user_mode(u, parv[7]);

		/* with ESVID:
		 * If the user's SVID is equal to their accountname,
		 * they're properly logged in.	Alternatively, the
		 * 'without ESVID' criteria is used. --nenolod
		 *
		 * without ESVID:
		 * If the user's SVID is equal to their nick TS,
		 * they're properly logged in --jilles
		 */
		if (use_esvid && !IsDigit(*parv[6]))
		{
			handle_burstlogin(u, parv[6], 0);

			if (authservice_loaded && should_reg_umode(u))
				sts(":%s SVS2MODE %s +r", nicksvs.nick, u->nick);
		}
		else if (u->ts > 100 && (time_t)atoi(parv[6]) == u->ts)
			handle_burstlogin(u, NULL, 0);

		handle_nickchange(u);
	}
	else
	{
		int i;
		slog(LG_DEBUG, "m_uid(): got UID with wrong number of params");

		for (i = 0; i < parc; i++)
			slog(LG_DEBUG, "m_uid():   parv[%d] = %s", i, parv[i]);
	}
}

static void
m_nick(struct sourceinfo *si, int parc, char *parv[])
{
	struct server *s;
	struct user *u;
	bool realchange;
	const char *vhost;
	const char *ipb64;
	char ipstring[64];
	int af;
	size_t iplen;
	char ipdata[16];

	if (parc > 10)
	{
		s = server_find(parv[5]);
		if (!s)
		{
			slog(LG_DEBUG, "m_nick(): new user on nonexistent server: %s", parv[5]);
			return;
		}

		slog(LG_DEBUG, "m_nick(): new user on `%s': %s", s->name, parv[0]);

		vhost = strcmp(parv[8], "*") ? parv[8] : NULL;
		iplen = 0;
		if (parc == 11 && strcmp(parv[parc - 2], "*"))
		{
			ipb64 = parv[parc - 2];
			af = AF_INET;
			if (strlen(ipb64) == 8)
			{
				iplen = 4;
				if (base64_decode(ipb64, ipdata, iplen) != iplen)
					iplen = 0;
				af = AF_INET;
			}
			else if (strlen(ipb64) == 24)
			{
				iplen = 16;
				if (base64_decode(ipb64, ipdata, iplen) != iplen)
					iplen = 0;
				af = AF_INET6;
			}
			if (iplen != 0)
				if (!inet_ntop(af, ipdata, ipstring, sizeof ipstring))
					iplen = 0;
		}
		u = user_add(parv[0], parv[3], parv[4], vhost, iplen != 0 ? ipstring : NULL, NULL, parv[parc - 1], s, atoi(parv[2]));
		if (u == NULL)
			return;

		user_mode(u, parv[7]);

		/* with ESVID:
		 * If the user's SVID is equal to their accountname,
		 * they're properly logged in.	Alternatively, the
		 * 'without ESVID' criteria is used. --nenolod
		 *
		 * without ESVID:
		 * If the user's SVID is equal to their nick TS,
		 * they're properly logged in --jilles
		 */
		if (use_esvid && !IsDigit(*parv[6]))
		{
			handle_burstlogin(u, parv[6], 0);

			if (authservice_loaded && should_reg_umode(u))
				sts(":%s SVS2MODE %s +r", nicksvs.nick, u->nick);
		}
		else if (u->ts > 100 && (time_t)atoi(parv[6]) == u->ts)
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
		if (realchange && !nicksvs.no_nick_ownership && !use_esvid)
		{
			if (should_reg_umode(si->su))
				// changed nick to registered one, reset +r
				sts(":%s SVS2MODE %s +rd %ld", nicksvs.nick, parv[0], atol(parv[1]));
			else
				// changed from registered nick, remove +r
				sts(":%s SVS2MODE %s -r+d 0", nicksvs.nick, parv[0]);
		}
		else if (realchange && !nicksvs.no_nick_ownership && use_esvid)
		{
			if (should_reg_umode(si->su))
				sts(":%s SVS2MODE %s +r", nicksvs.nick, parv[0]);
			else
				sts(":%s SVS2MODE %s -r", nicksvs.nick, parv[0]);
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
m_quit(struct sourceinfo *si, int parc, char *parv[])
{
	slog(LG_DEBUG, "m_quit(): user leaving: %s", si->su->nick);

	// user_delete() takes care of removing channels and so forth
	user_delete(si->su, parv[0]);
}

static void
unreal_user_mode(struct user *u, const char *changes)
{
	const char *p;
	int dir;

	if (u == NULL)
		return;
	user_mode(u, changes);
	dir = 0;
	for (p = changes; *p != '\0'; p++)
		switch (*p)
		{
			case '-': dir = MTYPE_DEL; break;
			case '+': dir = MTYPE_ADD; break;
			case 'x':
				// If +x is set then the users vhost is set to their cloaked host - Adam
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
				else if (dir == MTYPE_DEL)
				{
					strshare_unref(u->vhost);
					u->vhost = strshare_get(u->host);
				}
				break;
		}
}

static bool
unreal_is_extban(const char *mask)
{
	const char mask_len = strlen(mask);
	unsigned char offset = 0;

	if (mask_len < 4 || mask[0] != '~' || mask[2] != ':' || strchr(mask, ' '))
		return false;

	if ((mask[1] < 'a' || mask[1] > 'z') && (mask[1] < 'A' || mask[1] > 'Z'))
		return false;

	return true;
}

static void
m_mode(struct sourceinfo *si, int parc, char *parv[])
{
	if (*parv[0] == '#')
		channel_mode(NULL, channel_find(parv[0]), parc - 1, &parv[1]);
	else
		unreal_user_mode(user_find(parv[0]), parv[1]);
}

static void
m_umode(struct sourceinfo *si, int parc, char *parv[])
{
	unreal_user_mode(si->su, parv[0]);
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
	const char *inf;

	/* because multiple PROTOCTL messages are allowed without a PROTOCTL END,
	 * and even if we added a PROTOCTL END in unreal 3.4, we'd still have to do
	 * this hack in order to support 3.2... -- kaniini
	 */
	if (has_protoctl)
	{
		if (ts6sid[0] == '\0')
		{
			ircd->uses_uid = false;
			if (me.me->sid)
			{
				/* XXX: this is a hack that fixes *most*, though not
				 * all, of the brokenness when linking to Unreal 3.2
				 *
				 * (the EOB is sent before this, therefore still with
				 * a SID, but apparently still works) --grawity */
				slog(LG_DEBUG, "m_server(): erasing our SID");
				sfree(me.me->sid);
				me.me->sid = NULL;
			}
		}

		services_init();
		has_protoctl = false;	// only once after PROTOCTL message.
	}

	slog(LG_DEBUG, "m_server(): new server: %s", parv[0]);
	if (si->s == NULL && (inf = strchr(parv[2], ' ')) != NULL)
		inf++;
	else
		inf = parv[2];
	s = handle_server(si, parv[0], si->s || !ircd->uses_uid ? NULL : ts6sid, atoi(parv[1]), inf);

	if (s != NULL && s->uplink != me.me)
	{
		/* elicit PONG for EOB detection; pinging uplink is
		 * already done elsewhere -- jilles
		 */
		sts(":%s PING %s %s", ME, me.name, s->name);
	}
}

static void
m_sid(struct sourceinfo *si, int parc, char *parv[])
{
	struct server *s;
	const char *inf;

	s = handle_server(si, parv[0], parv[2], atoi(parv[1]), parv[3]);

	if (s != NULL && s->uplink != me.me)
	{
		/* elicit PONG for EOB detection; pinging uplink is
		 * already done elsewhere -- jilles
		 */
		sts(":%s PING %s %s", ME, me.name, s->sid);
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
m_sethost(struct sourceinfo *si, int parc, char *parv[])
{
	strshare_unref(si->su->vhost);
	si->su->vhost = strshare_get(parv[0]);
}

static void
m_chghost(struct sourceinfo *si, int parc, char *parv[])
{
	struct user *u = user_find(parv[0]);

	if (!u)
		return;

	strshare_unref(u->vhost);
	u->vhost = strshare_get(parv[1]);
}

static void
m_motd(struct sourceinfo *si, int parc, char *parv[])
{
	handle_motd(si->su);
}

/* :src MD <type> <object name> <variable name> <value>
 * :virgule.cluenet.org MD client 1RGGFJS0G certfp :8ef42b0e3f6f7b9ca9ab841be34c4797e91be67a1bd8721217ef1f319debfe0e
 */
static void
m_md(struct sourceinfo *si, int parc, char *parv[])
{
	char *type = parv[0];
	char *obj = parv[1];
	char *key = parv[2];
	char *value = (parc >= 4) ? parv[3] : NULL;

	struct user *u;
	struct channel *c;

	if (!strcmp(type, "client"))
	{
		u = user_find(obj);
		if (!u)
		{
			slog(LG_DEBUG, "m_md(): got metadata '%s' for unknown %s '%s'", key, type, obj);
			return;
		}

		if (!strcmp(key, "certfp"))
		{
			handle_certfp(si, u, value);
		}
	}
}

static void
nick_group(struct hook_user_req *hdata)
{
	struct user *u;

	u = hdata->si->su != NULL && !irccasecmp(hdata->si->su->nick, hdata->mn->nick) ? hdata->si->su : user_find_named(hdata->mn->nick);
	if (!use_esvid && u != NULL && should_reg_umode(u))
		sts(":%s SVS2MODE %s +rd %lu", nicksvs.nick, u->nick,
				(unsigned long)u->ts);
}

static void
nick_ungroup(struct hook_user_req *hdata)
{
	struct user *u;

	u = hdata->si->su != NULL && !irccasecmp(hdata->si->su->nick, hdata->mn->nick) ? hdata->si->su : user_find_named(hdata->mn->nick);
	if (u != NULL && (!use_esvid || !nicksvs.no_nick_ownership))
		sts(":%s SVS2MODE %s -r+d 0", nicksvs.nick, u->nick);
}

static void
m_protoctl(struct sourceinfo *si, int parc, char *parv[])
{
	int i;

	has_protoctl = true;

	for (i = 0; i < parc; i++)
	{
		if (!irccasecmp(parv[i], "ESVID"))
			use_esvid = true;
		else if (!irccasecmp(parv[i], "MLOCK"))
			use_mlock = true;
		else if (!strncmp(parv[i], "SID=", 4))
		{
			ircd->uses_uid = true;
			mowgli_strlcpy(ts6sid, (parv[i] + 4), sizeof(ts6sid));
		}
	}
}

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "transport/rfc1459")
	MODULE_TRY_REQUEST_DEPENDENCY(m, "protocol/base36uid")

	server_login = &unreal_server_login;
	introduce_nick = &unreal_introduce_nick;
	quit_sts = &unreal_quit_sts;
	wallops_sts = &unreal_wallops_sts;
	join_sts = &unreal_join_sts;
	kick = &unreal_kick;
	msg = &unreal_msg;
	msg_global_sts = &unreal_msg_global_sts;
	notice_user_sts = &unreal_notice_user_sts;
	notice_global_sts = &unreal_notice_global_sts;
	notice_channel_sts = &unreal_notice_channel_sts;
	numeric_sts = &unreal_numeric_sts;
	kill_id_sts = &unreal_kill_id_sts;
	part_sts = &unreal_part_sts;
	kline_sts = &unreal_kline_sts;
	unkline_sts = &unreal_unkline_sts;
	xline_sts = &unreal_xline_sts;
	unxline_sts = &unreal_unxline_sts;
	qline_sts = &unreal_qline_sts;
	unqline_sts = &unreal_unqline_sts;
	topic_sts = &unreal_topic_sts;
	mode_sts = &unreal_mode_sts;
	ping_sts = &unreal_ping_sts;
	ircd_on_login = &unreal_on_login;
	ircd_on_logout = &unreal_on_logout;
	jupe = &unreal_jupe;
	sethost_sts = &unreal_sethost_sts;
	fnc_sts = &unreal_fnc_sts;
	invite_sts = &unreal_invite_sts;
	holdnick_sts = &unreal_holdnick_sts;
	chan_lowerts = &unreal_chan_lowerts;
	sasl_sts = &unreal_sasl_sts;
	sasl_mechlist_sts = &unreal_sasl_mechlist_sts;
	svslogin_sts = &unreal_svslogin_sts;
	quarantine_sts = &unreal_quarantine_sts;
	mlock_sts = &unreal_mlock_sts;
	is_extban = &unreal_is_extban;

	next_matching_ban = &unreal_next_matching_ban;
	mode_list = unreal_mode_list;
	ignore_mode_list = unreal_ignore_mode_list;
	status_mode_list = unreal_status_mode_list;
	prefix_mode_list = unreal_prefix_mode_list;
	user_mode_list = unreal_user_mode_list;
	ignore_mode_list_size = ARRAY_SIZE(unreal_ignore_mode_list);

	ircd = &Unreal;

	pcommand_add("PING", m_ping, 1, MSRC_USER | MSRC_SERVER);
	pcommand_add("PONG", m_pong, 1, MSRC_SERVER);
	pcommand_add("PRIVMSG", m_privmsg, 2, MSRC_USER);
	pcommand_add("NOTICE", m_notice, 2, MSRC_UNREG | MSRC_USER | MSRC_SERVER);
	pcommand_add("SJOIN", m_sjoin, 2, MSRC_USER | MSRC_SERVER);
	pcommand_add("PART", m_part, 1, MSRC_USER);
	pcommand_add("NICK", m_nick, 2, MSRC_USER | MSRC_SERVER);
	pcommand_add("UID", m_uid, 10, MSRC_SERVER);
	pcommand_add("QUIT", m_quit, 1, MSRC_USER);
	pcommand_add("UMODE2", m_umode, 1, MSRC_USER);
	pcommand_add("MODE", m_mode, 2, MSRC_USER | MSRC_SERVER);
	pcommand_add("KICK", m_kick, 2, MSRC_USER | MSRC_SERVER);
	pcommand_add("KILL", m_kill, 1, MSRC_USER | MSRC_SERVER);
	pcommand_add("SQUIT", m_squit, 1, MSRC_USER | MSRC_SERVER);
	pcommand_add("SERVER", m_server, 3, MSRC_UNREG | MSRC_SERVER);
	pcommand_add("SID", m_sid, 4, MSRC_UNREG | MSRC_SERVER);
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
	pcommand_add("SETHOST", m_sethost, 1, MSRC_USER);
	pcommand_add("CHGHOST", m_chghost, 2, MSRC_USER | MSRC_SERVER);
	pcommand_add("MOTD", m_motd, 1, MSRC_USER);
	pcommand_add("PROTOCTL", m_protoctl, 1, MSRC_UNREG);
	pcommand_add("SASL", m_sasl, 4, MSRC_SERVER);
	pcommand_add("MLOCK", m_mlock, 3, MSRC_SERVER);
	pcommand_add("MD", m_md, 3, MSRC_SERVER);

	hook_add_nick_group(nick_group);
	hook_add_nick_ungroup(nick_ungroup);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{

}

SIMPLE_DECLARE_MODULE_V1("protocol/unreal4", MODULE_UNLOAD_CAPABILITY_NEVER)
