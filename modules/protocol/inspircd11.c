/*
 * Copyright (c) 2005-2006 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * The People's Democratic Republic of InspIRCd 1.1 support.
 *
 */

#include "atheme.h"
#include "uplink.h"
#include "pmodule.h"
#include "protocol/inspircd.h"

DECLARE_MODULE_V1("protocol/inspircd", true, _modinit, NULL, PACKAGE_STRING, "InspIRCd Core Team <http://www.inspircd.org/>");

/* *INDENT-OFF* */

ircd_t InspIRCd = {
        "InspIRCd 1.1.x", /* IRCd name */
        "$",                            /* TLD Prefix, used by Global. */
        false,                          /* Whether or not we use IRCNet/TS6 UID */
        false,                          /* Whether or not we use RCOMMAND */
        true,                          /* Whether or not we support channel owners. */
        true,                          /* Whether or not we support channel protection. */
        true,                           /* Whether or not we support halfops. */
		false,				/* Whether or not we use P10 */
		true,				/* Whether or not we use vHosts. */
		CMODE_OPERONLY,			/* Oper-only cmodes */
        CSTATUS_OWNER,                    /* Integer flag for owner channel flag. */
        CSTATUS_PROTECT,                  /* Integer flag for protect channel flag. */
        CSTATUS_HALFOP,                   /* Integer flag for halfops. */
        "+q",                           /* Mode we set for owner. */
        "+a",                           /* Mode we set for protect. */
        "+h",                           /* Mode we set for halfops. */
		PROTOCOL_INSPIRCD,		/* Protocol type */
		0,                              /* Permanent cmodes */
		0,                              /* Oper-immune cmode */
		"beIg",                         /* Ban-like cmodes */
		'e',                            /* Except mchar */
		'I',                            /* Invex mchar */
		IRCD_CIDR_BANS | IRCD_HOLDNICK  /* Flags */
};

struct cmode_ inspircd_mode_list[] = {
  { 'i', CMODE_INVITE   },
  { 'm', CMODE_MOD      },
  { 'n', CMODE_NOEXT    },
  { 'p', CMODE_PRIV     },
  { 's', CMODE_SEC      },
  { 't', CMODE_TOPIC    },
  { 'c', CMODE_NOCOLOR  },
  { 'M', CMODE_MODREG   },
  { 'R', CMODE_REGONLY  },
  { 'O', CMODE_OPERONLY },
  { 'S', CMODE_STRIP    },
  { 'K', CMODE_NOKNOCK  },
  { 'V', CMODE_NOINVITE },
  { 'C', CMODE_NOCTCP   },
  { 'N', CMODE_STICKY   },
  { 'G', CMODE_CENSOR   },
  { 'P', CMODE_NOCAPS   },
  { 'z', CMODE_SSLONLY	},
  { 'T', CMODE_NONOTICE },
  { 'u', CMODE_HIDING   },
  { 'Q', CMODE_PEACE    },
  { '\0', 0 }
};

static bool check_flood(const char *, channel_t *, mychan_t *, user_t *, myuser_t *);
static bool check_nickflood(const char *, channel_t *, mychan_t *, user_t *, myuser_t *);
static bool check_jointhrottle(const char *, channel_t *, mychan_t *, user_t *, myuser_t *);
static bool check_forward(const char *, channel_t *, mychan_t *, user_t *, myuser_t *);
static bool check_rejoindelay(const char *, channel_t *, mychan_t *, user_t *, myuser_t *);

struct extmode inspircd_ignore_mode_list[] = {
  { 'f', check_flood },
  { 'F', check_nickflood },
  { 'j', check_jointhrottle },
  { 'L', check_forward },
  { 'J', check_rejoindelay },
  { '\0', 0 }
};

struct cmode_ inspircd_status_mode_list[] = {
  { 'q', CSTATUS_OWNER   },
  { 'a', CSTATUS_PROTECT },
  { 'o', CSTATUS_OP      },
  { 'h', CSTATUS_HALFOP  },
  { 'v', CSTATUS_VOICE   },
  { '\0', 0 }
};

struct cmode_ inspircd_prefix_mode_list[] = {
  { '~', CSTATUS_OWNER   },
  { '&', CSTATUS_PROTECT },
  { '@', CSTATUS_OP      },
  { '%', CSTATUS_HALFOP  },
  { '+', CSTATUS_VOICE   },
  { '\0', 0 }
};

struct cmode_ inspircd_user_mode_list[] = {
  { 'i', UF_INVIS    },
  { 'o', UF_IRCOP    },
  { 'd', UF_DEAF     },
  { '\0', 0 }
};

