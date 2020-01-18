/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2009 Celestin, et al.
 * Copyright (C) 2014-2016 Austin Ellis
 *
 * This file is a meta-module for compatibility with old
 * setups pre-SET split.
 */

#include <atheme.h>

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "botserv/set_core")
	MODULE_TRY_REQUEST_DEPENDENCY(m, "botserv/set_fantasy")
	MODULE_TRY_REQUEST_DEPENDENCY(m, "botserv/set_nobot")
	MODULE_TRY_REQUEST_DEPENDENCY(m, "botserv/set_private")
	MODULE_TRY_REQUEST_DEPENDENCY(m, "botserv/set_saycaller")
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{

}

SIMPLE_DECLARE_MODULE_V1("botserv/set", MODULE_UNLOAD_CAPABILITY_OK)
