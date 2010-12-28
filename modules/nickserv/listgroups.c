/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the nickserv LISTGROUPS function.
 *
 */

#include "atheme.h"

#include "../groupserv/groupserv.h"

DECLARE_MODULE_V1
(
	"nickserv/listgroups", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void ns_cmd_listgroups(sourceinfo_t *si, int parc, char *parv[]);

command_t ns_listgroups = { "LISTGROUPS", N_("Lists groups that you have access to."), AC_NONE, 1, ns_cmd_listgroups, { .path = "nickserv/listgroups" } };

static void ns_cmd_listgroups(sourceinfo_t *si, int parc, char *parv[])
{
	myuser_t *mu;
	mowgli_node_t *n;
	mowgli_list_t *l;
	unsigned int bans = 0, i;

	/* Optional target */
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
			command_fail(si, fault_nosuch_target, _("\2%s\2 is not registered."), target);
			return;
		}
	}
	else
	{
		mu = si->smu;
		if (mu == NULL)
		{
			command_fail(si, fault_noprivs, _("You are not logged in."));
			return;
		}
	}

	if (mu != si->smu)
	{	/* must have been an oper */
		logcommand(si, CMDLOG_ADMIN, "LISTGROUPS: \2%s\2", entity(mu)->name);
	}
	else
	{	/* just a user, or oper is listing himself */
		logcommand(si, CMDLOG_GET, "LISTGROUPS");
	}

	l = myuser_get_membership_list(mu);

	if (MOWGLI_LIST_LENGTH(l) == 0)
	{
		command_success_nodata(si, _("No group access was found for the nickname \2%s\2."), entity(mu)->name);
		return;
	}

	MOWGLI_ITER_FOREACH(n, l->head)
	{
		groupacs_t *ga = n->data;

		if (ga->mu != mu)
			continue;

		if (groupacs_find(ga->mg, mu, GA_BAN) != NULL)
		{
			bans++;
			continue;
		}

		command_success_nodata(si, _("Access flag(s) %s in %s"), gflags_tostr(ga_flags, ga->flags), entity(ga->mg)->name);
	}

	i = MOWGLI_LIST_LENGTH(l) - bans;

	if (i == 0)
		command_success_nodata(si, _("No group access was found for the nickname \2%s\2."), entity(mu)->name);
	else
		command_success_nodata(si, ngettext(N_("\2%d\2 group access match for the nickname \2%s\2"),
						    N_("\2%d\2 group access matches for the nickname \2%s\2"), i),
						    i, entity(mu)->name);
}

void _modinit(module_t *m)
{
	use_groupserv_main_symbols(m);

	service_named_bind_command("nickserv", &ns_listgroups);
}

void _moddeinit(module_unload_intent_t intent)
{
	service_named_unbind_command("nickserv", &ns_listgroups);
}


