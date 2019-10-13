/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005 William Pitcock, et al.
 *
 * This file contains code for the CService STATUS function.
 */

#include <atheme.h>

static void
ns_cmd_acc(struct sourceinfo *si, int parc, char *parv[])
{
	const char *targuser = parv[0];
	const char *targaccount = parv[1];
	struct user *u;
	struct myuser *mu;
	struct mynick *mn;
	bool show_id = config_options.show_entity_id || has_priv(si, PRIV_USER_AUSPEX);

	if (!targuser)
	{
		u = si->su;
		targuser = u != NULL ? u->nick : "?";
	}
	else
		u = user_find_named(targuser);

	if (!u)
	{
		command_fail(si, fault_nosuch_target, _("%s%s%s ACC 0 (offline)"), targuser, parc >= 2 ? " -> " : "", parc >= 2 ? targaccount : "");
		return;
	}

	if (!targaccount)
		targaccount = u->nick;
	if (!strcmp(targaccount, "*"))
		mu = u->myuser;
	else
		mu = myuser_find_ext(targaccount);

	if (!mu)
	{
		command_fail(si, fault_nosuch_target, _("%s%s%s ACC 0 (not registered)"), u->nick, parc >= 2 ? " -> " : "", parc >= 2 ? targaccount : "");
		return;
	}

	if (u->myuser == mu)
		command_success_nodata(si, "%s%s%s ACC 3 %s",
			u->nick,
			parc >= 2 ? " -> " : "",
			parc >= 2 ? entity(mu)->name : "",
			show_id ? entity(mu)->id : "");
	else if ((mn = mynick_find(u->nick)) != NULL && mn->owner == mu &&
			myuser_access_verify(u, mu))
		command_success_nodata(si, "%s%s%s ACC 2 %s",
			u->nick,
			parc >= 2 ? " -> " : "",
			parc >= 2 ? entity(mu)->name : "",
			show_id ? entity(mu)->id : "");
	else
		command_success_nodata(si, "%s%s%s ACC 1 %s",
			u->nick,
			parc >= 2 ? " -> " : "",
			parc >= 2 ? entity(mu)->name : "",
			show_id ? entity(mu)->id : "");
}

static void
ns_cmd_status(struct sourceinfo *si, int parc, char *parv[])
{
	logcommand(si, CMDLOG_GET, "STATUS");

	if (!si->smu)
		command_success_nodata(si, STR_NOT_LOGGED_IN);
	else
	{
		command_success_nodata(si, _("You are logged in as \2%s\2."), entity(si->smu)->name);

		if (is_soper(si->smu))
		{
			struct soper *soper = si->smu->soper;

			command_success_nodata(si, _("You are a services operator of class %s."), soper->operclass ? soper->operclass->name : soper->classname);
		}
	}

	if (si->su != NULL)
	{
		struct mynick *mn;

		mn = mynick_find(si->su->nick);
		if (mn != NULL && mn->owner != si->smu &&
				myuser_access_verify(si->su, mn->owner))
			command_success_nodata(si, _("You are recognized as \2%s\2."), entity(mn->owner)->name);
	}

	if (si->su != NULL && is_admin(si->su))
		command_success_nodata(si, _("You are a server administrator."));

	if (si->su != NULL && is_ircop(si->su))
		command_success_nodata(si, _("You are an IRC operator."));
}

static struct command ns_status = {
	.name           = "STATUS",
	.desc           = N_("Displays session information."),
	.access         = AC_NONE,
	.maxparc        = 0,
	.cmd            = &ns_cmd_status,
	.help           = { .path = "nickserv/status" },
};

static struct command ns_acc = {
	.name           = "ACC",
	.desc           = N_("Displays parsable session information."),
	.access         = AC_NONE,
	.maxparc        = 2,
	.cmd            = &ns_cmd_acc,
	.help           = { .path = "nickserv/acc" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "nickserv/main")

	service_named_bind_command("nickserv", &ns_acc);
	service_named_bind_command("nickserv", &ns_status);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("nickserv", &ns_acc);
	service_named_unbind_command("nickserv", &ns_status);
}

SIMPLE_DECLARE_MODULE_V1("nickserv/status", MODULE_UNLOAD_CAPABILITY_OK)
