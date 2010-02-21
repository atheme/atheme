/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Copyright (c) 2005-2008 Atheme Development Group
 * Copyright (c) 2008 ShadowIRCd Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains protocol support for shadowircd.
 *
 * $Id$
 */

/* option: enable hosts with slashes ('/') */
/* #define HOSTSLASH */

#include "atheme.h"
#include "uplink.h"
#include "pmodule.h"
#include "protocol/charybdis.h"
#include "protocol/shadowircd.h"

DECLARE_MODULE_V1("protocol/shadowircd", true, _modinit, NULL, "$Id$", "ShadowIRCd Development Group <http://www.shadowircd.net>");

/* *INDENT-OFF* */

ircd_t ShadowIRCd = {
        "ShadowIRCd 5+",		/* IRCd name */
        "$$",                           /* TLD Prefix, used by Global. */
        true,                           /* Whether or not we use IRCNet/TS6 UID */
        false,                          /* Whether or not we use RCOMMAND */
        true,                           /* Whether or not we support channel owners. */
        true,                           /* Whether or not we support channel protection. */
        true,                           /* Whether or not we support halfops. */
	false,				/* Whether or not we use P10 */
	false,				/* Whether or not we use vHosts. */
	CMODE_EXLIMIT | CMODE_PERM | CMODE_IMMUNE, /* Oper-only cmodes */
        CSTATUS_PROTECT,                  /* Integer flag for owner channel flag. */
        CSTATUS_PROTECT,                  /* Integer flag for protect channel flag. */
        CSTATUS_HALFOP,                   /* Integer flag for halfops. */
        "+a",                           /* Mode we set for owner. */
        "+a",                           /* Mode we set for protect. */
        "+h",                           /* Mode we set for halfops. */
	PROTOCOL_SHADOWIRCD,		/* Protocol type */
	CMODE_PERM,                     /* Permanent cmodes */
	CMODE_IMMUNE,                   /* Oper-immune cmode */
	"beIq",                         /* Ban-like cmodes */
	'e',                            /* Except mchar */
	'I',                            /* Invex mchar */
	IRCD_CIDR_BANS | IRCD_HOLDNICK  /* Flags */
};

struct cmode_ shadowircd_mode_list[] = {
  { 'i', CMODE_INVITE },
  { 'm', CMODE_MOD    },
  { 'n', CMODE_NOEXT  },
  { 'p', CMODE_PRIV   },
  { 's', CMODE_SEC    },
  { 't', CMODE_TOPIC  },
  { 'c', CMODE_NOCOLOR},		/* cmodes/nocolour.c */
  { 'r', CMODE_REGONLY},
  { 'z', CMODE_OPMOD  },
  { 'g', CMODE_FINVITE},
  { 'L', CMODE_EXLIMIT},
  { 'P', CMODE_PERM   },
  { 'F', CMODE_FTARGET},
  { 'Q', CMODE_DISFWD },
  { 'M', CMODE_IMMUNE },		/* cmodes/no_oper_kick */
  { 'C', CMODE_NOCTCP },		/* cmodes/noctcp */
  { 'A', CMODE_ADMINONLY },		/* cmodes/adminonly */
  { 'O', CMODE_OPERONLY },		/* cmodes/operonly */
  { 'Z', CMODE_SSLONLY },		/* cmodes/sslonly */
  { 'D', CMODE_NOACTIONS },		/* cmodes/noactions */
  { 'X', CMODE_NOSPAM },		/* cmodes/nospam */
  { 'S', CMODE_SCOLOR },		/* cmodes/stripcolour */
  { 'T', CMODE_NONOTICE },		/* cmodes/nonotice */
  { 'R', CMODE_MODNOREG },		/* cmodes/quietunreg */
  { 'G', CMODE_NOCAPS },		/* cmodes/nocaps */
  { 'E', CMODE_NOKICKS },		/* cmodes/nokicks */
  { 'N', CMODE_NONICKS },		/* cmodes/nonicks */
  { 'K', CMODE_NOREPEAT },		/* cmodes/norepeat */
  { '\0', 0 }
};

struct cmode_ shadowircd_status_mode_list[] = {
  { 'a', CSTATUS_PROTECT },
  { 'o', CSTATUS_OP    },
  { 'h', CSTATUS_HALFOP },
  { 'v', CSTATUS_VOICE },
  { '\0', 0 }
};

struct cmode_ shadowircd_prefix_mode_list[] = {
  { '!', CSTATUS_PROTECT },
  { '@', CSTATUS_OP    },
  { '%', CSTATUS_HALFOP },
  { '+', CSTATUS_VOICE },
  { '\0', 0 }
};

struct cmode_ shadowircd_user_mode_list[] = {
  { 'a', UF_ADMIN    },
  { 'm', UF_IMMUNE   },			/* immune.c */
  { 'i', UF_INVIS    },
  { 'o', UF_IRCOP    },
  { 'D', UF_DEAF     },
  { '\0', 0 }
};

/* *INDENT-ON* */

#ifdef HOSTSLASH
static bool shadowircd_is_valid_hostslash(const char *host)
{
        const char *p;
        bool dot = false;

        if (*host == '.' || *host == '/' || *host == ':')
                return false;

        for (p = host; *p != '\0'; p++)
        {
                if (*p == '.' || *p == ':' || *p == '/')
                        dot = true;
                else if (!((*p >= '0' && *p <= '9') || (*p >= 'A' && *p <= 'Z') ||
                                        (*p >= 'a' && *p <= 'z') || *p == '-'))
                        return false;
        }
        /* hyperion allows a trailing / but RichiH does not want it, whatever */
        if (dot && p[-1] == '/')
                return false;
        return dot;
}
#endif

void _modinit(module_t * m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "protocol/charybdis");

	mode_list = shadowircd_mode_list;
	user_mode_list = shadowircd_user_mode_list;
	status_mode_list = shadowircd_status_mode_list;
	prefix_mode_list = shadowircd_prefix_mode_list;

#ifdef HOSTSLASH
	is_valid_host = &shadowircd_is_valid_hostslash;
#endif

	ircd = &ShadowIRCd;

	m->mflags = MODTYPE_CORE;

	pmodule_loaded = true;
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
