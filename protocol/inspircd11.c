/*
 * Copyright (c) 2005-2006 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains protocol support for spanning tree 1.1 branch inspircd.
 *
 * $Id: inspircd11.c 6439 2006-09-24 15:35:55Z w00t $
 */

#include "atheme.h"
#include "uplink.h"
#include "pmodule.h"
#include "protocol/inspircd.h"

DECLARE_MODULE_V1("protocol/inspircd", TRUE, _modinit, NULL, "$Id: inspircd11.c 6439 2006-09-24 15:35:55Z w00t $", "InspIRCd Core Team <http://www.inspircd.org/>");

/* *INDENT-OFF* */

ircd_t InspIRCd = {
        "InspIRCd 1.1.x", /* IRCd name */
        "$",                            /* TLD Prefix, used by Global. */
        FALSE,                          /* Whether or not we use IRCNet/TS6 UID */
        FALSE,                          /* Whether or not we use RCOMMAND */
        TRUE,                           /* Whether or not we support channel owners. */
        TRUE,                           /* Whether or not we support channel protection. */
        TRUE,                           /* Whether or not we support halfops. */
		FALSE,				/* Whether or not we use P10 */
		TRUE,				/* Whether or not we use vHosts. */
		CMODE_OPERONLY,			/* Oper-only cmodes */
        CMODE_OWNER,                    /* Integer flag for owner channel flag. */
        CMODE_PROTECT,                  /* Integer flag for protect channel flag. */
        CMODE_HALFOP,                   /* Integer flag for halfops. */
        "+q",                           /* Mode we set for owner. */
        "+a",                           /* Mode we set for protect. */
        "+h",                           /* Mode we set for halfops. */
		PROTOCOL_INSPIRCD,		/* Protocol type */
		0,                              /* Permanent cmodes */
		"beIg",                         /* Ban-like cmodes */
		'e',                            /* Except mchar */
		'I'                             /* Invex mchar */
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
  { '\0', 0 }
};

static boolean_t check_flood(const char *, channel_t *, mychan_t *, user_t *, myuser_t *);
static boolean_t check_jointhrottle(const char *, channel_t *, mychan_t *, user_t *, myuser_t *);
static boolean_t check_forward(const char *, channel_t *, mychan_t *, user_t *, myuser_t *);

struct extmode inspircd_ignore_mode_list[] = {
  { 'f', check_flood },
  { 'j', check_jointhrottle },
  { 'L', check_forward },
  { '\0', 0 }
};

struct cmode_ inspircd_status_mode_list[] = {
  { 'q', CMODE_OWNER   },
  { 'a', CMODE_PROTECT },
  { 'o', CMODE_OP      },
  { 'h', CMODE_HALFOP  },
  { 'v', CMODE_VOICE   },
  { '\0', 0 }
};

struct cmode_ inspircd_prefix_mode_list[] = {
  { '~', CMODE_OWNER   },
  { '&', CMODE_PROTECT },
  { '@', CMODE_OP      },
  { '%', CMODE_HALFOP  },
  { '+', CMODE_VOICE   },
  { '\0', 0 }
};

/* CAPABilities */
static boolean_t has_servicesmod = false;
static boolean_t has_globopsmod = false;
static boolean_t has_remstatus = false; /* this is temporary, to keep inspircd_dev compatible with 1.1b1 */


/* *INDENT-ON* */

static boolean_t check_flood(const char *value, channel_t *c, mychan_t *mc, user_t *u, myuser_t *mu)
{

	return *value == '*' ? check_jointhrottle(value + 1, c, mc, u, mu) : check_jointhrottle(value, c, mc, u, mu);
}

