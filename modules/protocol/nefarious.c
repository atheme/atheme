/*
 * Copyright (c) 2005-2006 William Pitcock, et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains protocol support for P10 ircd's.
 * Some sources used: Run's documentation, beware's description,
 * raw data sent by nefarious.
 *
 */

#include "atheme.h"
#include "uplink.h"
#include "pmodule.h"
#include "protocol/nefarious.h"

DECLARE_MODULE_V1("protocol/nefarious", true, _modinit, NULL, PACKAGE_STRING, "Atheme Development Group <http://www.atheme.org>");

/* *INDENT-OFF* */

ircd_t Nefarious = {
        "Nefarious IRCU 0.4.0 or later", /* IRCd name */
        "$",                            /* TLD Prefix, used by Global. */
        true,                           /* Whether or not we use IRCNet/TS6 UID */
        false,                          /* Whether or not we use RCOMMAND */
        false,                          /* Whether or not we support channel owners. */
        false,                          /* Whether or not we support channel protection. */
        true,                           /* Whether or not we support halfops. */
	true,				/* Whether or not we use P10 */
	true,				/* Whether or not we use vhosts. */
	CMODE_PERM|CMODE_OPERONLY|CMODE_ADMONLY, /* Oper-only cmodes */
        0,                              /* Integer flag for owner channel flag. */
        0,                              /* Integer flag for protect channel flag. */
        CSTATUS_HALFOP,                   /* Integer flag for halfops. */
        "+",                            /* Mode we set for owner. */
        "+",                            /* Mode we set for protect. */
        "+",                            /* Mode we set for halfops. */
	PROTOCOL_NEFARIOUS,		/* Protocol type */
	CMODE_PERM,                     /* Permanent cmodes */
	0,                              /* Oper-immune cmode */
	"be",                           /* Ban-like cmodes */
	'e',                            /* Except mchar */
	'e',                            /* Invex mchar (+e also exempts from +i in Nefarious) */
	IRCD_CIDR_BANS                  /* Flags */
};

struct cmode_ nefarious_mode_list[] = {
  { 'a', CMODE_ADMONLY },
  { 'i', CMODE_INVITE },
  { 'm', CMODE_MOD    },
  { 'n', CMODE_NOEXT  },
  { 'p', CMODE_PRIV   },
  { 'r', CMODE_REGONLY },
  { 's', CMODE_SEC    },
  { 't', CMODE_TOPIC  },
  { 'z', CMODE_PERM   },
  { 'c', CMODE_NOCOLOR },
  { 'C', CMODE_NOCTCP },
  { 'D', CMODE_DELAYED },
  { 'Q', CMODE_NOQUIT },
  { 'N', CMODE_NONOTICE },
  { 'M', CMODE_SOFTMOD },
  { 'C', CMODE_NOCTCP },
  { 'S', CMODE_STRIP },
  { 'T', CMODE_NOAMSG },
  { 'O', CMODE_OPERONLY },
  { 'L', CMODE_SOFTPRIV },
  { 'Z', CMODE_SSLONLY },
  { '\0', 0 }
};

struct extmode nefarious_ignore_mode_list[] = {
  { '\0', 0 }
};

struct cmode_ nefarious_status_mode_list[] = {
  { 'o', CSTATUS_OP     },
  { 'h', CSTATUS_HALFOP },
  { 'v', CSTATUS_VOICE  },
  { '\0', 0 }
};

struct cmode_ nefarious_prefix_mode_list[] = {
  { '@', CSTATUS_OP     },
  { '%', CSTATUS_HALFOP },
  { '+', CSTATUS_VOICE  },
  { '\0', 0 }
};

struct cmode_ nefarious_user_mode_list[] = {
  { 'a', UF_ADMIN    },
  { 'i', UF_INVIS    },
  { 'o', UF_IRCOP    },
  { 'd', UF_DEAF     },
  { '\0', 0 }
};

static void check_hidehost(user_t *u);

/* *INDENT-ON* */

/* join a channel */
static void nefarious_join_sts(channel_t *c, user_t *u, bool isnew, char *modes)
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
		sts("%s M %s +o %s", u->uid, c->name, u->uid);
	}
}

/* kicks a user from a channel */
static void nefarious_kick(user_t *source, channel_t *c, user_t *u, const char *reason)
{
	sts("%s K %s %s :%s", source->uid, c->name, u->uid, reason);

	chanuser_delete(c, u);
}

/* NOTICE wrapper */
static void nefarious_notice_channel_sts(user_t *from, channel_t *target, const char *text)
{
	sts("%s O %s :%s", from ? from->uid : me.numeric, target->name, text);
}

