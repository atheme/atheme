/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2006 Patrick Fish, et al.
 * Copyright (C) 2011 William Pitcock <nenolod@dereferenced.org>
 * Copyright (C) 2016 Atheme Development Group (https://atheme.github.io/)
 *
 * This file contains functionality which implements the OService SPECS command.
 */

#include <atheme.h>
#include "../groupserv/main/groupserv_common.h"

struct privilege
{
	const char *priv;
	const char *desc;
};

struct priv_category
{
	const char *name;
	struct privilege privs[];
};

static void
os_cmd_specs(struct sourceinfo *si, int parc, char *parv[])
{
	static const struct priv_category nickserv_privs = {
		N_("Nicknames/Accounts"),
		{
			{ PRIV_USER_AUSPEX, N_("view concealed information about accounts") },
			{ PRIV_USER_ADMIN, N_("drop accounts, freeze accounts, reset passwords") },
			{ PRIV_USER_SENDPASS, N_("send passwords") },
			{ PRIV_USER_VHOST, N_("set vhosts") },
			{ PRIV_USER_FREGISTER, N_("register accounts on behalf of another user") },
			{ PRIV_MARK, N_("mark accounts") },
			{ PRIV_HOLD, N_("hold accounts") },
			{ PRIV_LOGIN_NOLIMIT, N_("bypass login limits") },
			{ NULL, NULL },
		}
	};

	static const struct priv_category chanserv_privs = {
		N_("Channels"),
		{
			{ PRIV_CHAN_AUSPEX, N_("view concealed information about channels") },
			{ PRIV_CHAN_ADMIN, N_("drop channels, close channels, transfer ownership") },
			{ PRIV_CHAN_CMODES, N_("mlock operator modes") },
			{ PRIV_JOIN_STAFFONLY, N_("join staff channels") },
			{ PRIV_MARK, N_("mark channels") },
			{ PRIV_HOLD, N_("hold channels") },
			{ PRIV_REG_NOLIMIT, N_("bypass channel registration limits") },
			{ NULL, NULL },
		}
	};

	static const struct priv_category general_privs = {
		N_("General"),
		{
			{ PRIV_SERVER_AUSPEX, N_("view concealed information about servers") },
			{ PRIV_VIEWPRIVS, N_("view privileges of other users") },
			{ PRIV_FLOOD, N_("exempt from flood control") },
			{ PRIV_ADMIN, N_("administer services") },
			{ PRIV_METADATA, N_("edit private and internal metadata") },
			{ NULL, NULL },
		}
	};

	static const struct priv_category operserv_privs = {
		N_("OperServ"),
		{
			{ PRIV_OMODE, N_("set channel modes") },
			{ PRIV_AKILL, N_("add and remove autokills") },
			{ PRIV_MASS_AKILL, N_("masskill channels or regexes") },
			{ PRIV_JUPE, N_("jupe servers") },
			{ PRIV_NOOP, N_("NOOP access") },
			{ PRIV_GLOBAL, N_("send global notices") },
			{ PRIV_GRANT, N_("edit oper privileges") },
			{ NULL, NULL },
		}
	};

	static const struct priv_category groupserv_privs = {
		N_("GroupServ"),
		{
			{ PRIV_GROUP_AUSPEX, N_("view concealed information about groups") },
			{ PRIV_GROUP_ADMIN, N_("administer groups") },
			{ PRIV_REG_NOLIMIT, N_("bypass group registration limits") },
			{ NULL, NULL },
		}
	};

	static const struct priv_category *priv_categories[] = {
		&nickserv_privs, &chanserv_privs, &general_privs, &operserv_privs, &groupserv_privs,
	};

	struct user *tu = NULL;
	struct operclass *cl = NULL;
	const char *targettype = parv[0];
	const char *target = parv[1];
	unsigned int i;
	int j, n;

	if (!has_any_privs(si))
	{
		command_fail(si, fault_noprivs, _("You are not authorized to use %s."), si->service->nick);
		return;
	}

	if (targettype != NULL)
	{
		if (!has_priv(si, PRIV_VIEWPRIVS))
		{
			command_fail(si, fault_noprivs, STR_NO_PRIVILEGE, PRIV_VIEWPRIVS);
			return;
		}
		if (target == NULL)
			target = "?";
		if (!strcasecmp(targettype, "USER"))
		{
			tu = user_find_named(target);
			if (tu == NULL)
			{
				command_fail(si, fault_nosuch_target, _("\2%s\2 is not online."), target);
				return;
			}
			if (!has_any_privs_user(tu))
			{
				command_success_nodata(si, _("\2%s\2 is unprivileged."), tu->nick);
				return;
			}
			if (is_internal_client(tu))
			{
				command_fail(si, fault_noprivs, _("\2%s\2 is an internal client."), tu->nick);
				return;
			}
		}
		else if (!strcasecmp(targettype, "OPERCLASS") || !strcasecmp(targettype, "CLASS"))
		{
			cl = operclass_find(target);
			if (cl == NULL)
			{
				command_fail(si, fault_nosuch_target, _("No such oper class \2%s\2."), target);
				return;
			}
		}
		else
		{
			command_fail(si, fault_badparams, _("Valid target types: USER, OPERCLASS."));
			return;
		}
	}
	else
		tu = si->su;

	if (targettype == NULL)
		command_success_nodata(si, _("Privileges for \2%s\2:"), get_source_name(si));
	else if (tu)
		command_success_nodata(si, _("Privileges for \2%s\2:"), tu->nick);
	else
		command_success_nodata(si, _("Privileges for oper class \2%s\2:"), cl->name);

	for (i = 0; i < ARRAY_SIZE(priv_categories); i++)
	{
		const struct priv_category *cat = priv_categories[i];

		command_success_nodata(si, "\2%s\2:", _(cat->name));

		for (j = n = 0; cat->privs[j].priv != NULL; j++)
		{
			if (targettype == NULL ? has_priv(si, cat->privs[j].priv) : (tu ? has_priv_user(tu, cat->privs[j].priv) : has_priv_operclass(cl, cat->privs[j].priv)))
			{
				command_success_nodata(si, "    %s (%s)", cat->privs[j].priv, _(cat->privs[j].desc));
				++n;
			}
		}

		if (!n)
			command_success_nodata(si, "    %s", _("(no privileges held)"));
	}

	command_success_nodata(si, _("End of privileges."));

	if (targettype == NULL)
		logcommand(si, CMDLOG_ADMIN, "SPECS");
	else if (tu)
		logcommand(si, CMDLOG_ADMIN, "SPECS:USER: \2%s!%s@%s\2", tu->nick, tu->user, tu->vhost);
	else
		logcommand(si, CMDLOG_ADMIN, "SPECS:OPERCLASS: \2%s\2", cl->name);
}

static struct command os_specs = {
	.name           = "SPECS",
	.desc           = N_("Shows oper flags."),
	.access         = AC_NONE,
	.maxparc        = 2,
	.cmd            = &os_cmd_specs,
	.help           = { .path = "oservice/specs" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "operserv/main")

        service_named_bind_command("operserv", &os_specs);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("operserv", &os_specs);
}

SIMPLE_DECLARE_MODULE_V1("operserv/specs", MODULE_UNLOAD_CAPABILITY_OK)
