/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2007 Patrick Fish, et al.
 *
 * Gives services the ability to freeze nicknames
 */

#include <atheme.h>
#include "list_common.h"
#include "list.h"

static bool
is_frozen(const struct mynick *mn, const void *arg)
{
	struct myuser *mu = mn->owner;

	return !!metadata_find(mu, "private:freeze:freezer");
}

static bool
frozen_match(const struct mynick *mn, const void *arg)
{
	const char *frozenpattern = (const char*)arg;
	struct metadata *mdfrozen;

	struct myuser *mu = mn->owner;
	mdfrozen = metadata_find(mu, "private:freeze:reason");

	if (mdfrozen != NULL && !match(frozenpattern, mdfrozen->value))
		return true;

	return false;
}

// FREEZE ON|OFF -- don't pollute the root with THAW
static void
ns_cmd_freeze(struct sourceinfo *si, int parc, char *parv[])
{
	struct myuser *mu;
	const char *target = parv[0];
	const char *action = parv[1];
	const char *reason = parv[2];
	struct user *u;
	mowgli_node_t *n, *tn;

	if (!target || !action)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "FREEZE");
		command_fail(si, fault_needmoreparams, _("Usage: FREEZE <account> <ON|OFF> [reason]"));
		return;
	}

	mu = myuser_find_ext(target);

	if (!mu)
	{
		command_fail(si, fault_nosuch_target, STR_IS_NOT_REGISTERED, target);
		return;
	}

	target = entity(mu)->name;

	if (!strcasecmp(action, "ON"))
	{
		if (!reason)
		{
			command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "FREEZE");
			command_fail(si, fault_needmoreparams, _("Usage: FREEZE <nickname> ON <reason>"));
			return;
		}

		if (is_soper(mu))
		{
			command_fail(si, fault_badparams, _("The account \2%s\2 belongs to a services operator; it cannot be frozen."), target);
			return;
		}

		if (metadata_find(mu, "private:freeze:freezer"))
		{
			command_fail(si, fault_badparams, _("\2%s\2 is already frozen."), target);
			return;
		}

		metadata_add(mu, "private:freeze:freezer", get_oper_name(si));
		metadata_add(mu, "private:freeze:reason", reason);
		metadata_add(mu, "private:freeze:timestamp", number_to_string(CURRTIME));

		// log them out
		MOWGLI_ITER_FOREACH_SAFE(n, tn, mu->logins.head)
		{
			u = (struct user *)n->data;
			if (!ircd_logout_or_kill(u, target))
			{
				u->myuser = NULL;
				mowgli_node_delete(n, &mu->logins);
				mowgli_node_free(n);
			}
		}
		mu->flags |= MU_NOBURSTLOGIN;
		authcookie_destroy_all(mu);

		wallops("\2%s\2 froze the account \2%s\2 (%s).", get_oper_name(si), target, reason);
		logcommand(si, CMDLOG_ADMIN, "FREEZE:ON: \2%s\2 (reason: \2%s\2)", target, reason);
		command_success_nodata(si, _("\2%s\2 is now frozen."), target);
	}
	else if (!strcasecmp(action, "OFF"))
	{
		if (!metadata_find(mu, "private:freeze:freezer"))
		{
			command_fail(si, fault_badparams, _("\2%s\2 is not frozen."), target);
			return;
		}

		metadata_delete(mu, "private:freeze:freezer");
		metadata_delete(mu, "private:freeze:reason");
		metadata_delete(mu, "private:freeze:timestamp");

		wallops("\2%s\2 thawed the account \2%s\2.", get_oper_name(si), target);
		logcommand(si, CMDLOG_ADMIN, "FREEZE:OFF: \2%s\2", target);
		command_success_nodata(si, _("\2%s\2 has been thawed"), target);
	}
	else
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "FREEZE");
		command_fail(si, fault_needmoreparams, _("Usage: FREEZE <account> <ON|OFF> [reason]"));
	}
}

static struct command ns_freeze = {
	.name           = "FREEZE",
	.desc           = N_("Freezes an account."),
	.access         = PRIV_USER_ADMIN,
	.maxparc        = 3,
	.cmd            = &ns_cmd_freeze,
	.help           = { .path = "nickserv/freeze" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "nickserv/main")

	use_nslist_main_symbols(m);

	service_named_bind_command("nickserv", &ns_freeze);

	static struct list_param frozen;
	frozen.opttype = OPT_BOOL;
	frozen.is_match = is_frozen;

	static struct list_param frozen_reason;
	frozen_reason.opttype = OPT_STRING;
	frozen_reason.is_match = frozen_match;

	list_register("frozen", &frozen);
	list_register("frozen-reason", &frozen_reason);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("nickserv", &ns_freeze);

	list_unregister("frozen");
	list_unregister("frozen-reason");
}

SIMPLE_DECLARE_MODULE_V1("nickserv/freeze", MODULE_UNLOAD_CAPABILITY_OK)
