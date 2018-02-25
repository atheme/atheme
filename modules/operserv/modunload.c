/*
 * Copyright (c) 2005-2006 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Removes a module from memory.
 */

#include "atheme.h"

static void os_cmd_modunload(struct sourceinfo *si, int parc, char *parv[]);

static struct command os_modunload = { "MODUNLOAD", N_("Unloads a module."), PRIV_ADMIN, 20, os_cmd_modunload, { .path = "oservice/modunload" } };

static void
mod_init(struct module *const restrict m)
{
	service_named_bind_command("operserv", &os_modunload);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("operserv", &os_modunload);
}

static void
os_cmd_modunload(struct sourceinfo *si, int parc, char *parv[])
{
	char *module;
	int i;
	struct module *m;

	if (parc < 1)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "MODUNLOAD");
		command_fail(si, fault_needmoreparams, _("Syntax: MODUNLOAD <module...>"));
		return;
	}
	i = 0;
	while (i < parc)
	{
		module = parv[i++];
		m = module_find_published(module);

		if (!m)
		{
			command_fail(si, fault_nosuch_target, _("\2%s\2 is not loaded; it cannot be unloaded."), module);
			continue;
		}

		if (m->can_unload != MODULE_UNLOAD_CAPABILITY_OK)
		{
			slog(LG_INFO, "\2%s\2 tried to unload a permanent module",
				get_oper_name(si));
			command_fail(si, fault_noprivs, _("\2%s\2 is a permanent module; it cannot be unloaded."), module);
			continue;
		}

		if (!strcmp(m->name, "operserv/main") || !strcmp(m->name, "operserv/modload") || !strcmp(m->name, "operserv/modunload"))
		{
			command_fail(si, fault_noprivs, _("Refusing to unload \2%s\2."),
					module);
			continue;
		}

		logcommand(si, CMDLOG_ADMIN, "MODUNLOAD: \2%s\2", module);

		module_unload(m, MODULE_UNLOAD_INTENT_PERM);

		command_success_nodata(si, _("Module \2%s\2 unloaded."), module);
	}
}

SIMPLE_DECLARE_MODULE_V1("operserv/modunload", MODULE_UNLOAD_CAPABILITY_OK)
