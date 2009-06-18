/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Copyright (c) 2005-2006 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains protocol support for plexus-based ircd.
 *
 * $Id: plexus.c 8301 2007-05-20 13:22:15Z jilles $
 */

/* option: set the netadmin umode +N */
#define USE_NETADMIN

#include "atheme.h"
#include "uplink.h"
#include "pmodule.h"
#include "protocol/plexus.h"

DECLARE_MODULE_V1("protocol/plexus", true, _modinit, NULL, "$Id: plexus.c 8301 2007-05-20 13:22:15Z jilles $", "Atheme Development Group <http://www.atheme.org>");

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
  { '\0', 0 }
};

/* *INDENT-ON* */

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

void _modinit(module_t * m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "protocol/ts6-generic");

	/* Symbol relocation voodoo. */
	ircd_on_login = &plexus_on_login;
	ircd_on_logout = &plexus_on_logout;
	sethost_sts = &plexus_sethost_sts;

	mode_list = plexus_mode_list;
	ignore_mode_list = plexus_ignore_mode_list;
	status_mode_list = plexus_status_mode_list;
	prefix_mode_list = plexus_prefix_mode_list;
	user_mode_list = plexus_user_mode_list;

	ircd = &PleXusIRCd;

	hook_add_event("nick_group");
	hook_add_hook("nick_group", (void (*)(void *))nick_group);
	hook_add_event("nick_ungroup");
	hook_add_hook("nick_ungroup", (void (*)(void *))nick_ungroup);

	m->mflags = MODTYPE_CORE;

	pmodule_loaded = true;
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
