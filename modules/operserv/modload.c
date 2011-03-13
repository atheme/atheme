/*
 * Copyright (c) 2005-2006 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Loads a new module in.
 *
 */

#include "atheme.h"
#include "conf.h"

DECLARE_MODULE_V1
(
	"operserv/modload", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void os_cmd_modload(sourceinfo_t *si, int parc, char *parv[]);

command_t os_modload = { "MODLOAD", N_("Loads a module."), PRIV_ADMIN, 20, os_cmd_modload, { .path = "oservice/modload" } };

void _modinit(module_t *m)
{
	service_named_bind_command("operserv", &os_modload);
}

void _moddeinit(module_unload_intent_t intent)
{
	service_named_unbind_command("operserv", &os_modload);
}

static void os_cmd_modload(sourceinfo_t *si, int parc, char *parv[])
{
	char *module;
	module_t *m;
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

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
