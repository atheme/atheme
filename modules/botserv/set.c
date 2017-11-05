/*
 * Copyright (c) 2009 Celestin, et al.
 * Copyright (c) 2014-2016 Austin Ellis
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file is a meta-module for compatibility with old
 * setups pre-SET split.
 */

#include "atheme.h"
#include "botserv.h"

DECLARE_MODULE_V1("botserv/set", MODULE_UNLOAD_CAPABILITY_OK,
                  _modinit, _moddeinit,
                  PACKAGE_STRING, VENDOR_STRING);

void _modinit(module_t *m)
{
	/* Some MODULE_TRY_REQUEST_DEPENDENCY gubbins */
	MODULE_TRY_REQUEST_DEPENDENCY(m, "botserv/set_core");
	MODULE_TRY_REQUEST_DEPENDENCY(m, "botserv/set_fantasy");
	MODULE_TRY_REQUEST_DEPENDENCY(m, "botserv/set_nobot");
	MODULE_TRY_REQUEST_DEPENDENCY(m, "botserv/set_private");
	MODULE_TRY_REQUEST_DEPENDENCY(m, "botserv/set_saycaller");

}

void _moddeinit(module_unload_intent_t intent)
{
}