/* CAPABilities */
static bool has_servicesmod = false;
static bool has_globopsmod = false;
static bool has_chghostmod = false;
static bool has_cbanmod = false;
static bool has_hidechansmod = false;
static bool has_svshold = false;
static int has_protocol = 0;

#define PROTOCOL_SNONOTICE 1102 /* has SNONOTICE and OPERNOTICE commands */
#define PROTOCOL_NEWTS 1104 /* does not send "confirming" FMODEs on TS changes and uses clear/ignore semantics also for FMODE */

/* *INDENT-ON* */

static bool check_flood(const char *value, channel_t *c, mychan_t *mc, user_t *u, myuser_t *mu)
{
	/* +F doesn't support *, so don't bother checking for it -- w00t */
	return check_jointhrottle(value, c, mc, u, mu);
}

static bool check_nickflood(const char *value, channel_t *c, mychan_t *mc, user_t *u, myuser_t *mu)
{
	return *value == '*' ? check_jointhrottle(value + 1, c, mc, u, mu) : check_jointhrottle(value, c, mc, u, mu);
}

static bool check_jointhrottle(const char *value, channel_t *c, mychan_t *mc, user_t *u, myuser_t *mu)
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
	if (p - arg2 > 10 || arg2 - value - 1 > 10 || !atoi(value) || !atoi(arg2))
		return false;
	return true;
}

static bool check_forward(const char *value, channel_t *c, mychan_t *mc, user_t *u, myuser_t *mu)
{
	channel_t *target_c;
	mychan_t *target_mc;

	if (*value != '#' || strlen(value) > 50)
		return false;
	if (u == NULL && mu == NULL)
		return true;
	target_c = channel_find(value);
	target_mc = mychan_find(value);
	if (target_c == NULL && target_mc == NULL)
		return false;
	return true;
}

