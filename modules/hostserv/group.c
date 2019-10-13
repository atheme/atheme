/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2009 William Pitcock <nenolod -at- nenolod.net>
 *
 * Allows syncing the vhost for all nicks in a group.
 */

#include <atheme.h>
#include "hostserv.h"

static void
hs_cmd_group(struct sourceinfo *si, int parc, char *parv[])
{
	struct mynick *mn;
	struct metadata *md;
	char buf[BUFSIZE];

	// This is only because we need a nick to copy from.
	if (si->su == NULL)
	{
		command_fail(si, fault_noprivs, STR_IRC_COMMAND_ONLY, "GROUP");
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
		command_success_nodata(si, _("Please contact an Operator to get a vhost assigned to this nick."));
		return;
	}
	mowgli_strlcpy(buf, md->value, sizeof buf);
	hs_sethost_all(si->smu, buf, get_source_name(si));
	do_sethost_all(si->smu, buf);
	command_success_nodata(si, _("All vhosts in the group \2%s\2 have been set to \2%s\2."), entity(si->smu)->name, buf);
}

static struct command hs_group = {
	.name           = "GROUP",
	.desc           = N_("Syncs the vhost for all nicks in a group."),
	.access         = AC_AUTHENTICATED,
	.maxparc        = 1,
	.cmd            = &hs_cmd_group,
	.help           = { .path = "hostserv/group" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "hostserv/main")

	service_named_bind_command("hostserv", &hs_group);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("hostserv", &hs_group);
}

SIMPLE_DECLARE_MODULE_V1("hostserv/group", MODULE_UNLOAD_CAPABILITY_OK)
