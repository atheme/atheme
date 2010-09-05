/*
 * Copyright (c) 2005-2006 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * The People's Democratic Republic of InspIRCd 2.0/2.1 support.
 *
 */

#include "atheme.h"
#include "uplink.h"
#include "pmodule.h"
#include "protocol/inspircd.h"

DECLARE_MODULE_V1("protocol/inspircd2", true, _modinit, NULL, PACKAGE_STRING, "Atheme Development Group <http://www.atheme.org/>");

/* *INDENT-OFF* */

ircd_t InspIRCd = {
        "InspIRCd 2.0/2.1", /* IRCd name */
        "$",                            /* TLD Prefix, used by Global. */
        true,                           /* Whether or not we use IRCNet/TS6 UID */
        false,                          /* Whether or not we use RCOMMAND */
        true,                          /* Whether or not we support channel owners. */
        true,                          /* Whether or not we support channel protection. */
        true,                           /* Whether or not we support halfops. */
	false,				/* Whether or not we use P10 */
	true,				/* Whether or not we use vHosts. */
	CMODE_OPERONLY | CMODE_PERM | CMODE_IMMUNE,	    /* Oper-only cmodes */
        CSTATUS_OWNER,                    /* Integer flag for owner channel flag. */
        CSTATUS_PROTECT,                  /* Integer flag for protect channel flag. */
        CSTATUS_HALFOP,                   /* Integer flag for halfops. */
        "+q",                           /* Mode we set for owner. */
        "+a",                           /* Mode we set for protect. */
        "+h",                           /* Mode we set for halfops. */
	PROTOCOL_INSPIRCD,		/* Protocol type */
	CMODE_PERM,                              /* Permanent cmodes */
	CMODE_IMMUNE,                              /* Oper-immune cmode */
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
  { 'A', CMODE_NOINVITE },
  { 'C', CMODE_NOCTCP   },
  { 'N', CMODE_STICKY   },
  { 'G', CMODE_CENSOR   },
  { 'P', CMODE_PERM     },
  { 'B', CMODE_NOCAPS	},
  { 'z', CMODE_SSLONLY	},
  { 'T', CMODE_NONOTICE },
  { 'u', CMODE_HIDING   },
  { 'Q', CMODE_PEACE    },
  { 'Y', CMODE_IMMUNE	},
  { 'D', CMODE_DELAYJOIN },
  { '\0', 0 }
};

static bool check_flood(const char *, channel_t *, mychan_t *, user_t *, myuser_t *);
static bool check_nickflood(const char *, channel_t *, mychan_t *, user_t *, myuser_t *);
static bool check_jointhrottle(const char *, channel_t *, mychan_t *, user_t *, myuser_t *);
static bool check_forward(const char *, channel_t *, mychan_t *, user_t *, myuser_t *);
static bool check_rejoindelay(const char *, channel_t *, mychan_t *, user_t *, myuser_t *);
static bool check_delaymsg(const char *, channel_t *, mychan_t *, user_t *, myuser_t *);

struct extmode inspircd_ignore_mode_list[] = {
  { 'f', check_flood },
  { 'F', check_nickflood },
  { 'j', check_jointhrottle },
  { 'L', check_forward },
  { 'J', check_rejoindelay },
  { 'd', check_delaymsg },
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

#define PROTOCOL_12BETA 1201 /* we do not support anything older than this */

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

static bool check_delaymsg(const char *value, channel_t *c, mychan_t *mc, user_t *u, myuser_t *mu)
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

void _modinit(module_t * m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "protocol/inspircd-aux");

	/* Symbol relocation voodoo. */
	mode_list = inspircd_mode_list;
	ignore_mode_list = inspircd_ignore_mode_list;
	status_mode_list = inspircd_status_mode_list;
	prefix_mode_list = inspircd_prefix_mode_list;
	user_mode_list = inspircd_user_mode_list;
	ignore_mode_list_size = ARRAY_SIZE(inspircd_ignore_mode_list);

	ircd = &InspIRCd;

	m->mflags = MODTYPE_CORE;

	pmodule_loaded = true;
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