static bool check_rejoindelay(const char *value, channel_t *c, mychan_t *mc, user_t *u, myuser_t *mu)
{
	const char *ch = value;

	while (*ch)
	{
		if (!isdigit(*ch))
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


/* login to our uplink */
static unsigned int inspircd_server_login(void)
{
	int ret;

	/* XXX This is a horrible hack, but I don't know how else */
	sleep(3);

	/* will be determined in CAPAB. */
	ircd->uses_owner = false;
	ircd->uses_protect = false;
	ircd->uses_halfops = false;

	ret = sts("SERVER %s %s 0 :%s", me.name, curr_uplink->pass, me.desc);
	if (ret == 1)
		return 1;

	me.bursting = true;
	sts("BURST");
        sts(":%s VERSION :atheme-%s. %s %s",me.name, version, me.name, get_conf_opts());
	services_init();
	return 0;
}

/* introduce a client */
static void inspircd_introduce_nick(user_t *u)
{
	/* :services-dev.chatspike.net NICK 1133994664 OperServ chatspike.net chatspike.net services +oii 0.0.0.0 :Operator Server  */
	const char *umode = user_get_umodestr(u);

	sts(":%s NICK %lu %s %s %s %s %s%s 0.0.0.0 :%s", me.name, (unsigned long)u->ts, u->nick, u->host, u->host, u->user, umode, has_hidechansmod ? "I" : "", u->gecos);
	if (is_ircop(u))
		sts(":%s OPERTYPE Services", u->nick);
}

static void inspircd_quit_sts(user_t *u, const char *reason)
{
	if (!me.connected)
		return;

	sts(":%s QUIT :%s", u->nick, reason);
}

/* WALLOPS wrapper */
static void inspircd_wallops_sts(const char *text)
{
	char *sendernick = NULL;
	user_t *u;
	node_t *n;

	if (has_protocol >= PROTOCOL_SNONOTICE)
	{
		/* XXX */
		if (has_globopsmod)
			sts(":%s SNONOTICE g :%s", me.name, text);
		else
			sts(":%s OPERNOTICE :%s", me.name, text);
		return;
	}

	if (me.me == NULL)
		return;

	if (is_internal_client(user_find_named(opersvs.nick)))
	{
		sendernick = opersvs.nick;
	}
	else
	{
		LIST_FOREACH(n, me.me->userlist.head)
		{
			u = (user_t *)n->data;

			sendernick = u->nick;
			break;
		}
	}

	if (sendernick == NULL)
	{
		/*
		 * this means we have no pseudoclients -- under present inspircd, servers cannot globops, and
		 * thus, we will need to bail -- slog, and let them know. --w00t
		 */
		slog(LG_ERROR, "wallops_sts(): InspIRCD requires at least one pseudoclient module to be loaded to send wallops.");
		return;
	}

	sts(":%s GLOBOPS :%s", sendernick, text);
}

/* join a channel */
static void inspircd_join_sts(channel_t *c, user_t *u, bool isnew, char *modes)
{
	if (isnew)
	{
		sts(":%s FJOIN %s %lu :@,%s", me.name, c->name, (unsigned long)c->ts, u->nick);
		if (modes[0] && modes[1])
			sts(":%s FMODE %s %lu %s", me.name, c->name, (unsigned long)c->ts, modes);
	}
	else
	{
		sts(":%s FJOIN %s %lu :@,%s", me.name, c->name, (unsigned long)c->ts, u->nick);
	}
}

static void inspircd_chan_lowerts(channel_t *c, user_t *u)
{
	slog(LG_DEBUG, "inspircd_chan_lowerts(): lowering TS for %s to %lu", 
		c->name, (unsigned long)c->ts);

	sts(":%s FJOIN %s %lu :@,%s", me.name, c->name, (unsigned long)c->ts, u->nick);
	sts(":%s FMODE %s %lu %s", me.name, c->name, (unsigned long)c->ts, channel_modes(c, true));
}

/* kicks a user from a channel */
static void inspircd_kick(user_t *source, channel_t *c, user_t *u, const char *reason)
{
	sts(":%s KICK %s %s :%s", source->nick, c->name, u->nick, reason);

	chanuser_delete(c, u);
}

/* PRIVMSG wrapper */
static void inspircd_msg(const char *from, const char *target, const char *fmt, ...)
{
	va_list ap;
	char buf[BUFSIZE];

	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZE, fmt, ap);
	va_end(ap);

	sts(":%s PRIVMSG %s :%s", from, target, buf);
}

static void inspircd_msg_global_sts(user_t *from, const char *mask, const char *text)
{
	sts(":%s PRIVMSG %s%s :%s", from ? from->nick : me.name, ircd->tldprefix, mask, text);
}

/* NOTICE wrapper */
static void inspircd_notice_user_sts(user_t *from, user_t *target, const char *text)
{
	sts(":%s NOTICE %s :%s", from ? from->nick : me.name, target->nick, text);
}

static void inspircd_notice_global_sts(user_t *from, const char *mask, const char *text)
{
	sts(":%s NOTICE %s%s :%s", from ? from->nick : me.name, ircd->tldprefix, mask, text);
}

static void inspircd_notice_channel_sts(user_t *from, channel_t *target, const char *text)
{
	sts(":%s NOTICE %s :%s", from ? from->nick : me.name, target->name, text);
}

static void inspircd_numeric_sts(server_t *from, int numeric, user_t *target, const char *fmt, ...)
{
	va_list ap;
	char buf[BUFSIZE];

	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZE, fmt, ap);
	va_end(ap);

	sts(":%s PUSH %s ::%s %d %s %s", from->name, target->nick, from->name, numeric, target->nick, buf);
}

/* KILL wrapper */
static void inspircd_kill_id_sts(user_t *killer, const char *id, const char *reason)
{
	if (killer != NULL)
		sts(":%s KILL %s :[%s] Killed (%s (%s))", killer->nick, id, me.name, killer->nick, reason);
	else
		sts(":%s KILL %s :[%s] Killed (%s (%s))", me.name, id, me.name, me.name, reason);
}

/* PART wrapper */
static void inspircd_part_sts(channel_t *c, user_t *u)
{
	sts(":%s PART %s :Leaving", u->nick, c->name);
}

/* server-to-server KLINE wrapper */
static void inspircd_kline_sts(const char *server, const char *user, const char *host, long duration, const char *reason)
{
	if (!me.connected)
		return;

	/* :services-dev.chatspike.net ADDLINE G test@test.com Brain 1133994664 0 :You are banned from this network */
	sts(":%s ADDLINE G %s@%s %s %lu %ld :%s", me.name, user, host, opersvs.nick, (unsigned long)CURRTIME, duration, reason);
}

/* server-to-server UNKLINE wrapper */
static void inspircd_unkline_sts(const char *server, const char *user, const char *host)
{
	if (!me.connected)
		return;

	/* I know this looks wrong, but it's really not. Trust me. --w00t */
	sts(":%s GLINE %s@%s",opersvs.nick, user, host);
}

