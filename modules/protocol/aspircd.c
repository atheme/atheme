/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Copyright (c) 2005-2007 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains protocol support for AspIRCd 3 and AspIRCd 4.
 * AspIRCd - A Modern, Scalable, and Advanced IRC Daemon.
 * Copyright (C) - 2018-2021 Flox <flox -at- aspircd.com>
 * 
 */

#include "atheme.h"
#include "protocol/elemental-ircd.h"

static struct ircd aspircd = {
	.ircdname = "aspircd",
	.tldprefix = "$$",
	.uses_uid = true,
	.uses_rcommand = false,
	.uses_owner = true,
	.uses_protect = true,
	.uses_halfops = true,
	.uses_p10 = false,
	.uses_vhost = false,
	.oper_only_modes = CMODE_EXLIMIT | CMODE_PERM | CMODE_IMMUNE,
	.owner_mode = 0,
	.protect_mode = 0,
	.halfops_mode = 0,
	.owner_mchar = "+q",
	.protect_mchar = "+a",
	.halfops_mchar = "+h",
	.type = PROTOCOL_CHARYBDIS,
	.perm_mode = CMODE_CHANREG,
	.oimmune_mode = CMODE_IMMUNE,
	.ban_like_modes = "beIM",
	.except_mchar = 'e',
	.invex_mchar = 'I',
	.flags = IRCD_CIDR_BANS | IRCD_HOLDNICK,
};

static const struct cmode aspircd_mode_list[] = {
  { 'i', CMODE_INVITE },
  { 'm', CMODE_MOD    },
  { 'n', CMODE_NOEXT  },
  { 'p', CMODE_PRIV   },
  { 'r', CMODE_CHANREG},
  { 's', CMODE_SEC    },
  { 't', CMODE_TOPIC  },
  { 'c', CMODE_NOCOLOR},
  { 'R', CMODE_REGONLY},
  { 'z', CMODE_OPMOD  },
  { 'g', CMODE_FINVITE},
  { 'L', CMODE_EXLIMIT},
  { 'P', CMODE_PERM   },
  { 'F', CMODE_FTARGET},
  { 'Q', CMODE_DISFWD },
  { 'M', CMODE_IMMUNE },
  { 'T', CMODE_NONOTICE},
  { 'C', CMODE_NOCTCP },
  { 'D', CMODE_NOREPEAT },
  /* following modes are added as extensions */
  { 'S', CMODE_SSLONLY	 },
  { 'O', CMODE_OPERONLY  },
  { 'A', CMODE_ADMINONLY },

  { '\0', 0 }
};

static const struct cmode aspircd_status_mode_list[] = {
  { 'y', CSTATUS_IMMUNE	 },
  { 'q', CSTATUS_OWNER	 },
  { 'a', CSTATUS_PROTECT },
  { 'o', CSTATUS_OP	 },
  { 'h', CSTATUS_HALFOP  },
  { 'v', CSTATUS_VOICE	 },
  { '\0', 0 }
};

static const struct cmode aspircd_prefix_mode_list[] = {
  { '*', CSTATUS_IMMUNE	 },
  { '~', CSTATUS_OWNER	 },
  { '&', CSTATUS_PROTECT },
  { '@', CSTATUS_OP	 },
  { '%', CSTATUS_HALFOP  },
  { '+', CSTATUS_VOICE	 },
  { '\0', 0 }
};


static const struct cmode aspircd_user_mode_list[] = {
  { 'p', UF_IMMUNE   },
  { 'a', UF_ADMIN    },
  { 'i', UF_INVIS    },
  { 'o', UF_IRCOP    },
  { 'D', UF_DEAF     },
  { 'S', UF_SERVICE  },
  { '\0', 0 }
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "protocol/charybdis");

	mode_list = aspircd_mode_list;
	user_mode_list = aspircd_user_mode_list;
	status_mode_list = aspircd_status_mode_list;
	prefix_mode_list = aspircd_prefix_mode_list;

	ircd = &aspircd;
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{

}

SIMPLE_DECLARE_MODULE_V1("protocol/aspircd", MODULE_UNLOAD_CAPABILITY_NEVER)
