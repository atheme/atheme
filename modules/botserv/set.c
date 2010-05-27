/*
 * Copyright (c) 2009 Celestin, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains a BService SET which can change
 * botserv settings on channel or bot.
 *
 * $Id: set.c 7969 2009-04-09 19:19:38Z celestin $
 */

#include "atheme.h"
#include "botserv.h"

DECLARE_MODULE_V1
(
	"botserv/set", false, _modinit, _moddeinit,
	"$Id: set.c 7969 2009-04-09 19:19:38Z celestin $",
	"Rizon Development Group <http://dev.rizon.net>"
);

void _modinit(module_t *m)
{
	/* Some MODULE_TRY_REQUEST_DEPENDENCY gubbins */
	MODULE_TRY_REQUEST_DEPENDENCY(m, "botserv/set_core");
	MODULE_TRY_REQUEST_DEPENDENCY(m, "botserv/set_fantasy");
	MODULE_TRY_REQUEST_DEPENDENCY(m, "botserv/set_nobot");
	MODULE_TRY_REQUEST_DEPENDENCY(m, "botserv/set_private");

}

void _moddeinit()
{
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
