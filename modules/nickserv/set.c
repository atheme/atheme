/*
 * Copyright (c) 2006 William Pitcock, et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file is a meta-module for compatibility with old
 * setups pre-SET split.
 *
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"nickserv/set", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

void _modinit(module_t *m)
{
	/* MODULE_TRY_REQUEST_DEPENDENCY stuff so this acts like a meta-
	 * module like chanserv/set.c .
	 */
	MODULE_TRY_REQUEST_DEPENDENCY(m, "nickserv/set_core");
	MODULE_TRY_REQUEST_DEPENDENCY(m, "nickserv/set_email");
	MODULE_TRY_REQUEST_DEPENDENCY(m, "nickserv/set_emailmemos");
	MODULE_TRY_REQUEST_DEPENDENCY(m, "nickserv/set_hidemail");

#ifdef ENABLE_NLS
	MODULE_TRY_REQUEST_DEPENDENCY(m, "nickserv/set_language");
#endif

	MODULE_TRY_REQUEST_DEPENDENCY(m, "nickserv/set_neverop");
	MODULE_TRY_REQUEST_DEPENDENCY(m, "nickserv/set_nomemo");
	MODULE_TRY_REQUEST_DEPENDENCY(m, "nickserv/set_noop");
	MODULE_TRY_REQUEST_DEPENDENCY(m, "nickserv/set_password");
	MODULE_TRY_REQUEST_DEPENDENCY(m, "nickserv/set_property");
	MODULE_TRY_REQUEST_DEPENDENCY(m, "nickserv/set_quietchg");
}

void _moddeinit()
{
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
