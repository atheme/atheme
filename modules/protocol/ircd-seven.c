/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Copyright (c) 2005-2007 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains protocol support for charybdis-based ircd.
 *
 * $Id: charybdis.c 8223 2007-05-05 12:58:06Z jilles $
 */

#include "atheme.h"
#include "uplink.h"
#include "pmodule.h"
#include "protocol/charybdis.h"

DECLARE_MODULE_V1("protocol/ircd-seven", TRUE, _modinit, NULL, "$Id: charybdis.c 8223 2007-05-05 12:58:06Z jilles $", "Atheme Development Group <http://www.atheme.org>");

/* *INDENT-OFF* */

struct cmode_ seven_user_mode_list[] = {
  { 'm', UF_IMMUNE   },
  { 'a', UF_ADMIN    },
  { 'i', UF_INVIS    },
  { 'o', UF_IRCOP    },
  { '\0', 0 }
};

/* *INDENT-ON* */

void _modinit(module_t * m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "protocol/charybdis");

	user_mode_list = seven_user_mode_list;

	m->mflags = MODTYPE_CORE;

	pmodule_loaded = TRUE;
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
