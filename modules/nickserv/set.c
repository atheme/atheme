/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2006 William Pitcock, et al.
 *
 * This file is a meta-module for compatibility with old
 * setups pre-SET split.
 */

#include <atheme.h>

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "nickserv/set_core")
	MODULE_TRY_REQUEST_DEPENDENCY(m, "nickserv/set_email")
	MODULE_TRY_REQUEST_DEPENDENCY(m, "nickserv/set_emailmemos")
	MODULE_TRY_REQUEST_DEPENDENCY(m, "nickserv/set_hidelastlogin")
	MODULE_TRY_REQUEST_DEPENDENCY(m, "nickserv/set_hidemail")

#ifdef ENABLE_NLS
	MODULE_TRY_REQUEST_DEPENDENCY(m, "nickserv/set_language")
#endif

	MODULE_TRY_REQUEST_DEPENDENCY(m, "nickserv/set_neverop")
	MODULE_TRY_REQUEST_DEPENDENCY(m, "nickserv/set_nomemo")
	MODULE_TRY_REQUEST_DEPENDENCY(m, "nickserv/set_noop")
	MODULE_TRY_REQUEST_DEPENDENCY(m, "nickserv/set_password")
	MODULE_TRY_REQUEST_DEPENDENCY(m, "nickserv/set_property")
	MODULE_TRY_REQUEST_DEPENDENCY(m, "nickserv/set_quietchg")
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{

}

SIMPLE_DECLARE_MODULE_V1("nickserv/set", MODULE_UNLOAD_CAPABILITY_OK)
