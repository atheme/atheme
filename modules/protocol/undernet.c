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
#include "protocol/undernet.h"

DECLARE_MODULE_V1("protocol/undernet", true, _modinit, NULL, PACKAGE_STRING, "Atheme Development Group <http://www.atheme.org>");

/* *INDENT-OFF* */

ircd_t Undernet = {
        "ircu 2.10.11.07 or later",     /* IRCd name */
        "$",                            /* TLD Prefix, used by Global. */
        true,                           /* Whether or not we use IRCNet/TS6 UID */
        false,                          /* Whether or not we use RCOMMAND */
        false,                          /* Whether or not we support channel owners. */
        false,                          /* Whether or not we support channel protection. */
        false,                          /* Whether or not we support halfops. */
	true,				/* Whether or not we use P10 */
	false,				/* Whether or not we use vHosts. */
	0,				/* Oper-only cmodes */
        0,                              /* Integer flag for owner channel flag. */
        0,                              /* Integer flag for protect channel flag. */
        0,                              /* Integer flag for halfops. */
        "+",                            /* Mode we set for owner. */
        "+",                            /* Mode we set for protect. */
        "+",                            /* Mode we set for halfops. */
	PROTOCOL_UNDERNET,		/* Protocol type */
	0,                              /* Permanent cmodes */
	0,                              /* Oper-immune cmode */
	"b",                            /* Ban-like cmodes */
	0,                              /* Except mchar */
	0,                              /* Invex mchar */
	IRCD_CIDR_BANS                  /* Flags */
};

struct cmode_ undernet_mode_list[] = {
  { 'i', CMODE_INVITE },
  { 'm', CMODE_MOD    },
  { 'n', CMODE_NOEXT  },
  { 'p', CMODE_PRIV   },
  { 's', CMODE_SEC    },
  { 't', CMODE_TOPIC  },
  { 'D', CMODE_DELAYED},
  { '\0', 0 }
};

struct extmode undernet_ignore_mode_list[] = {
  { '\0', 0 }
};

struct cmode_ undernet_status_mode_list[] = {
  { 'o', CSTATUS_OP    },
  { 'v', CSTATUS_VOICE },
  { '\0', 0 }
};

struct cmode_ undernet_prefix_mode_list[] = {
  { '@', CSTATUS_OP    },
  { '+', CSTATUS_VOICE },
  { '\0', 0 }
};

struct cmode_ undernet_user_mode_list[] = {
  { 'i', UF_INVIS    },
  { 'o', UF_IRCOP    },
  { 'd', UF_DEAF     },
  { '\0', 0 }
};

/* *INDENT-ON* */

void _modinit(module_t * m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "protocol/p10-generic");

	mode_list = undernet_mode_list;
	ignore_mode_list = undernet_ignore_mode_list;
	status_mode_list = undernet_status_mode_list;
	prefix_mode_list = undernet_prefix_mode_list;
	user_mode_list = undernet_user_mode_list;
	ignore_mode_list_size = ARRAY_SIZE(undernet_ignore_mode_list);

	ircd = &Undernet;

	m->mflags = MODTYPE_CORE;

	pmodule_loaded = true;
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