/* server-to-server QLINE wrapper */
static void inspircd_qline_sts(const char *server, const char *name, long duration, const char *reason)
{

	if (!me.connected)
		return;

	if (*name != '#')
	{
		sts(":%s ADDLINE Q %s %s %lu %ld :%s", me.name, name, opersvs.nick, (unsigned long)CURRTIME, duration, reason);
		return;
	}

	if (has_cbanmod)
		sts(":%s CBAN %s %ld :%s", opersvs.nick, name, duration, reason);

	else
		slog(LG_INFO, "SQLINE: Could not set SQLINE on \2%s\2 due to m_cban not being loaded in inspircd.", name);
}

/* server-to-server UNQLINE wrapper */
static void inspircd_unqline_sts(const char *server, const char *name)
{

	if (!me.connected)
		return;

	if (*name != '#')
	{
		sts(":%s QLINE %s", opersvs.nick, name);
		return;
	}

	if (has_cbanmod)
		sts(":%s CBAN %s", opersvs.nick, name);

	else
		slog(LG_INFO, "SQLINE: Could not remove SQLINE on \2%s\2 due to m_cban not being loaded in inspircd.", name);
}

/* topic wrapper */
static void inspircd_topic_sts(channel_t *c, user_t *source, const char *setter, time_t ts, time_t prevts, const char *topic)
{
	if (!me.connected || !c)
		return;

	/* If possible, try to use FTOPIC
	 * Note that because TOPIC does not contain topicTS, it may be
	 * off a few seconds on other servers, hence the 60 seconds here.
	 * -- jilles */
	/* Restoring old topic */
	if (ts > prevts + 60 || prevts == 0)
	{
		sts(":%s FTOPIC %s %lu %s :%s", source->nick, c->name, (unsigned long)ts, setter, topic);
		return;
	}
	/* Tweaking a topic */
	else if (ts == prevts)
	{
		ts += 60;
		sts(":%s FTOPIC %s %lu %s :%s", source->nick, c->name, (unsigned long)ts, setter, topic);
		c->topicts = ts;
		return;
	}
	sts(":%s TOPIC %s :%s", source->nick, c->name, topic);
	c->topicts = CURRTIME;
}

/* mode wrapper */
static void inspircd_mode_sts(char *sender, channel_t *target, char *modes)
{
	if (!me.connected)
		return;

	if (has_protocol >= PROTOCOL_NEWTS)
	{
		/* FMODE from user is ok, use it */
		sts(":%s FMODE %s %lu %s", sender, target->name, (unsigned long)target->ts, modes);
	}
	else
	{
		sts(":%s MODE %s %s", sender, target->name, modes);
	}
}

/* ping wrapper */
static void inspircd_ping_sts(void)
{
	if (!me.connected)
		return;

	sts(":%s PING :%s", me.name, curr_uplink->name);
}

/* protocol-specific stuff to do on login */
static void inspircd_on_login(user_t *u, myuser_t *account, const char *wantedhost)
{
	if (!me.connected || u == NULL)
		return;

	sts(":%s METADATA %s accountname :%s", me.name, u->nick, account->name);
}

/* protocol-specific stuff to do on logout */
static bool inspircd_on_logout(user_t *u, const char *account)
{
	if (!me.connected || u == NULL)
		return false;

	sts(":%s METADATA %s accountname :", me.name, u->nick);
	return false;
}

static void inspircd_jupe(const char *server, const char *reason)
{
	server_t *s;
	if (!me.connected)
		return;

	s = server_find(server);
	if (s != NULL)
	{
		/* We need to wait for the RSQUIT to be processed -- jilles */
		sts(":%s RSQUIT :%s", opersvs.nick, server);
		s->flags |= SF_JUPE_PENDING;
	}
	else
	{
		/* Remove any previous jupe first */
		sts(":%s SQUIT %s :%s", me.name, server, reason);
		sts(":%s SERVER %s * 1 :%s", me.name, server, reason);
	}
}

static void inspircd_sethost_sts(user_t *source, user_t *target, const char *host)
{
	if (!me.connected)
		return;

	if (has_chghostmod)
		sts(":%s CHGHOST %s %s", source->nick, target->nick, host);

	else
		slog(LG_INFO, "VHOST: Could not set \2%s\2 due to m_chghost not being loaded in inspircd.", host);
}

static void inspircd_fnc_sts(user_t *source, user_t *u, char *newnick, int type)
{
	/* svsnick can only be sent by a server */
	sts(":%s SVSNICK %s %s %lu", me.name, u->nick, newnick,
		(unsigned long)(CURRTIME - 60));
}


