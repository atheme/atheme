/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2003-2004 E. Will, et al.
 * Copyright (C) 2005-2008 Atheme Project (http://atheme.org/)
 * Copyright (C) 2008-2010 ShadowIRCd Development Group
 * Copyright (C) 2013 PonyChat Development Group
 *
 * This file contains protocol support for ponychat-ircd.
 */

#include <atheme.h>
#include <atheme/protocol/elemental-ircd.h>

static struct ircd elemental_ircd = {
	.ircdname = "elemental-ircd",
	.tldprefix = "$$",
	.uses_uid = true,
	.uses_rcommand = false,
	.uses_owner = true,
	.uses_protect = true,
	.uses_halfops = true,
	.uses_p10 = false,
	.uses_vhost = false,
	.oper_only_modes = CMODE_EXLIMIT | CMODE_PERM | CMODE_IMMUNE,
	.owner_mode = CSTATUS_OWNER,
	.protect_mode = CSTATUS_PROTECT,
	.halfops_mode = CSTATUS_HALFOP,
	.owner_mchar = "+y",
	.protect_mchar = "+a",
	.halfops_mchar = "+h",
	.type = PROTOCOL_ELEMENTAL_IRCD,
	.perm_mode = CMODE_PERM,
	.oimmune_mode = CMODE_IMMUNE,
	.ban_like_modes = "beIq",
	.except_mchar = 'e',
	.invex_mchar = 'I',
	.flags = IRCD_CIDR_BANS | IRCD_HOLDNICK,
};

static const struct cmode elemental_mode_list[] = {
  { 'i', CMODE_INVITE },
  { 'm', CMODE_MOD    },
  { 'n', CMODE_NOEXT  },
  { 'p', CMODE_PRIV   },
  { 's', CMODE_SEC    },
  { 't', CMODE_TOPIC  },
  { 'c', CMODE_NOCOLOR},
  { 'r', CMODE_REGONLY},
  { 'z', CMODE_OPMOD  },
  { 'g', CMODE_FINVITE},
  { 'L', CMODE_EXLIMIT},
  { 'P', CMODE_PERM   },
  { 'F', CMODE_FTARGET},
  { 'Q', CMODE_DISFWD },
  { 'M', CMODE_IMMUNE },
  { 'C', CMODE_NOCTCP },
  { 'A', CMODE_ADMINONLY },
  { 'O', CMODE_OPERONLY },
  { 'S', CMODE_SSLONLY },
  { 'D', CMODE_NOACTIONS },
  { 'T', CMODE_NONOTICE },
  { 'G', CMODE_NOCAPS },
  { 'E', CMODE_NOKICKS },
  { 'd', CMODE_NONICKS },
  { 'K', CMODE_NOREPEAT },
  { 'J', CMODE_KICKNOREJOIN },
  { '\0', 0 }
};

static const struct cmode elemental_status_mode_list[] = {
  { 'y', CSTATUS_OWNER },
  { 'a', CSTATUS_PROTECT },
  { 'o', CSTATUS_OP    },
  { 'h', CSTATUS_HALFOP },
  { 'v', CSTATUS_VOICE },
  { '\0', 0 }
};

static const struct cmode elemental_prefix_mode_list[] = {
  { '~', CSTATUS_OWNER },
  { '!', CSTATUS_PROTECT },
  { '@', CSTATUS_OP    },
  { '%', CSTATUS_HALFOP },
  { '+', CSTATUS_VOICE },
  { '\0', 0 }
};

static const struct cmode elemental_user_mode_list[] = {
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
	MODULE_TRY_REQUEST_DEPENDENCY(m, "protocol/charybdis")

	mode_list = elemental_mode_list;
	user_mode_list = elemental_user_mode_list;
	status_mode_list = elemental_status_mode_list;
	prefix_mode_list = elemental_prefix_mode_list;

	ircd = &elemental_ircd;
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{

}

SIMPLE_DECLARE_MODULE_V1("protocol/elemental-ircd", MODULE_UNLOAD_CAPABILITY_NEVER)
