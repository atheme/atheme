/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Copyright (c) 2005-2006 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains protocol support for plexus-based ircd.
 *
 */

/* option: set the netadmin umode +N */
#define USE_NETADMIN

#include "atheme.h"
#include "uplink.h"
#include "pmodule.h"
#include "protocol/plexus.h"

DECLARE_MODULE_V1("protocol/plexus", true, _modinit, NULL, PACKAGE_STRING, "Atheme Development Group <http://www.atheme.org>");

/* *INDENT-OFF* */

ircd_t PleXusIRCd = {
        "hybrid-7.2.1+plexus-3.x family",	/* IRCd name */
        "$$",                           /* TLD Prefix, used by Global. */
        false,                          /* Whether or not we use IRCNet/TS6 UID */
        false,                          /* Whether or not we use RCOMMAND */
        true,                           /* Whether or not we support channel owners. */
        true,                           /* Whether or not we support channel protection. */
        true,                           /* Whether or not we support halfops. */
	false,				/* Whether or not we use P10 */
	true,				/* Whether or not we use vHosts. */
	(CMODE_OPERONLY | CMODE_PERM),	/* Oper-only cmodes */
        CSTATUS_OWNER,	                /* Integer flag for owner channel flag. */
        CSTATUS_PROTECT,                  /* Integer flag for protect channel flag. */
        CSTATUS_HALFOP,                   /* Integer flag for halfops. */
        "+q",                           /* Mode we set for owner. */
        "+a",                           /* Mode we set for protect. */
        "+h",                           /* Mode we set for halfops. */
	PROTOCOL_PLEXUS,		/* Protocol type */
	CMODE_PERM,                     /* Permanent cmodes */
	0,                              /* Oper-immune cmode */
	"beI",                          /* Ban-like cmodes */
	'e',                            /* Except mchar */
	'I',                            /* Invex mchar */
	IRCD_CIDR_BANS                  /* Flags */
};

struct cmode_ plexus_mode_list[] = {
  { 'i', CMODE_INVITE },
  { 'm', CMODE_MOD    },
  { 'n', CMODE_NOEXT  },
  { 'p', CMODE_PRIV   },
  { 's', CMODE_SEC    },
  { 't', CMODE_TOPIC  },
  { 'c', CMODE_NOCOLOR },
  { 'R', CMODE_REGONLY },
  { 'O', CMODE_OPERONLY },
  { 'S', CMODE_STRIP },
  { 'K', CMODE_NOKNOCK },
  { 'N', CMODE_STICKY },
  { 'z', CMODE_PERM },
  { '\0', 0 }
};

struct extmode plexus_ignore_mode_list[] = {
  { '\0', 0 }
};

struct cmode_ plexus_status_mode_list[] = {
  { 'q', CSTATUS_OWNER   },
  { 'a', CSTATUS_PROTECT },
  { 'o', CSTATUS_OP      },
  { 'h', CSTATUS_HALFOP  },
  { 'v', CSTATUS_VOICE   },
  { '\0', 0 }
};

struct cmode_ plexus_prefix_mode_list[] = {
  { '~', CSTATUS_OWNER   },
  { '&', CSTATUS_PROTECT },
  { '@', CSTATUS_OP      },
  { '%', CSTATUS_HALFOP  },
  { '+', CSTATUS_VOICE   },
  { '\0', 0 }
};

struct cmode_ plexus_user_mode_list[] = {
  { 'a', UF_ADMIN    },
  { 'i', UF_INVIS    },
  { 'o', UF_IRCOP    },
  { 'N', UF_IMMUNE   },
  { 'D', UF_DEAF     },
  { '\0', 0 }
};

/* *INDENT-ON* */

static void plexus_introduce_nick(user_t *u)
{
	const char *umode = user_get_umodestr(u);

	if (ircd->uses_uid)
		sts(":%s UID %s 1 %lu %s %s %s 127.0.0.1 %s 0 %s :%s", me.numeric, u->nick, (unsigned long)u->ts, umode, u->user, u->host, u->uid, u->host, u->gecos);
	else
		sts("NICK %s 1 %lu %s %s %s %s :%s", u->nick, (unsigned long)u->ts, umode, u->user, u->host, me.name, u->gecos);
}

/* protocol-specific stuff to do on login */
static void plexus_on_login(user_t *u, myuser_t *account, const char *wantedhost)
{
	if (!me.connected || u == NULL)
		return;

	/* Can only do this for nickserv, and can only record identified
	 * state if logged in to correct nick, sorry -- jilles
	 */
	if (!should_reg_umode(u))
		return;

#ifdef USE_NETADMIN
	if (has_priv_user(u, PRIV_ADMIN))
		sts(":%s ENCAP * SVSMODE %s %lu +rdN %lu", nicksvs.nick, u->nick, (unsigned long)u->ts, (unsigned long)CURRTIME);
	else
#endif
		sts(":%s ENCAP * SVSMODE %s %lu +rd %lu", nicksvs.nick, u->nick, (unsigned long)u->ts, (unsigned long)CURRTIME);
}

