/*
 * Copyright (c) 2005-2006 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Removes a module from memory.
 *
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/modunload", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void os_cmd_modunload(sourceinfo_t *si, int parc, char *parv[]);

command_t os_modunload = { "MODUNLOAD", N_("Unloads a module."), PRIV_ADMIN, 20, os_cmd_modunload, { .path = "oservice/modunload" } };

extern mowgli_list_t modules;

void _modinit(module_t *m)
{
	service_named_bind_command("operserv", &os_modunload);
}

void _moddeinit()
{
	service_named_unbind_command("operserv", &os_modunload);
}

static void os_cmd_modunload(sourceinfo_t *si, int parc, char *parv[])
{
	char *module;
	int i;
	module_t *m;

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

		if (m->header->norestart)
		{
			slog(LG_INFO, "\2%s\2 tried to unload a permanent module",
				get_oper_name(si));
			command_fail(si, fault_noprivs, _("\2%s\2 is a permanent module; it cannot be unloaded."), module);
			continue;
		}

		if (!strcmp(m->header->name, "operserv/main") || !strcmp(m->header->name, "operserv/modload") || !strcmp(m->header->name, "operserv/modunload"))
		{
			command_fail(si, fault_noprivs, _("Refusing to unload \2%s\2."),
					module);
			continue;
		}

		logcommand(si, CMDLOG_ADMIN, "MODUNLOAD: \2%s\2", module);

		module_unload(m);

		command_success_nodata(si, _("Module \2%s\2 unloaded."), module);
	}
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
