/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Loads a new module in.
 *
 * $Id: modunload.c 6631 2006-10-02 10:24:13Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/modunload", FALSE, _modinit, _moddeinit,
	"$Id: modunload.c 6631 2006-10-02 10:24:13Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void os_cmd_modunload(sourceinfo_t *si, int parc, char *parv[]);

command_t os_modunload = { "MODUNLOAD", "Unloads a module.", PRIV_ADMIN, 20, os_cmd_modunload };

list_t *os_cmdtree;
list_t *os_helptree;
extern list_t modules;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(os_cmdtree, "operserv/main", "os_cmdtree");	
	MODULE_USE_SYMBOL(os_helptree, "operserv/main", "os_helptree");	

	command_add(&os_modunload, os_cmdtree);
	help_addentry(os_helptree, "MODUNLOAD", "help/oservice/modunload", NULL);
}

void _moddeinit()
{
	command_delete(&os_modunload, os_cmdtree);
	help_delentry(os_helptree, "MODUNLOAD");
}

static void os_cmd_modunload(sourceinfo_t *si, int parc, char *parv[])
{
	char *module;
	int i;
	module_t *m;

	if (parc < 1)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "MODUNLOAD");
		command_fail(si, fault_needmoreparams, "Syntax: MODUNLOAD <module...>");
		return;
	}
	i = 0;
	while (i < parc)
	{
		module = parv[i++];
		m = module_find_published(module);

		if (!m)
		{
			command_fail(si, fault_nosuch_target, "\2%s\2 is not loaded; "
					"it cannot be unloaded.", module);
			continue;
		}

		if (m->header->norestart)
		{
			slog(LG_INFO, "%s tried to unload a permanent module",
				get_oper_name(si));
			command_fail(si, fault_noprivs, "\2%s\2 is an permanent module; "
					"it cannot be unloaded.", module);
			continue;
		}

		if (!strcmp(m->header->name, "operserv/main") || !strcmp(m->header->name, "operserv/modload") || !strcmp(m->header->name, "operserv/modunload"))
		{
			command_fail(si, fault_noprivs, "Refusing to unload \2%s\2.",
					module);
			continue;
		}

		module_unload(m);

		logcommand(si, CMDLOG_ADMIN, "MODUNLOAD %s", module);
		command_success_nodata(si, "Module \2%s\2 unloaded.", module);
	}
}