/* protocol-specific stuff to do on login */
static bool plexus_on_logout(user_t *u, const char *account)
{
	if (!me.connected || u == NULL)
		return false;

	if (nicksvs.no_nick_ownership)
		return false;

#ifdef USE_NETADMIN
	sts(":%s ENCAP * SVSMODE %s %lu -rN", nicksvs.nick, u->nick, (unsigned long)u->ts);
#else
	sts(":%s ENCAP * SVSMODE %s %lu -r", nicksvs.nick, u->nick, (unsigned long)u->ts);
#endif
	return false;
}

static void plexus_sethost_sts(user_t *source, user_t *target, const char *host)
{
	if (!me.connected)
		return;

	if (irccasecmp(target->host, host))
		numeric_sts(me.me, 396, target, "%s :is now your hidden host (set by %s)", host, source->nick);
	else
	{
		numeric_sts(me.me, 396, target, "%s :hostname reset by %s", host, source->nick);
		sts(":%s ENCAP * SVSMODE %s %lu -x", CLIENT_NAME(source), CLIENT_NAME(target), (unsigned long)target->ts);
	}
	sts(":%s ENCAP * CHGHOST %s :%s", ME, CLIENT_NAME(target), host);
}

static void nick_group(hook_user_req_t *hdata)
{
	user_t *u;

	u = hdata->si->su != NULL && !irccasecmp(hdata->si->su->nick, hdata->mn->nick) ? hdata->si->su : user_find_named(hdata->mn->nick);
	if (u != NULL && should_reg_umode(u))
		sts(":%s ENCAP * SVSMODE %s %lu +rd %lu", nicksvs.nick, u->nick, (unsigned long)u->ts, (unsigned long)CURRTIME);
}

static void nick_ungroup(hook_user_req_t *hdata)
{
	user_t *u;

	u = hdata->si->su != NULL && !irccasecmp(hdata->si->su->nick, hdata->mn->nick) ? hdata->si->su : user_find_named(hdata->mn->nick);
	if (u != NULL && !nicksvs.no_nick_ownership)
		sts(":%s ENCAP * SVSMODE %s %lu -r", nicksvs.nick, u->nick, (unsigned long)u->ts);
}

static void m_uid(sourceinfo_t *si, int parc, char *parv[])
{
	server_t *s;
	user_t *u;

	/* got the right number of args for an introduction? */
	if (parc == 11)
	{
		s = si->s;
		slog(LG_DEBUG, "m_uid(): new user on `%s': %s", s->name, parv[0]);

		u = user_add(parv[0], parv[4], parv[5], parv[9], parv[6], parv[7], parv[10], s, atoi(parv[2]));
		if (u == NULL)
			return;

		user_mode(u, parv[3]);

		/* If server is not yet EOB we will do this later.
		 * This avoids useless "please identify" -- jilles
		 */
		if (s->flags & SF_EOB)
			handle_nickchange(user_find(parv[0]));
	}
	else
	{
		int i;
		slog(LG_DEBUG, "m_uid(): got UID with wrong number of params");

		for (i = 0; i < parc; i++)
			slog(LG_DEBUG, "m_uid():   parv[%d] = %s", i, parv[i]);
	}
}

void _modinit(module_t * m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "protocol/ts6-generic");

	/* Symbol relocation voodoo. */
	introduce_nick = &plexus_introduce_nick;
	ircd_on_login = &plexus_on_login;
	ircd_on_logout = &plexus_on_logout;
	sethost_sts = &plexus_sethost_sts;

	mode_list = plexus_mode_list;
	ignore_mode_list = plexus_ignore_mode_list;
	status_mode_list = plexus_status_mode_list;
	prefix_mode_list = plexus_prefix_mode_list;
	user_mode_list = plexus_user_mode_list;
	ignore_mode_list_size = ARRAY_SIZE(plexus_ignore_mode_list);

	ircd = &PleXusIRCd;

	pcommand_delete("UID");
	pcommand_add("UID", m_uid, 11, MSRC_SERVER);

	hook_add_event("nick_group");
	hook_add_nick_group(nick_group);
	hook_add_event("nick_ungroup");
	hook_add_nick_ungroup(nick_ungroup);

	m->mflags = MODTYPE_CORE;

	pmodule_loaded = true;
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
