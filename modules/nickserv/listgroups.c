/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005 Atheme Project (http://atheme.org/)
 *
 * This file contains code for the nickserv LISTGROUPS function.
 */

#include <atheme.h>
#include "../groupserv/groupserv.h"

static void
ns_cmd_listgroups(struct sourceinfo *si, int parc, char *parv[])
{
	struct myuser *mu;
	mowgli_node_t *n;
	mowgli_list_t *l;
	unsigned int bans = 0, i;

	// Optional target
	char *target = parv[0];

	if (target)
	{
		if (!has_priv(si, PRIV_GROUP_AUSPEX))
		{
			command_fail(si, fault_noprivs, _("You are not authorized to use the target argument."));
			return;
		}

		mu = myuser_find_ext(target);

		if (mu == NULL)
		{
			command_fail(si, fault_nosuch_target, STR_IS_NOT_REGISTERED, target);
			return;
		}
	}
	else
	{
		mu = si->smu;
		if (mu == NULL)
		{
			command_fail(si, fault_noprivs, STR_NOT_LOGGED_IN);
			return;
		}
	}

	if (mu != si->smu)
	{	// must have been an oper
		logcommand(si, CMDLOG_ADMIN, "LISTGROUPS: \2%s\2", entity(mu)->name);
	}
	else
	{	// just a user, or oper is listing himself
		logcommand(si, CMDLOG_GET, "LISTGROUPS");
	}

	l = myentity_get_membership_list(entity(mu));

	if (MOWGLI_LIST_LENGTH(l) == 0)
	{
		command_success_nodata(si, _("No group access was found for the nickname \2%s\2."), entity(mu)->name);
		return;
	}

	MOWGLI_ITER_FOREACH(n, l->head)
	{
		struct groupacs *ga = n->data;

		if (ga->mt != entity(mu))
			continue;

		if ((ga->flags & GA_BAN))
		{
			bans++;
			continue;
		}

		command_success_nodata(si, _("Access flag(s) \2%s\2 in \2%s\2"), gflags_tostr(ga_flags, ga->flags), entity(ga->mg)->name);
	}

	i = MOWGLI_LIST_LENGTH(l) - bans;

	if (i == 0)
		command_success_nodata(si, _("No group access was found for the nickname \2%s\2."), entity(mu)->name);
	else
		command_success_nodata(si, ngettext(N_("\2%u\2 group access match for the nickname \2%s\2."),
						    N_("\2%u\2 group access matches for the nickname \2%s\2."), i),
						    i, entity(mu)->name);
}

static struct command ns_listgroups = {
	.name           = "LISTGROUPS",
	.desc           = N_("Lists groups that you have access to."),
	.access         = AC_NONE,
	.maxparc        = 1,
	.cmd            = &ns_cmd_listgroups,
	.help           = { .path = "nickserv/listgroups" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "nickserv/main")

	use_groupserv_main_symbols(m);

	service_named_bind_command("nickserv", &ns_listgroups);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("nickserv", &ns_listgroups);
}

SIMPLE_DECLARE_MODULE_V1("nickserv/listgroups", MODULE_UNLOAD_CAPABILITY_OK)
