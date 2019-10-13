/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2006 Patrick Fish, et al.
 *
 * This file contains code for OS UPTIME
 */

#include <atheme.h>

static void
os_cmd_uptime_func(struct sourceinfo *const restrict si, const int ATHEME_VATTR_UNUSED parc,
                   char **const restrict parv)
{
	(void) logcommand(si, CMDLOG_GET, "UPTIME");

	(void) command_success_nodata(si, _("%s [%s] Build Date: %s"), PACKAGE_STRING, revision, get_build_date());

	(void) command_success_nodata(si, _("Services have been up for %s"), timediff(CURRTIME - me.start));
	(void) command_success_nodata(si, _("Current PID: %lu"), (unsigned long) getpid());
	(void) command_success_nodata(si, _("Registered accounts: %u"), cnt.myuser);

	if (! nicksvs.no_nick_ownership)
		(void) command_success_nodata(si, _("Registered nicknames: %u"), cnt.mynick);

	(void) command_success_nodata(si, _("Registered channels: %u"), cnt.mychan);
	(void) command_success_nodata(si, _("Users currently online: %u"), (cnt.user - me.me->users));
}

static struct command os_cmd_uptime = {
	.name           = "UPTIME",
	.desc           = N_("Shows services uptime and the number of registered nicks and channels."),
	.access         = PRIV_SERVER_AUSPEX,
	.maxparc        = 1,
	.cmd            = &os_cmd_uptime_func,
	.help           = { .path = "oservice/uptime" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "operserv/main")

	(void) service_named_bind_command("operserv", &os_cmd_uptime);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	(void) service_named_unbind_command("operserv", &os_cmd_uptime);
}

SIMPLE_DECLARE_MODULE_V1("operserv/uptime", MODULE_UNLOAD_CAPABILITY_OK)
