/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file is a meta-module for compatibility with 
 * old setups pre-SET split.
 *
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/set", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

void _modinit(module_t *m)
{
	/* Here's all the MODULE_TRY_REQUEST_DEPENDENCY out the ass
	 * stuff to make easy upgrades.
	 */
	MODULE_TRY_REQUEST_DEPENDENCY(m, "chanserv/set_core");
	MODULE_TRY_REQUEST_DEPENDENCY(m, "chanserv/set_email");
	MODULE_TRY_REQUEST_DEPENDENCY(m, "chanserv/set_entrymsg");
	MODULE_TRY_REQUEST_DEPENDENCY(m, "chanserv/set_fantasy");
	MODULE_TRY_REQUEST_DEPENDENCY(m, "chanserv/set_founder");
	MODULE_TRY_REQUEST_DEPENDENCY(m, "chanserv/set_guard");
	MODULE_TRY_REQUEST_DEPENDENCY(m, "chanserv/set_keeptopic");
	MODULE_TRY_REQUEST_DEPENDENCY(m, "chanserv/set_mlock");
	MODULE_TRY_REQUEST_DEPENDENCY(m, "chanserv/set_property");
	MODULE_TRY_REQUEST_DEPENDENCY(m, "chanserv/set_restricted");
	MODULE_TRY_REQUEST_DEPENDENCY(m, "chanserv/set_secure");
	MODULE_TRY_REQUEST_DEPENDENCY(m, "chanserv/set_topiclock");
	MODULE_TRY_REQUEST_DEPENDENCY(m, "chanserv/set_url");
	MODULE_TRY_REQUEST_DEPENDENCY(m, "chanserv/set_verbose");
}

void _moddeinit()
{
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
