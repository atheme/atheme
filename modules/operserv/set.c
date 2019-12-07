/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2019 Atheme Development Group (https://atheme.github.io/)
 */

#include <atheme.h>

static void
mod_init(struct module *const restrict m)
{
	(void) slog(LG_INFO, "%s: this module is deprecated, & only for configuration compatibility", m->name);

	MODULE_TRY_REQUEST_DEPENDENCY(m, "operserv/set_akicktime")
	MODULE_TRY_REQUEST_DEPENDENCY(m, "operserv/set_chanexpire")
	MODULE_TRY_REQUEST_DEPENDENCY(m, "operserv/set_commitinterval")
	MODULE_TRY_REQUEST_DEPENDENCY(m, "operserv/set_enforceprefix")
	MODULE_TRY_REQUEST_DEPENDENCY(m, "operserv/set_klinetime")
	MODULE_TRY_REQUEST_DEPENDENCY(m, "operserv/set_maxchanacs")
	MODULE_TRY_REQUEST_DEPENDENCY(m, "operserv/set_maxchans")
	MODULE_TRY_REQUEST_DEPENDENCY(m, "operserv/set_maxfounders")
	MODULE_TRY_REQUEST_DEPENDENCY(m, "operserv/set_maxlogins")
	MODULE_TRY_REQUEST_DEPENDENCY(m, "operserv/set_maxnicks")
	MODULE_TRY_REQUEST_DEPENDENCY(m, "operserv/set_maxusers")
	MODULE_TRY_REQUEST_DEPENDENCY(m, "operserv/set_mdlimit")
	MODULE_TRY_REQUEST_DEPENDENCY(m, "operserv/set_nickexpire")
	MODULE_TRY_REQUEST_DEPENDENCY(m, "operserv/set_recontime")
	MODULE_TRY_REQUEST_DEPENDENCY(m, "operserv/set_spam")
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{

}

SIMPLE_DECLARE_MODULE_V1("operserv/set", MODULE_UNLOAD_CAPABILITY_OK)