static boolean_t check_jointhrottle(const char *value, channel_t *c, mychan_t *mc, user_t *u, myuser_t *mu)
{
	const char *p, *arg2;

	p = value, arg2 = NULL;
	while (*p != '\0')
	{
		if (*p == ':')
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
	if (p - arg2 > 10 || arg2 - value - 1 > 10 || !atoi(value) || !atoi(arg2))
		return FALSE;
	return TRUE;
}

static boolean_t check_forward(const char *value, channel_t *c, mychan_t *mc, user_t *u, myuser_t *mu)
{
	channel_t *target_c;
	mychan_t *target_mc;

	if (*value != '#' || strlen(value) > 50)
		return FALSE;
	if (u == NULL && mu == NULL)
		return TRUE;
	target_c = channel_find(value);
	target_mc = mychan_find(value);
	if (target_c == NULL && target_mc == NULL)
		return FALSE;
	return TRUE;
}

/* login to our uplink */
static uint8_t inspircd_server_login(void)
{
	int8_t ret;

	ret = sts("SERVER %s %s 0 :%s", me.name, curr_uplink->pass, me.desc);
	if (ret == 1)
		return 1;

	me.bursting = TRUE;
	sts("BURST");
	/* XXX: Being able to get this data as a char* would be nice - Brain */
        sts(":%s VERSION :atheme-%s. %s %s%s%s%s%s%s%s%s%s",me.name, version, me.name, (match_mapping) ? "A" : "",
								                      (me.loglevel & LG_DEBUG) ? "d" : "",
							                              (me.auth) ? "e" : "",
										      (config_options.flood_msgs) ? "F" : "",
										      (config_options.leave_chans) ? "l" : "",
										      (config_options.join_chans) ? "j" : "",
										      (!match_mapping) ? "R" : "",
										      (config_options.raw) ? "r" : "",
										      (runflags & RF_LIVE) ? "n" : "");
	services_init();
	return 0;
}

/* introduce a client */
static void inspircd_introduce_nick(user_t *u)
{
	/* :services-dev.chatspike.net NICK 1133994664 OperServ chatspike.net chatspike.net services +oii 0.0.0.0 :Operator Server  */
	sts(":%s NICK %ld %s %s %s %s +%s 0.0.0.0 :%s", me.name, u->ts, u->nick, u->host, u->host, u->user, "io", u->gecos);
	sts(":%s OPERTYPE Services", u->nick);
}

static void inspircd_quit_sts(user_t *u, char *reason)
{
	if (!me.connected)
		return;

	sts(":%s QUIT :%s", u->nick, reason);
}

/* WALLOPS wrapper */
static void inspircd_wallops(char *fmt, ...)
{
	va_list ap;
	char buf[BUFSIZE];
	char *sendernick = NULL;
	user_t *u;
	node_t *n;

	if (config_options.silent)
		return;
	if (me.me == NULL)
		return;

	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZE, fmt, ap);
	va_end(ap);

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
		slog(LG_ERROR, "wallops(): InspIRCD requires at least one pseudoclient module to be loaded to send wallops.");
		return;
	}

	sts(":%s GLOBOPS :%s", sendernick, buf);
}

/* join a channel */
static void inspircd_join_sts(channel_t *c, user_t *u, boolean_t isnew, char *modes)
{
	if (isnew)
	{
		sts(":s FJOIN %s %ld :@,%s", me.name, c->name, c->ts, u->nick);
		if (modes[0] && modes[1])
			sts(":%s FMODE %s %ld %s", me.name, c->name, c->ts, modes);
	}
	else
	{
		sts(":%s FJOIN %s %ld :@,%s", me.name, c->name, c->ts, u->nick);
	}
}

static void inspircd_chan_lowerts(channel_t *c, user_t *u)
{
	slog(LG_DEBUG, "inspircd_chan_lowerts(): lowering TS for %s to %ld", 
		c->name, (long)c->ts);

	if (has_remstatus == true)
	{
		/* instruct server to remove all status modes */
		sts(":%s REMSTATUS %s", me.name, c->name);
	}

	sts(":%s FJOIN %s %ld :@,%s", me.name, c->name, c->ts, u->nick);
	sts(":%s FMODE %s %ld %s", me.name, c->name, c->ts, channel_modes(c, TRUE));
}

/* kicks a user from a channel */
static void inspircd_kick(char *from, char *channel, char *to, char *reason)
{
	channel_t *chan = channel_find(channel);
	user_t *user = user_find(to);

	if (!chan || !user)
		return;

	sts(":%s KICK %s %s :%s", from, channel, to, reason);

	chanuser_delete(chan, user);
}

