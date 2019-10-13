/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2006 William Pitcock, et al.
 *
 * Loads a new module in.
 */

#include <atheme.h>

static void
os_cmd_modload(struct sourceinfo *si, int parc, char *parv[])
{
	char *module;
	struct module *m;
	int i;

	if (parc < 1)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "MODLOAD");
		command_fail(si, fault_needmoreparams, _("Syntax: MODLOAD <module...>"));
		return;
	}
	i = 0;
	while (i < parc)
	{
		module = parv[i++];
		if (module_find_published(module))
		{
			command_fail(si, fault_nochange, _("\2%s\2 is already loaded."), module);
			continue;
		}

		logcommand(si, CMDLOG_ADMIN, "MODLOAD: \2%s\2", module);
		m = module_load(module);

		if (m != NULL)
			command_success_nodata(si, _("Module \2%s\2 loaded."), module);
		else
			command_fail(si, fault_nosuch_target, _("Module \2%s\2 failed to load."), module);
	}

	if (conf_need_rehash)
	{
		logcommand(si, CMDLOG_ADMIN, "REHASH (MODLOAD)");
		wallops("Rehashing \2%s\2 to complete module load by request of \2%s\2.", config_file, get_oper_name(si));
		if (!conf_rehash())
			command_fail(si, fault_nosuch_target, _("REHASH of \2%s\2 failed. Please correct any errors in the file and try again."), config_file);
	}
}

static struct command os_modload = {
	.name           = "MODLOAD",
	.desc           = N_("Loads a module."),
	.access         = PRIV_ADMIN,
	.maxparc        = 20,
	.cmd            = &os_cmd_modload,
	.help           = { .path = "oservice/modload" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "operserv/main")

	service_named_bind_command("operserv", &os_modload);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("operserv", &os_modload);
}

SIMPLE_DECLARE_MODULE_V1("operserv/modload", MODULE_UNLOAD_CAPABILITY_OK)
