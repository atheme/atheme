/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005 William Pitcock, et al.
 *
 * This file contains code for the nickserv DROP function.
 */

#include <atheme.h>

static void
cmd_ns_drop_func(struct sourceinfo *const restrict si, const int ATHEME_VATTR_UNUSED parc, char *parv[])
{
	const char *const acc = parv[0];
	const char *const key = parv[1];

	if (! acc)
	{
		(void) command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "DROP");
		(void) command_fail(si, fault_needmoreparams, _("Syntax: DROP <account>"));
		return;
	}

	struct myuser *mu;

	if (! (mu = myuser_find(acc)))
	{
		if (! nicksvs.no_nick_ownership)
		{
			struct mynick *const mn = mynick_find(acc);

			if (mn && command_find(si->service->commands, "UNGROUP"))
			{
				(void) command_fail(si, fault_nosuch_target, _("\2%s\2 is a grouped nick, use %s "
				                                               "to remove it."), acc, "UNGROUP");
				return;
			}
		}

		(void) command_fail(si, fault_nosuch_target, STR_IS_NOT_REGISTERED, acc);
		return;
	}

	if (si->smu != mu)
	{
		(void) command_fail(si, fault_noprivs, _("You are not logged in as \2%s\2."), acc);
		return;
	}

	return_if_fail(! metadata_find(mu, "private:freeze:freezer"));

	if (! nicksvs.no_nick_ownership && MOWGLI_LIST_LENGTH(&mu->nicks) > 1 &&
	    command_find(si->service->commands, "UNGROUP"))
	{
		(void) command_fail(si, fault_noprivs, _("Account \2%s\2 has \2%zu\2 other nick(s) grouped to it, "
		                                         "remove those first."), entity(mu)->name,
		                                         MOWGLI_LIST_LENGTH(&mu->nicks) - 1);
		return;
	}

	if (is_soper(mu))
	{
		(void) command_fail(si, fault_noprivs, _("The nickname \2%s\2 belongs to a services operator; "
		                                         "it cannot be dropped."), acc);
		return;
	}

	if (mu->flags & MU_HOLD)
	{
		(void) command_fail(si, fault_noprivs, _("The account \2%s\2 is held; it cannot be dropped."), acc);
		return;
	}

	const char *const challenge = create_weak_challenge(si, entity(mu)->name);

	if (! challenge)
	{
		(void) command_fail(si, fault_internalerror, _("Failed to create challenge"));
		return;
	}

	if (! key)
	{
		(void) command_success_nodata(si, _("This is a friendly reminder that you are about to \2DESTROY\2 "
		                                    "the account \2%s\2."), entity(mu)->name);

		(void) command_success_nodata(si, _("To avoid accidental use of this command, this operation has to "
		                                    "be confirmed. Please confirm by replying with \2/msg %s DROP %s "
		                                    "%s\2"), nicksvs.me->disp, entity(mu)->name, challenge);
		return;
	}

	if (strcmp(challenge, key) != 0)
	{
		(void) command_fail(si, fault_badparams, _("Invalid key for \2%s\2."), "DROP");
		return;
	}

	(void) command_add_flood(si, FLOOD_MODERATE);
	(void) logcommand(si, CMDLOG_REGISTER, "DROP: \2%s\2", entity(mu)->name);

	(void) hook_call_user_drop(mu);

	if (! nicksvs.no_nick_ownership)
		(void) holdnick_sts(si->service->me, 0, entity(mu)->name, NULL);

	(void) command_success_nodata(si, _("The account \2%s\2 has been dropped."), entity(mu)->name);
	(void) atheme_object_dispose(mu);
}

static void
cmd_ns_fdrop_func(struct sourceinfo *const restrict si, const int ATHEME_VATTR_UNUSED parc, char *parv[])
{
	const char *const acc = parv[0];

	if (! acc)
	{
		(void) command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "FDROP");
		(void) command_fail(si, fault_needmoreparams, _("Syntax: FDROP <account>"));
		return;
	}

	struct myuser *mu;

	if (! (mu = myuser_find(acc)))
	{
		if (!nicksvs.no_nick_ownership)
		{
			struct mynick *const mn = mynick_find(acc);

			if (mn != NULL && command_find(si->service->commands, "FUNGROUP"))
			{
				(void) command_fail(si, fault_nosuch_target, _("\2%s\2 is a grouped nick, use %s "
				                                               "to remove it."), acc, "FUNGROUP");
				return;
			}
		}

		(void) command_fail(si, fault_nosuch_target, STR_IS_NOT_REGISTERED, acc);
		return;
	}

	if (is_soper(mu))
	{
		(void) command_fail(si, fault_noprivs, _("The nickname \2%s\2 belongs to a services operator; "
		                                         "it cannot be dropped."), acc);
		return;
	}

	if (mu->flags & MU_HOLD)
	{
		(void) command_fail(si, fault_noprivs, _("The account \2%s\2 is held; it cannot be dropped."), acc);
		return;
	}

	(void) wallops("\2%s\2 dropped the account \2%s\2", get_oper_name(si), entity(mu)->name);
	(void) logcommand(si, CMDLOG_ADMIN | LG_REGISTER, "FDROP: \2%s\2", entity(mu)->name);

	(void) hook_call_user_drop(mu);

	if (! nicksvs.no_nick_ownership)
	{
		mowgli_node_t *n;

		MOWGLI_ITER_FOREACH(n, mu->nicks.head)
		{
			struct mynick *const mn = n->data;

			(void) holdnick_sts(si->service->me, 0, mn->nick, NULL);
		}
	}

	(void) command_success_nodata(si, _("The account \2%s\2 has been dropped."), entity(mu)->name);
	(void) atheme_object_dispose(mu);
}

static struct command cmd_ns_drop = {
	.name           = "DROP",
	.desc           = N_("Drops an account registration."),
	.access         = AC_AUTHENTICATED,
	.maxparc        = 2,
	.cmd            = &cmd_ns_drop_func,
	.help           = { .path = "nickserv/drop" },
};

static struct command cmd_ns_fdrop = {
	.name           = "FDROP",
	.desc           = N_("Forces dropping an account registration."),
	.access         = PRIV_USER_ADMIN,
	.maxparc        = 1,
	.cmd            = &cmd_ns_fdrop_func,
	.help           = { .path = "nickserv/fdrop" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "nickserv/main")

	(void) service_named_bind_command("nickserv", &cmd_ns_drop);
	(void) service_named_bind_command("nickserv", &cmd_ns_fdrop);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	(void) service_named_unbind_command("nickserv", &cmd_ns_drop);
	(void) service_named_unbind_command("nickserv", &cmd_ns_fdrop);
}

SIMPLE_DECLARE_MODULE_V1("nickserv/drop", MODULE_UNLOAD_CAPABILITY_OK)
