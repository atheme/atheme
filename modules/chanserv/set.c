/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2003-2004 E. Will, et al.
 * Copyright (C) 2005 Atheme Project (http://atheme.org/)
 *
 * This file is a meta-module for compatibility with
 * old setups pre-SET split.
 */

#include <atheme.h>

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "chanserv/set_core")
	MODULE_TRY_REQUEST_DEPENDENCY(m, "chanserv/set_email")
	MODULE_TRY_REQUEST_DEPENDENCY(m, "chanserv/set_entrymsg")
	MODULE_TRY_REQUEST_DEPENDENCY(m, "chanserv/set_fantasy")
	MODULE_TRY_REQUEST_DEPENDENCY(m, "chanserv/set_guard")
	MODULE_TRY_REQUEST_DEPENDENCY(m, "chanserv/set_keeptopic")
	MODULE_TRY_REQUEST_DEPENDENCY(m, "chanserv/set_mlock")
	MODULE_TRY_REQUEST_DEPENDENCY(m, "chanserv/set_property")
	MODULE_TRY_REQUEST_DEPENDENCY(m, "chanserv/set_restricted")
	MODULE_TRY_REQUEST_DEPENDENCY(m, "chanserv/set_secure")
	MODULE_TRY_REQUEST_DEPENDENCY(m, "chanserv/set_topiclock")
	MODULE_TRY_REQUEST_DEPENDENCY(m, "chanserv/set_url")
	MODULE_TRY_REQUEST_DEPENDENCY(m, "chanserv/set_verbose")
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{

}

SIMPLE_DECLARE_MODULE_V1("chanserv/set", MODULE_UNLOAD_CAPABILITY_OK)