/* topic wrapper */
static void nefarious_topic_sts(channel_t *c, user_t *source, const char *setter, time_t ts, time_t prevts, const char *topic)
{
	if (!me.connected || !c)
		return;

	/* for nefarious, set topicsetter iff we can set the proper topicTS */
	if (ts > prevts || prevts == 0)
		sts("%s T %s %s %lu %lu :%s", source->uid, c->name, setter, (unsigned long)c->ts, (unsigned long)ts, topic);
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
static void nefarious_mode_sts(char *sender, channel_t *target, char *modes)
{
	user_t *fptr = user_find_named(sender);

	if (!fptr)
		return;

	sts("%s M %s %s", fptr->uid, target->name, modes);
}

/* protocol-specific stuff to do on login */
static void nefarious_on_login(user_t *u, myuser_t *mu, const char *wantedhost)
{
	if (!me.connected || u == NULL)
		return;

	sts("%s AC %s R %s %lu", me.numeric, u->uid, entity(mu)->name,
			(unsigned long)mu->registered);
	check_hidehost(u);
}

/* P10 does not support logout, so kill the user
 * we can't keep track of which logins are stale and which aren't -- jilles 
 * Except we can in Nefarious --nenolod
 */
static bool nefarious_on_logout(user_t *u, const char *account)
{
	if (!me.connected || u == NULL)
		return false;

	sts("%s AC %s U", me.numeric, u->uid);
	if (u->flags & UF_HIDEHOSTREQ && me.hidehostsuffix != NULL &&
			!strcmp(u->vhost + strlen(u->vhost) - strlen(me.hidehostsuffix), me.hidehostsuffix))
	{
		slog(LG_DEBUG, "nefarious_on_logout(): removing +x vhost for %s: %s -> %s",
				u->nick, u->vhost, u->host);
		strlcpy(u->vhost, u->host, sizeof u->vhost);
	}

	return false;
}

static void nefarious_sethost_sts(user_t *source, user_t *target, const char *host)
{
	sts("%s FA %s %s", me.numeric, target->uid, host);
	/* need to set +x; this will be echoed */
	if (!(target->flags & UF_HIDEHOSTREQ))
		sts("%s M %s +x", me.numeric, target->uid);
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
	handle_topic_from(si, c, parc > 4 ? parv[parc - 4] : source, ts, parv[parc - 1]);
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
	int bantype;

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
	else if (ts < c->ts)
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
	if (parc < 3 || parv[2][0] != '+')
	{
		/* Tell the core to check mode locks now,
		 * otherwise it may only happen after the next
		 * mode change. -- jilles */
		channel_mode_va(NULL, c, 1, "+");
	}

	bantype = 'b';
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
			channel_mode(NULL, c, modec, modev);
		}
		else if (parv[j][0] == '%')
		{
			userc = sjtoken(parv[j++] + 1, ' ', userv);
			for (i = 0; i < userc; i++)
				if (!strcmp(userv[i], "~"))
					/* A ban "~" means exceptions are
					 * following */
					bantype = 'e';
				else
					chanban_add(c, userv[i], bantype);
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
					while (*p)
					{
						if (*p == 'o' || (*p >= '0' && *p <= '9' && !prefix[0]))
							prefix[prefix[0] ? 1 : 0] = '@';
						else if (*p == 'h')
							prefix[prefix[0] ? 1 : 0] = '%';
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

static void m_nick(sourceinfo_t *si, int parc, char *parv[])
{
	user_t *u;
	char ipstring[HOSTIPLEN];
	char *p;
	int i;

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
			i = 1;
			if (strchr(parv[5], 'r'))
			{
				p = strchr(parv[5+i], ':');
				if (p != NULL)
					*p++ = '\0';
				handle_burstlogin(u, parv[5+i], p ? atol(p) : 0);
				/* killed to force logout? */
				if (user_find(parv[parc - 2]) == NULL)
					return;
				i++;
			}
			if (strchr(parv[5], 'h'))
			{
				p = strchr(parv[5+i], '@');
				if (p == NULL)
					strlcpy(u->vhost, parv[5+i], sizeof u->vhost);
				else
				{
					strlcpy(u->vhost, p + 1, sizeof u->vhost);
					strlcpy(u->user, parv[5+i], sizeof u->user);
					p = strchr(u->user, '@');
					if (p != NULL)
						*p = '\0';
				}
				i++;
			}
			if (strchr(parv[5], 'f'))
			{
				strlcpy(u->vhost, parv[5+i], sizeof u->vhost);
				i++;
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
		slog(LG_DEBUG, "m_nick(): got NICK with wrong (%d) number of params", parc);

		for (i = 0; i < parc; i++)
			slog(LG_DEBUG, "m_nick():   parv[%d] = %s", i, parv[i]);
	}
}

static void m_mode(sourceinfo_t *si, int parc, char *parv[])
{
	user_t *u;
	char *p;

	if (*parv[0] == '#')
		channel_mode(NULL, channel_find(parv[0]), parc - 1, &parv[1]);
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
		if (strchr(parv[1], 'h'))
		{
			if (parc > 2)
			{
				/* assume +h */
				p = strchr(parv[2], '@');
				if (p == NULL)
					strlcpy(u->vhost, parv[2], sizeof u->vhost);
				else
				{
					strlcpy(u->vhost, p + 1, sizeof u->vhost);
					strlcpy(u->user, parv[2], sizeof u->user);
					p = strchr(u->user, '@');
					if (p != NULL)
						*p = '\0';
				}
				slog(LG_DEBUG, "m_mode(): user %s setting vhost %s@%s", u->nick, u->user, u->vhost);
			}
			else
			{
				/* must be -h */
				/* XXX we don't know the original ident */
				slog(LG_DEBUG, "m_mode(): user %s turning off vhost", u->nick);
				strlcpy(u->vhost, u->host, sizeof u->vhost);
				/* revert to +x vhost if applicable */
				check_hidehost(u);
			}
		}
	}
}

static void m_clearmode(sourceinfo_t *si, int parc, char *parv[])
{
	channel_t *chan;
	char *p, c;
	mowgli_node_t *n, *tn;
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
		{
			MOWGLI_ITER_FOREACH_SAFE(n, tn, chan->bans.head)
			{
				if (((chanban_t *)n->data)->type == 'b')
					chanban_delete(n->data);
			}
		}
		else if (c == 'e')
		{
			MOWGLI_ITER_FOREACH_SAFE(n, tn, chan->bans.head)
			{
				if (((chanban_t *)n->data)->type == 'e')
					chanban_delete(n->data);
			}
		}
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
		else if (c == 'h')
		{
			MOWGLI_ITER_FOREACH(n, chan->members.head)
			{
				cu = (chanuser_t *)n->data;
				cu->modes &= ~CSTATUS_HALFOP;
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

static void m_account(sourceinfo_t *si, int parc, char *parv[])
{
	user_t *u;
	static bool warned = false;

	u = user_find(parv[0]);
	if (u == NULL)
		return;
	if (strlen(parv[1]) != 1 || (parv[1][0] != 'U' && parc < 3))
	{
		if (!warned)
		{
			slog(LG_ERROR, "m_account(): got account with second parameter %s, %u parameters, Atheme requires F:EXTENDED_ACCOUNTS:TRUE", parv[1], parc);
			wallops("Invalid ACCOUNT syntax, check F:EXTENDED_ACCOUNTS:TRUE");
			warned = true;
		}
		return;
	}
	switch (parv[1][0])
	{
		case 'R':
			handle_setlogin(si, u, parv[2], parc > 3 ? atol(parv[3]) : 0);
			break;
		case 'M':
			if (!u->myuser)
				slog(LG_INFO, "Account rename (%s) for not logged in user %s, processing anyway",
						parv[2], u->nick);
			handle_setlogin(si, u, parv[2], 0);
			break;
		case 'U':
			handle_clearlogin(si, u);
			break;
		default:
			slog(LG_INFO, "Unrecognized ACCOUNT type %s", parv[1]);
	}
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
	MODULE_TRY_REQUEST_DEPENDENCY(m, "protocol/p10-generic");

	/* Symbol relocation voodoo. */
	join_sts = &nefarious_join_sts;
	kick = &nefarious_kick;
	notice_channel_sts = &nefarious_notice_channel_sts;
	topic_sts = &nefarious_topic_sts;
	mode_sts = &nefarious_mode_sts;
	ircd_on_login = &nefarious_on_login;
	ircd_on_logout = &nefarious_on_logout;
	sethost_sts = &nefarious_sethost_sts;

	mode_list = nefarious_mode_list;
	ignore_mode_list = nefarious_ignore_mode_list;
	status_mode_list = nefarious_status_mode_list;
	prefix_mode_list = nefarious_prefix_mode_list;
	user_mode_list = nefarious_user_mode_list;
	ignore_mode_list_size = ARRAY_SIZE(nefarious_ignore_mode_list);

	ircd = &Nefarious;

	/* override these */
	pcommand_delete("B");
	pcommand_delete("N");
	pcommand_delete("M");
	pcommand_delete("OM");
	pcommand_delete("CM");
	pcommand_delete("T");
	pcommand_delete("AC");
	pcommand_add("B", m_burst, 2, MSRC_SERVER);
	pcommand_add("N", m_nick, 2, MSRC_USER | MSRC_SERVER);
	pcommand_add("M", m_mode, 2, MSRC_USER | MSRC_SERVER);
	pcommand_add("OM", m_mode, 2, MSRC_USER); /* OPMODE, treat as MODE */
	pcommand_add("CM", m_clearmode, 2, MSRC_USER);
	pcommand_add("T", m_topic, 2, MSRC_USER | MSRC_SERVER);
	pcommand_add("AC", m_account, 2, MSRC_SERVER);

	m->mflags = MODTYPE_CORE;

	pmodule_loaded = true;
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
