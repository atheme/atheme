/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (c) 2005-2007 William Pitcock, et al.
 *
 * This file contains code for the NickServ LISTLOGINS function.
 */

#include <atheme.h>

static void
ns_cmd_listlogins_func(struct sourceinfo *const restrict si, const int ATHEME_VATTR_UNUSED parc,
                       char ATHEME_VATTR_UNUSED **const restrict parv)
{
	if (si->smu->flags & MU_WAITAUTH)
	{
		(void) command_fail(si, fault_notverified, STR_EMAIL_NOT_VERIFIED);
		return;
	}

	(void) command_success_nodata(si, _("Clients identified to account \2%s\2:"), entity(si->smu)->name);

	unsigned int matches = 0;
	mowgli_node_t *n;

	MOWGLI_ITER_FOREACH(n, si->smu->logins.head)
	{
		const struct user *const u = n->data;

		(void) command_success_nodata(si, _("- %s!%s@%s (real host: %s)"), u->nick, u->user, u->vhost, u->host);

		matches++;
	}

	(void) command_success_nodata(si, ngettext(N_("\2%u\2 client found."), N_("\2%u\2 clients found."),
	                                  matches), matches);

	(void) logcommand(si, CMDLOG_GET, "LISTLOGINS: (\2%u\2 matches)", matches);
}

static struct command ns_cmd_listlogins = {
	.name           = "LISTLOGINS",
	.desc           = N_("Lists details of clients authenticated as you."),
	.access         = AC_AUTHENTICATED,
	.maxparc        = 1,
	.cmd            = &ns_cmd_listlogins_func,
	.help           = { .path = "nickserv/listlogins" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_CONFLICT(m, "contrib/ns_listlogins")
	MODULE_TRY_REQUEST_DEPENDENCY(m, "nickserv/main")

	(void) service_named_bind_command("nickserv", &ns_cmd_listlogins);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	(void) service_named_unbind_command("nickserv", &ns_cmd_listlogins);
}

SIMPLE_DECLARE_MODULE_V1("nickserv/listlogins", MODULE_UNLOAD_CAPABILITY_OK)
