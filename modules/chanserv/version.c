/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005 William Pitcock, et al.
 *
 * This file contains code for the ChanServ VERSION function.
 */

#include <atheme.h>

static void
cs_cmd_version(struct sourceinfo *si, int parc, char *parv[])
{
        char buf[BUFSIZE];
        snprintf(buf, BUFSIZE, "%s [%s]", PACKAGE_STRING, SERNO);
        command_success_string(si, buf, "%s", buf);
        return;
}

static struct command cs_version = {
	.name           = "VERSION",
	.desc           = N_("Displays version information of the services."),
	.access         = AC_NONE,
	.maxparc        = 0,
	.cmd            = &cs_cmd_version,
	.help           = { .path = "" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "chanserv/main")

        service_named_bind_command("chanserv", &cs_version);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("chanserv", &cs_version);
}

SIMPLE_DECLARE_MODULE_V1("chanserv/version", MODULE_UNLOAD_CAPABILITY_OK)