/* invite a user to a channel */
static void inspircd_invite_sts(user_t *sender, user_t *target, channel_t *channel)
{
	sts(":%s INVITE %s %s", sender->nick, target->nick, channel->name);
}

static void inspircd_holdnick_sts(user_t *source, int duration, const char *nick, myuser_t *account)
{
	if (duration == 0)
	{
		if (has_svshold)
			/* remove SVSHOLD */
			sts(":%s SVSHOLD %s", source->nick, nick);
		else
			sts(":%s QLINE %s", opersvs.me->me->uid, nick);
	}
	else
	{
		if (has_svshold)
			sts(":%s SVSHOLD %s %d :Registered nickname.", source->nick, nick, duration);
		else
			sts(":%s ADDLINE Q %s %s %lu %d :%s", me.numeric, nick, opersvs.nick, (unsigned long)CURRTIME, duration, "Nickname Enforcer");
	}
}

static void m_topic(sourceinfo_t *si, int parc, char *parv[])
{
	channel_t *c = channel_find(parv[0]);

	if (!c)
		return;

	handle_topic_from(si, c, si->su->nick, time(NULL), parv[1]);
}

static void m_ftopic(sourceinfo_t *si, int parc, char *parv[])
{
	channel_t *c = channel_find(parv[0]);
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

static void m_ping(sourceinfo_t *si, int parc, char *parv[])
{
	/* reply to PING's */
	sts(":%s PONG %s", me.name, parv[0]);
}

static void m_pong(sourceinfo_t *si, int parc, char *parv[])
{
	handle_eob(si->s);

	if (irccasecmp(me.actual, si->s->name))
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

static void m_fjoin(sourceinfo_t *si, int parc, char *parv[])
{
	/* FJOIN #flaps 1234 :@,fanny +%,arse ,tits ,breasts &~,poontang */
	channel_t *c;
	unsigned int userc;
	unsigned int i;
	unsigned int j;
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
		/* Tell the core to check mode locks now,
		 * otherwise it may only happen after the next
		 * mode change.
		 * Inspircd does not allow any redundant modes
		 * so this will not look ugly. -- jilles */
		/* As discussed with Brain, if this is in a burst,
		 * an FMODE with the simple modes will follow so we
		 * can skip this. -- jilles */
		if (!me.bursting)
			channel_mode_va(NULL, c, 1, "+");
	}

	if (ts < c->ts)
	{
		chanuser_t *cu;
		node_t *n;

		/* the TS changed.  a TS change requires us to do
		 * bugger all except update the TS, because in InspIRCd
		 * remote servers enforce the TS change - Brain
		 *
		 * This is no longer the case with 1.1, we need to bounce their modes
		 * as well as lowering the channel ts. Do both. -- w00t
		 */

		if (has_protocol >= PROTOCOL_NEWTS)
		{
			clear_simple_modes(c);
			chanban_clear(c);
		}
		LIST_FOREACH(n, c->members.head)
		{
			cu = (chanuser_t *)n->data;
			if (cu->user->server == me.me)
			{
				/* it's a service, reop */
				sts(":%s FMODE %s %lu +o %s", me.name, c->name, (unsigned long)ts, cu->user->nick);
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
		keep_new_modes = false; /* ignore statuses */
	}

	/*
	 * ok, here's the difference from 1.0 -> 1.1:
	 * 1.0 sent p[3] and up as individual users, prefixed with their 'highest' prefix, @, % or +
	 * in 1.1, this is more complex. All prefixes are sent, with the additional caveat that modules
	 * can add their own prefixes (dangerous!) - therefore, don't just chanuser_add(), split the prefix
	 * out and ignore unknown prefixes (probably the safest option). --w00t
	 */
	userc = sjtoken(parv[parc - 1], ' ', userv);

	/* loop over all the users in this fjoin */
	for (i = 0; i < userc; i++)
	{
		nlen = 0;
		prefix = true;

		slog(LG_DEBUG, "m_fjoin(): processing user: %s", userv[i]);

		/*
		 * ok, now look at the chars in the nick.. we have something like "@%,w00t", but need @%w00t.. and
		 * we also want to ignore unknown prefixes.. loop through the chars
		 */
		for (; *userv[i]; userv[i]++)
		{
			/* does this char match a known prefix? */
			for (j = 0; prefix_mode_list[j].mode; j++)
			{
				/* yup. add it to the 'final' combination (@%w00t) */
				if (*userv[i] == prefix_mode_list[j].mode)
				{
					prefixandnick[nlen++] = *userv[i];
					continue;
				}
			}

			/* it's not a known prefix char, have we reached the end of the prefixes? */
			if (*userv[i] == ',')
			{
				/* yup, skip over the comma */
				userv[i]++;

				/* if we're ignoring status (keep_new_modes is false) then just add them to chan here.. */
				if (keep_new_modes == false)
				{
					/* This ignores the @%, and just adds 'w00t' to the chan */
					chanuser_add(c, userv[i]);
				}
				else
				{
					/* else, we do care about their prefixes.. add '@%w00t' to the chan */
					strlcpy(prefixandnick + nlen, userv[i], sizeof(prefixandnick) - nlen);
					chanuser_add(c, prefixandnick);
				}

				/* added them.. break out of this loop, which will move us to the next user */
				break;
			}
			else
			{
				/* unknown prefix char */
			}
		}

		/* go to the next user */
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

	/* :services-dev.chatspike.net NICK 1133994664 DevNull chatspike.net chatspike.net services +i 0.0.0.0 :/dev/null -- message sink */
	if (parc == 8)
	{
		slog(LG_DEBUG, "m_nick(): new user on `%s': %s", si->s->name, parv[1]);

		/* char *nick, char *user, char *host, char *vhost, char *ip, char *uid, char *gecos, server_t *server, unsigned int ts */
		u = user_add(parv[1], parv[4], parv[2], parv[3], parv[6], NULL, parv[7], si->s, atol(parv[0]));
		if (u == NULL)
			return;
		user_mode(u, parv[5]);

		/* If server is not yet EOB we will do this later.
		 * This avoids useless "please identify" -- jilles */
		if (si->s->flags & SF_EOB)
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

		/* No TS here for some reason, hmm */
		if (user_changenick(si->su, parv[0], si->su->ts))
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

static void m_quit(sourceinfo_t *si, int parc, char *parv[])
{
	slog(LG_DEBUG, "m_quit(): user leaving: %s", si->su->nick);

	/* user_delete() takes care of removing channels and so forth */
	user_delete(si->su, parv[0]);
}

static void m_saquit(sourceinfo_t *si, int parc, char *parv[])
{
	user_t *u = user_find(parv[0]);

	slog(LG_DEBUG, "m_saquit(): user leaving: %s", parv[0]);

	/* user_delete() takes care of removing channels and so forth */
	user_delete(u, parv[1]);
}

static void m_mode(sourceinfo_t *si, int parc, char *parv[])
{
	if (*parv[0] == '#')
		channel_mode(NULL, channel_find(parv[0]), parc - 1, &parv[1]);
	else
		user_mode(user_find(parv[0]), parv[1]);
}

static void m_fmode(sourceinfo_t *si, int parc, char *parv[])
{
	channel_t *c;
	bool onlydeop;
	time_t ts;
	const char *p;

	/* :server.moo FMODE #blarp tshere +ntsklLg keymoo 1337 secks */
	if (*parv[0] == '#')
	{
		c = channel_find(parv[0]);
		if (c == NULL)
		{
			slog(LG_DEBUG, "m_fmode(): nonexistant channel: %s", parv[0]);
			return;
		}
		ts = atoi(parv[1]);
		if (ts == c->ts && has_protocol < PROTOCOL_NEWTS)
		{
			onlydeop = true;
			p = parv[2];
			while (*p != '\0')
			{
				if (!strchr("-qaohv", *p))
					onlydeop = false;
				p++;
			}
			if (onlydeop && si->s != NULL)
			{
				/* ignore redundant deops generated
				 * if we lower the TS of a channel
				 * scenario: user with autoop privs recreates
				 * channel
				 * XXX could this ignore other stuff too?
				 * -- jilles */
				slog(LG_DEBUG, "m_fmode(): ignoring %s %s: incoming TS %lu is equal to our TS %lu, and only deops", parv[0], parv[2], (unsigned long)ts, (unsigned long)c->ts);
				return;
			}
		}
		else if (ts > c->ts)
		{
			if (has_protocol < PROTOCOL_NEWTS)
				slog(LG_DEBUG, "m_fmode(): accepting but should bounce %s %s: incoming TS %lu is newer than our TS %lu", parv[0], parv[2], (unsigned long)ts, (unsigned long)c->ts);
			else
				return;
		}
		else if (ts < c->ts)
			slog(LG_DEBUG, "m_fmode(): %s %s: incoming TS %lu is older than our TS %lu, possible desync", parv[0], parv[2], (unsigned long)ts, (unsigned long)c->ts);
		channel_mode(NULL, c, parc - 2, &parv[2]);
	}
	else
		user_mode(user_find(parv[0]), parv[2]);
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
	slog(LG_DEBUG, "m_squit(): server leaving: %s", parv[0]);
	server_delete(parv[0]);
}

static void m_server(sourceinfo_t *si, int parc, char *parv[])
{
	server_t *s;

	slog(LG_DEBUG, "m_server(): new server: %s", parv[0]);
	s = handle_server(si, parv[0], NULL, atoi(parv[2]), parv[3]);

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

static void m_motd(sourceinfo_t *si, int parc, char *parv[])
{
	handle_motd(si->su);
}

static void m_admin(sourceinfo_t *si, int parc, char *parv[])
{
	handle_admin(si->su);
}

static void m_away(sourceinfo_t *si, int parc, char *parv[])
{
	handle_away(si->su, parc >= 1 ? parv[0] : NULL);
}

static void m_join(sourceinfo_t *si, int parc, char *parv[])
{
	channel_t *c;

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

static void m_sajoin(sourceinfo_t *si, int parc, char *parv[])
{
        si->su = user_find(parv[0]);
	if (si->su == NULL)
		return;
	m_join(si, 1, &parv[1]);
}

static void m_sapart(sourceinfo_t *si, int parc, char *parv[])
{
        si->su = user_find(parv[0]);
	if (si->su == NULL)
		return;
        m_part(si, 1, &parv[1]);
}

static void m_sanick(sourceinfo_t *si, int parc, char *parv[])
{
        si->su = user_find(parv[0]);
	if (si->su == NULL)
		return;
	m_nick(si, 1, &parv[1]);
}

static void m_samode(sourceinfo_t *si, int parc, char *parv[])
{
	/* note that SAMODE is not checked in any way before propagation,
	 * and only works on channels, not users */
	channel_mode(NULL, channel_find(parv[0]), parc - 1, &parv[1]);
}

static void m_error(sourceinfo_t *si, int parc, char *parv[])
{
	slog(LG_INFO, "m_error(): error from server: %s", parv[0]);
}

static void m_idle(sourceinfo_t *si, int parc, char *parv[])
{
	if (parc == 1 && si->su != NULL)
	{
		sts(":%s IDLE %s %lu 0", parv[0], si->su->nick, (unsigned long)CURRTIME);
	}
	else
	{
		slog(LG_INFO, "m_idle(): Received an IDLE response but we didn't WHOIS anybody!");
	}
}

static void m_opertype(sourceinfo_t *si, int parc, char *parv[])
{
	/*
	 * Hope this works.. InspIRCd OPERTYPE means user is an oper, mark them so
	 * Later, we may want to look at saving their OPERTYPE for informational
	 * purposes, or not. --w00t
	 */
	user_mode(si->su, "+o");
}

static void m_fhost(sourceinfo_t *si, int parc, char *parv[])
{
	strlcpy(si->su->vhost, parv[0], HOSTLEN);
}

/*
 * :<source server> METADATA <channel|user> <key> :<value>
 * The sole piece of metadata we're interested in is 'accountname', set by Services,
 * and kept by ircd.
 *
 * :services.barafranca METADATA w00t accountname :w00t
 */

static void m_metadata(sourceinfo_t *si, int parc, char *parv[])
{
	user_t *u;

	if (!irccasecmp(parv[1], "accountname"))
	{
		/* find user */
		u = user_find(parv[0]);

		if (u == NULL)
			return;

		if (parv[2][0] == '\0')
			handle_clearlogin(si, u);
		else if (si->s->flags & SF_EOB)
			handle_setlogin(si, u, parv[2], 0);
		else
			handle_burstlogin(u, parv[2], 0);
	}
}

/*
 * rsquit:
 *  remote/request squit
 *  when squitting a remote server, inspircd sends RSQUIT along the tree until it reaches the server that has
 *  the server to be squit as a local connection, which should then close it's connection and send SQUIT back
 *  to the rest of the network.
 */
static void m_rsquit(sourceinfo_t *si, int parc, char *parv[])
{
	sts(":%s SQUIT %s :Jupe removed by %s", me.name, parv[0], si->su->nick);
}

static void m_capab(sourceinfo_t *si, int parc, char *parv[])
{
	int i, varc;
	char *varv[256];

	if (strcasecmp(parv[0], "START") == 0)
	{
		/* reset all our previously received CAPAB stuff */
		has_servicesmod = false;
		has_globopsmod = false;
		has_chghostmod = false;
		has_cbanmod = false;
		has_hidechansmod = false;
		has_svshold = false;
		has_protocol = 0;
	}
	else if (strcasecmp(parv[0], "CAPABILITIES") == 0 && parc > 1)
	{
		varc = sjtoken(parv[1], ' ', varv);
		for (i = 0; i < varc; i++)
		{
			if (!strncmp(varv[i], "PROTOCOL=", 9))
				has_protocol = atoi(varv[i] + 9);
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
			/* XXX check/store /CHANMAX/IDENTMAX */
		}
	}
	else if (strcasecmp(parv[0], "MODULES") == 0 && parc > 1)
	{
		if (strstr(parv[1], "m_services_account.so"))
		{
			has_servicesmod = true;
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
		if (strstr(parv[1], "m_hidechans.so"))
		{
			has_hidechansmod = true;
		}
		if (strstr(parv[1], "m_svshold.so"))
		{
			has_svshold = true;
		}
		TAINT_ON(strstr(parv[1], "m_invisible.so") != NULL, "invisible is not presently supported correctly in atheme, and won't be due to ethical obligations");
	}
	else if (strcasecmp(parv[0], "END") == 0)
	{
		if (has_globopsmod == false && has_protocol < PROTOCOL_SNONOTICE)
		{
			slog(LG_ERROR, "m_capab(): you didn't load m_globops into inspircd. atheme support requires this module. exiting.");
			exit(EXIT_FAILURE);
		}

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
	else
	{
		slog(LG_DEBUG, "m_capab(): unknown CAPAB type %s - out of date protocol module?", parv[0]);
	}
}

/* Server ended their burst: warn all their users if necessary -- jilles */
static void server_eob(server_t *s)
{
	node_t *n;

	LIST_FOREACH(n, s->userlist.head)
	{
		handle_nickchange((user_t *)n->data);
	}
}

void _modinit(module_t * m)
{
	/* Symbol relocation voodoo. */
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

	mode_list = inspircd_mode_list;
	ignore_mode_list = inspircd_ignore_mode_list;
	status_mode_list = inspircd_status_mode_list;
	prefix_mode_list = inspircd_prefix_mode_list;
	user_mode_list = inspircd_user_mode_list;
	ignore_mode_list_size = ARRAY_SIZE(inspircd_ignore_mode_list);

	ircd = &InspIRCd;

	pcommand_add("PING", m_ping, 1, MSRC_USER | MSRC_SERVER);
	pcommand_add("PONG", m_pong, 1, MSRC_SERVER);
	pcommand_add("PRIVMSG", m_privmsg, 2, MSRC_USER);
	pcommand_add("NOTICE", m_notice, 2, MSRC_USER | MSRC_SERVER);
	pcommand_add("FJOIN", m_fjoin, 3, MSRC_SERVER);
	pcommand_add("PART", m_part, 1, MSRC_USER);
	pcommand_add("NICK", m_nick, 1, MSRC_USER | MSRC_SERVER);
	pcommand_add("QUIT", m_quit, 1, MSRC_USER);
	pcommand_add("MODE", m_mode, 2, MSRC_USER | MSRC_SERVER);
	pcommand_add("FMODE", m_fmode, 3, MSRC_USER | MSRC_SERVER);
	pcommand_add("SAMODE", m_samode, 2, MSRC_USER);
	pcommand_add("SAJOIN", m_sajoin, 2, MSRC_USER);
	pcommand_add("SAPART", m_sapart, 2, MSRC_USER);
	pcommand_add("SANICK", m_sanick, 2, MSRC_USER);
	pcommand_add("SAQUIT", m_saquit, 1, MSRC_USER);
	pcommand_add("KICK", m_kick, 2, MSRC_USER | MSRC_SERVER);
	pcommand_add("KILL", m_kill, 1, MSRC_USER | MSRC_SERVER);
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
	pcommand_add("FHOST", m_fhost, 1, MSRC_USER);
	pcommand_add("IDLE", m_idle, 1, MSRC_USER);
	pcommand_add("AWAY", m_away, 0, MSRC_USER);
	pcommand_add("OPERTYPE", m_opertype, 1, MSRC_USER);
	pcommand_add("METADATA", m_metadata, 3, MSRC_SERVER);
	pcommand_add("CAPAB", m_capab, 1, MSRC_UNREG | MSRC_SERVER);

	hook_add_event("server_eob");
	hook_add_server_eob(server_eob);

	m->mflags = MODTYPE_CORE;

	pmodule_loaded = true;
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