/* PRIVMSG wrapper */
static void inspircd_msg(char *from, char *target, char *fmt, ...)
{
	va_list ap;
	char buf[BUFSIZE];

	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZE, fmt, ap);
	va_end(ap);

	sts(":%s PRIVMSG %s :%s", from, target, buf);
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

static void inspircd_numeric_sts(char *from, int numeric, char *target, char *fmt, ...)
{
	va_list ap;
	char buf[BUFSIZE];

	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZE, fmt, ap);
	va_end(ap);

	sts(":%s PUSH %s ::%s %d %s %s", me.name, target, from, numeric, target, buf);
}

/* KILL wrapper */
static void inspircd_skill(char *from, char *nick, char *fmt, ...)
{
	va_list ap;
	char buf[BUFSIZE];

	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZE, fmt, ap);
	va_end(ap);

	sts(":%s KILL %s :[%s] Killed (%s (%s))", from, nick, me.name, from, buf);
}

/* PART wrapper */
static void inspircd_part(char *chan, char *nick)
{
	user_t *u = user_find(nick);
	channel_t *c = channel_find(chan);
	chanuser_t *cu;

	if (!u || !c)
		return;

	if (!(cu = chanuser_find(c, u)))
		return;

	sts(":%s PART %s :Leaving", u->nick, c->name);

	chanuser_delete(c, u);
}

/* server-to-server KLINE wrapper */
static void inspircd_kline_sts(char *server, char *user, char *host, long duration, char *reason)
{
	if (!me.connected)
		return;

	/* :services-dev.chatspike.net ADDLINE G test@test.com Brain 1133994664 0 :You are banned from this network */
	sts(":%s ADDLINE G %s@%s %s %ld %ld :%s", me.name, user, host, opersvs.nick, time(NULL), duration, reason);
}

/* server-to-server UNKLINE wrapper */
static void inspircd_unkline_sts(char *server, char *user, char *host)
{
	if (!me.connected)
		return;

	/* I know this looks wrong, but it's really not. Trust me. --w00t */
	sts(":%s GLINE %s@%s",opersvs.nick, user, host);
}

/* topic wrapper */
static void inspircd_topic_sts(char *channel, char *setter, time_t ts, char *topic)
{
	if (!me.connected)
		return;

	if (ts > CURRTIME)
		/*
		 * Restoring an old topic, can set proper setter/ts -- jilles
		 * No, on inspircd, newer topic TS wins -- w00t
		 */
		sts(":%s FTOPIC %s %ld %s :%s", me.name, channel, ts, setter, topic);
	else
		/* FTOPIC would require us to set an older topicts */
		sts(":%s TOPIC %s :%s", chansvs.nick, channel, topic);
}

/* mode wrapper */
static void inspircd_mode_sts(char *sender, char *target, char *modes)
{
	if (!me.connected)
		return;

	sts(":%s MODE %s %s", sender, target, modes);

}

/* ping wrapper */
static void inspircd_ping_sts(void)
{
	if (!me.connected)
		return;

	sts(":%s PING :%s", me.name, curr_uplink->name);
}

/* protocol-specific stuff to do on login */
static void inspircd_on_login(char *origin, char *user, char *wantedhost)
{
	if (!me.connected)
		return;

	sts(":%s METADATA %s accountname :%s", me.name, origin, user);
}

/* protocol-specific stuff to do on logout */
static boolean_t inspircd_on_logout(char *origin, char *user, char *wantedhost)
{
	if (!me.connected)
		return FALSE;

	sts(":%s METADATA %s accountname :", me.name, origin);
	return FALSE;
}

static void inspircd_jupe(char *server, char *reason)
{
	if (!me.connected)
		return;

	sts(":%s SERVER %s * 1 :%s", me.name, server, reason);
}

