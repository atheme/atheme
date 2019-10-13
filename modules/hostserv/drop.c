/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2015-2016 Atheme Development Group (https://atheme.github.io/)
 *
 * Allows a user to drop (unset) their assigned vhost without
 * Staff intervention.
 */

#include <atheme.h>
#include "hostserv.h"

static void
hs_cmd_drop(struct sourceinfo *si, int parc, char *parv[])
{
	struct mynick *mn;
	struct metadata *md;
	char buf[BUFSIZE];

	// This is only because we need a nick to copy from.
	if (si->su == NULL)
	{
		command_fail(si, fault_noprivs, STR_IRC_COMMAND_ONLY, "DROP");
		return;
	}

	mn = mynick_find(si->su->nick);
	if (mn == NULL)
	{
		command_fail(si, fault_nosuch_target, STR_IS_NOT_REGISTERED, si->su->nick);
		return;
	}
	if (mn->owner != si->smu)
	{
		command_fail(si, fault_noprivs, _("Nick \2%s\2 is not registered to your account."), mn->nick);
		return;
	}
	snprintf(buf, BUFSIZE, "%s:%s", "private:usercloak", mn->nick);
	md = metadata_find(si->smu, buf);
	if (md == NULL)
		md = metadata_find(si->smu, "private:usercloak");
	if (md == NULL)
	{
		command_success_nodata(si, _("There is not a vhost assigned to this nick."));
		return;
	}
	hs_sethost_all(si->smu, NULL, get_source_name(si));
	command_success_nodata(si, _("Dropped all vhosts for \2%s\2."), get_source_name(si));
	logcommand(si, CMDLOG_ADMIN, "VHOST:DROP: \2%s\2", get_source_name(si));
	do_sethost_all(si->smu, NULL); // restore user vhost from user host

}

static struct command hs_drop = {
	.name           = "DROP",
	.desc           = N_("Drops your assigned vhost."),
	.access         = AC_AUTHENTICATED,
	.maxparc        = 1,
	.cmd            = &hs_cmd_drop,
	.help           = { .path = "hostserv/drop" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "hostserv/main")

	service_named_bind_command("hostserv", &hs_drop);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("hostserv", &hs_drop);
}

SIMPLE_DECLARE_MODULE_V1("hostserv/drop", MODULE_UNLOAD_CAPABILITY_OK)
