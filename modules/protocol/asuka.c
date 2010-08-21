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
#include "protocol/asuka.h"

DECLARE_MODULE_V1("protocol/asuka", true, _modinit, NULL, PACKAGE_STRING, "Atheme Development Group <http://www.atheme.org>");

/* *INDENT-OFF* */

ircd_t Asuka = {
        "Asuka 1.2.1 and later",        /* IRCd name */
        "$",                            /* TLD Prefix, used by Global. */
        true,                           /* Whether or not we use IRCNet/TS6 UID */
        false,                          /* Whether or not we use RCOMMAND */
        false,                          /* Whether or not we support channel owners. */
        false,                          /* Whether or not we support channel protection. */
        false,                          /* Whether or not we support halfops. */
	true,				/* Whether or not we use P10 */
	true,				/* Whether or not we use vhosts. */
	0,				/* Oper-only cmodes */
        0,                              /* Integer flag for owner channel flag. */
        0,                              /* Integer flag for protect channel flag. */
        0,                              /* Integer flag for halfops. */
        "+",                            /* Mode we set for owner. */
        "+",                            /* Mode we set for protect. */
        "+",                            /* Mode we set for halfops. */
	PROTOCOL_ASUKA,			/* Protocol type */
	0,                              /* Permanent cmodes */
	0,                              /* Oper-immune cmode */
	"b",                            /* Ban-like cmodes */
	0,                              /* Except mchar */
	0,                              /* Invex mchar */
	IRCD_CIDR_BANS                  /* Flags */
};

struct cmode_ asuka_mode_list[] = {
  { 'i', CMODE_INVITE },
  { 'm', CMODE_MOD    },
  { 'n', CMODE_NOEXT  },
  { 'p', CMODE_PRIV   },
  { 's', CMODE_SEC    },
  { 't', CMODE_TOPIC  },
  { 'c', CMODE_NOCOLOR },
  { 'C', CMODE_NOCTCP },
  { 'D', CMODE_DELAYED },
  { 'u', CMODE_NOQUIT },
  { 'N', CMODE_NONOTICE },
  { '\0', 0 }
};

struct extmode asuka_ignore_mode_list[] = {
  { '\0', 0 }
};

struct cmode_ asuka_status_mode_list[] = {
  { 'o', CSTATUS_OP    },
  { 'v', CSTATUS_VOICE },
  { '\0', 0 }
};

struct cmode_ asuka_prefix_mode_list[] = {
  { '@', CSTATUS_OP    },
  { '+', CSTATUS_VOICE },
  { '\0', 0 }
};

struct cmode_ asuka_user_mode_list[] = {
  { 'i', UF_INVIS    },
  { 'o', UF_IRCOP    },
  { 'd', UF_DEAF     },
  { '\0', 0 }
};

static void check_hidehost(user_t *u);

/* *INDENT-ON* */

/* NOTICE wrapper */
static void asuka_notice_channel_sts(user_t *from, channel_t *target, const char *text)
{
	if (target->modes & CMODE_NONOTICE)
	{
		/* asuka sucks */
		/* remove that stupid +N mode before it blocks our notice
		 * -- jilles */
		sts("%s M %s -N", from ? from->uid : me.numeric, target->name);
		target->modes &= ~CMODE_NONOTICE;
	}
	if (from == NULL || chanuser_find(target, from))
		sts("%s O %s :%s", from ? from->uid : me.numeric, target->name, text);
	else
		sts("%s O %s :[%s:%s] %s", me.numeric, target->name, from->nick, target->name, text);
}

static void asuka_wallchops(user_t *sender, channel_t *channel, const char *message)
{
	if (channel->modes & CMODE_NONOTICE)
	{
		/* asuka sucks */
		/* remove that stupid +N mode before it blocks our notice
		 * -- jilles */
		sts("%s M %s -N", sender->uid, channel->name);
		channel->modes &= ~CMODE_NONOTICE;
	}
	sts("%s WC %s :%s", sender->uid, channel->name, message);
}

/* protocol-specific stuff to do on login */
static void asuka_on_login(user_t *u, myuser_t *account, const char *wantedhost)
{
	if (!me.connected || u == NULL)
		return;

	sts("%s AC %s %s %lu", me.numeric, u->uid, entity(u->myuser)->name,
			(unsigned long)account->registered);
	check_hidehost(u);
}

/* P10 does not support logout, so kill the user
 * we can't keep track of which logins are stale and which aren't -- jilles */
static bool asuka_on_logout(user_t *u, const char *account)
{
	if (!me.connected)
		return false;

	if (u != NULL)
	{
		kill_user(NULL, u, "Forcing logout %s -> %s", u->nick, account);
		return true;
	}
	else
		return false;
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
	notice_channel_sts = &asuka_notice_channel_sts;
	wallchops = &asuka_wallchops;
	ircd_on_login = &asuka_on_login;
	ircd_on_logout = &asuka_on_logout;

	parse = &p10_parse;

	mode_list = asuka_mode_list;
	ignore_mode_list = asuka_ignore_mode_list;
	status_mode_list = asuka_status_mode_list;
	prefix_mode_list = asuka_prefix_mode_list;
	user_mode_list = asuka_user_mode_list;
	ignore_mode_list_size = ARRAY_SIZE(asuka_ignore_mode_list);

	ircd = &Asuka;

	/* override these */
	pcommand_delete("N");
	pcommand_delete("M");
	pcommand_delete("OM");
	pcommand_add("N", m_nick, 2, MSRC_USER | MSRC_SERVER);
	pcommand_add("M", m_mode, 2, MSRC_USER | MSRC_SERVER);
	pcommand_add("OM", m_mode, 2, MSRC_USER); /* OPMODE, treat as MODE */

	m->mflags = MODTYPE_CORE;

	pmodule_loaded = true;
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