static void inspircd_sethost_sts(char *source, char *target, char *host)
{
	if (!me.connected)
		return;

	sts(":%s CHGHOST %s %s", source, target, host);
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

	if (!c)
		return;

	handle_topic_from(si, c, parv[2], atol(parv[1]), parv[3]);
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

static void m_fjoin(sourceinfo_t *si, int parc, char *parv[])
{
	/* FJOIN #flaps 1234 :@,fanny +%,arse ,tits ,breasts &~,poontang */
	channel_t *c;
	uint8_t userc;
	uint8_t i;
	uint8_t j;
	uint8_t nlen;
	boolean_t prefix = true;
	boolean_t keep_new_modes = true;
	char *userv[256];
	char prefixandnick[51];
	time_t ts;
	char *bounce;

	c = channel_find(parv[0]);
	ts = atol(parv[1]);

	if (!c)
	{
		slog(LG_DEBUG, "m_fjoin(): new channel: %s", parv[0]);
		c = channel_add(parv[0], ts);
		/* Tell the core to check mode locks now,
		 * otherwise it may only happen after the next
		 * join if everyone is akicked.
		 * Inspircd does not allow any redundant modes
		 * so this will not look ugly. -- jilles */
		channel_mode_va(NULL, c, 1, "+");
	}

	if (ts < c->ts)
	{
		/* the TS changed.  a TS change requires us to do
		 * bugger all except update the TS, because in InspIRCd
		 * remote servers enforce the TS change - Brain
		 *
		 * This is no longer the case with 1.1, we need to bounce their modes
		 * as well as lowering the channel ts. Do both. -- w00t
		 */
		c->ts = ts;
		hook_call_event("channel_tschange", c);
		keep_new_modes = false; /* bounce them! */
		slog(LG_DEBUG, "m_fjoin(): preparing to bounce modes on: %s", parv[0]);
	}

	/*
	 * ok, here's the difference from 1.0 -> 1.1:
	 * 1.0 sent p[3] and up as individual users, prefixed with their 'highest' prefix, @, % or +
	 * in 1.1, this is more complex. All prefixes are sent, with the additional caveat that modules
	 * can add their own prefixes (dangerous!) - therefore, don't just chanuser_add(), split the prefix
	 * out and ignore unknown prefixes (probably the safest option). --w00t
	 */
	userc = sjtoken(parv[parc - 1], ' ', userv);

	if (keep_new_modes == true)
	{
		for (i = 0; i < userc; i++)
		{
			nlen = 0;
			prefix = true;

			slog(LG_DEBUG, "m_fjoin(): processing user: %s", userv[i]);

			for (; *userv[i]; userv[i]++)
			{
				for (j = 0; prefix_mode_list[j].mode; j++)
				{
					if (*userv[i] == prefix_mode_list[j].mode)
					{
						prefixandnick[nlen++] = *userv[i];
						continue;
					}

					if (*userv[i] == ',')
					{
						userv[i]++;
						strncpy(prefixandnick + nlen, userv[i], sizeof(prefixandnick));
						nlen = strlen(prefixandnick); /* eww, but it makes life so much easier */
						break;
					}
				}
			}

			chanuser_add(c, prefixandnick);
		}
	}
	else
	{
		for (i = 0; i < userc; i++)
		{
			/* remove prefixes. build a list of what we need to bounce first. */
			slog(LG_DEBUG, "m_fjoin(): processing AND BOUNCING user: %s", userv[i]);
			nlen = 0;

			while (*userv[i])
			{
				if (*userv[i] == ',')
				{
					userv[i]++;
					break;
				}

				for (j = 0; prefix_mode_list[j].mode; j++)
				{
					if (*userv[i] == prefix_mode_list[j].mode)
					{
						prefixandnick[nlen] = status_mode_list[j].mode;
						nlen++;
					}
				}	

				userv[i]++;
			}

			prefixandnick[nlen] = '\0';

			bounce = prefixandnick;

			/* now we have a list of prefixes, bounce them */
			while (*bounce)
			{
				slog(LG_DEBUG, "m_fjoin(): bouncing prefix %c for %s", *bounce, userv[i]);
				modestack_mode_param(me.name, c->name, MTYPE_DEL, *bounce, userv[i]);
				bounce++;
			}

			/* add modeless user to channel */
			slog(LG_DEBUG, "m_fjoin(): adding modeless user %s", userv[i]);
			chanuser_add(c, userv[i]);
		}
	}

	if (c->nummembers == 0 && !(c->modes & ircd->perm_mode))
		channel_delete(c->name);
}

static void m_part(sourceinfo_t *si, int parc, char *parv[])
{
	uint8_t chanc;
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

		/* char *nick, char *user, char *host, char *vhost, char *ip, char *uid, char *gecos, server_t *server, uint32_t ts */
		u = user_add(parv[1], parv[4], parv[2], parv[3], parv[6], NULL, parv[7], si->s, atol(parv[0]));
		user_mode(u, parv[5]);

		/* If server is not yet EOB we will do this later.
		 * This avoids useless "please identify" -- jilles */
		if (si->s->flags & SF_EOB)
			handle_nickchange(u);
	}
	/* if it's only 1 then it's a nickname change */
	else if (parc == 1)
	{
		node_t *n;

                if (!si->su)
                {       
                        slog(LG_DEBUG, "m_nick(): server trying to change nick: %s", si->s != NULL ? si->s->name : "<none>");
                        return;
                }

		slog(LG_DEBUG, "m_nick(): nickname change from `%s': %s", si->su->nick, parv[0]);

		/* remove the current one from the list */
		n = node_find(si->su, &userlist[si->su->hash]);
		node_del(n, &userlist[si->su->hash]);
		node_free(n);

		/* change the nick */
		strlcpy(si->su->nick, parv[0], NICKLEN);

		/* readd with new nick (so the hash works) */
		n = node_create();
		si->su->hash = UHASH((unsigned char *)si->su->nick);
		node_add(si->su, n, &userlist[si->su->hash]);

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
	user_delete(si->su);
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
	/* :server.moo FMODE #blarp tshere +ntsklLg keymoo 1337 secks */
	if (*parv[0] == '#')
		channel_mode(NULL, channel_find(parv[0]), parc - 1, &parv[2]);
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
	slog(LG_DEBUG, "m_server(): new server: %s", parv[0]);
	server_add(parv[0], atoi(parv[2]), si->s ? si->s->name : me.name, NULL, parv[3]);

	if (cnt.server == 2)
		me.actual = sstrdup(parv[0]);
	else
	{
		/* elicit PONG for EOB detection; pinging uplink is
		 * already done elsewhere -- jilles
		 */
		sts(":%s PING %s %s", me.name, me.name, parv[0]);
	}

	me.recvsvr = TRUE;
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

static void m_join(sourceinfo_t *si, int parc, char *parv[])
{
	channel_t *c;

	c = channel_find(parv[0]);
	if (!c)
	{
		slog(LG_DEBUG, "m_join(): new channel: %s (TS and modes lost)", parv[0]);
		c = channel_add(parv[0], CURRTIME);
		channel_mode_va(NULL, c, 1, "+");
	}
	chanuser_add(c, si->su->nick);
}

static void m_sajoin(sourceinfo_t *si, int parc, char *parv[])
{
        si->su = user_find(parv[0]);
	m_join(si, 1, &parv[1]);
}

static void m_sapart(sourceinfo_t *si, int parc, char *parv[])
{
        si->su = user_find(parv[0]);
        m_part(si, 1, &parv[1]);
}

static void m_sanick(sourceinfo_t *si, int parc, char *parv[])
{
        si->su = user_find(parv[0]);
	m_nick(si, 1, &parv[1]);
}

static void m_samode(sourceinfo_t *si, int parc, char *parv[])
{
	si->su = NULL;
        si->s = server_find(me.name);
	m_mode(si, parc - 1, &parv[1]);
}

static void m_error(sourceinfo_t *si, int parc, char *parv[])
{
	slog(LG_INFO, "m_error(): error from server: %s", parv[0]);
}

static void m_idle(sourceinfo_t *si, int parc, char *parv[])
{
	if (parc == 1 && si->su != NULL)
	{
		sts(":%s IDLE %s %ld 0", parv[0], si->su->nick, CURRTIME);
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

		handle_burstlogin(u, parv[2]);
	}
}

static void m_capab(sourceinfo_t *si, int parc, char *parv[])
{
	if (strcasecmp(parv[0], "START") == 0)
	{
		/* reset all our previously recieved CAPAB stuff */
		has_servicesmod = false;
		has_globopsmod = false;
		has_remstatus = false;
	}
	else if (strcasecmp(parv[0], "CAPABILITIES") == 0)
	{
		/* check for ident length, etc */
		if (strstr(parv[1], "PROTOCOL=1101"))
		{
			/* XXX - this is temporary! */
			has_remstatus = true;
		}
	}
	else if (strcasecmp(parv[0], "MODULES") == 0)
	{
		if (strstr(parv[1], "m_services_account.so"))
		{
			has_servicesmod = true;
		}

		if (strstr(parv[1], "m_globops.so"))
		{
			has_globopsmod = true;
		}
	}
	else if (strcasecmp(parv[0], "END") == 0)
	{
		if (has_globopsmod == false)
		{
			fprintf(stderr, "atheme: you didn't load m_globops into inspircd. atheme support requires this module. exiting.\n");
			exit(EXIT_FAILURE);
		}

		if (has_servicesmod == false)
		{
			fprintf(stderr, "atheme: you didn't load m_services_account into inspircd. atheme support requires this module. exiting.\n");
			exit(EXIT_FAILURE);	
		}

		if (has_remstatus == false)
		{
			fprintf(stderr, "atheme: You have beta1 or older of InspIRCd-1.1, this is bad and means that incorrect people may be left opped on channels.");
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
	wallops = &inspircd_wallops;
	join_sts = &inspircd_join_sts;
	chan_lowerts = &inspircd_chan_lowerts;
	kick = &inspircd_kick;
	msg = &inspircd_msg;
	notice_user_sts = &inspircd_notice_user_sts;
	notice_global_sts = &inspircd_notice_global_sts;
	notice_channel_sts = &inspircd_notice_channel_sts;
	numeric_sts = &inspircd_numeric_sts;
	skill = &inspircd_skill;
	part = &inspircd_part;
	kline_sts = &inspircd_kline_sts;
	unkline_sts = &inspircd_unkline_sts;
	topic_sts = &inspircd_topic_sts;
	mode_sts = &inspircd_mode_sts;
	ping_sts = &inspircd_ping_sts;
	ircd_on_login = &inspircd_on_login;
	ircd_on_logout = &inspircd_on_logout;
	jupe = &inspircd_jupe;
	sethost_sts = &inspircd_sethost_sts;
	fnc_sts = &inspircd_fnc_sts;
	invite_sts = &inspircd_invite_sts;

	mode_list = inspircd_mode_list;
	ignore_mode_list = inspircd_ignore_mode_list;
	status_mode_list = inspircd_status_mode_list;
	prefix_mode_list = inspircd_prefix_mode_list;

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
	pcommand_add("SAMODE", m_samode, 3, MSRC_USER);
	pcommand_add("SAJOIN", m_sajoin, 2, MSRC_USER);
	pcommand_add("SAPART", m_sapart, 2, MSRC_USER);
	pcommand_add("SANICK", m_sanick, 2, MSRC_USER);
	pcommand_add("KICK", m_kick, 2, MSRC_USER | MSRC_SERVER);
	pcommand_add("KILL", m_kill, 1, MSRC_USER | MSRC_SERVER);
	pcommand_add("SQUIT", m_squit, 1, MSRC_USER | MSRC_SERVER);
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
	pcommand_add("OPERTYPE", m_opertype, 1, MSRC_USER);
	pcommand_add("METADATA", m_metadata, 3, MSRC_SERVER);
	pcommand_add("CAPAB", m_capab, 1, MSRC_UNREG);

	hook_add_event("server_eob");
	hook_add_hook("server_eob", (void (*)(void *))server_eob);

	m->mflags = MODTYPE_CORE;

	pmodule_loaded = TRUE;
}
