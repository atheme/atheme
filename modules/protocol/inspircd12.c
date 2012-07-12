/*
 * Copyright (c) 2012 William Pitcock <nenolod@dereferenced.org>
 * Rights to this code are as defined in doc/LICENSE.
 *
 * Stub module which loads protocol/inspircd.
 */

#include "atheme.h"

DECLARE_MODULE_V1("protocol/inspircd12", true, _modinit, NULL, PACKAGE_STRING, "Atheme Development Group <http://www.atheme.org>");

void _modinit(module_t * m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "protocol/inspircd");

	slog(LG_INFO, "protocol/inspircd12 has been deprecated by protocol/inspircd, which is better maintained.  Please update your configuration.");
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
