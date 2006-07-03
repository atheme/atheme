/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Loads a new module in.
 *
 * $Id: modunload.c 5700 2006-07-03 22:56:53Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/modunload", FALSE, _modinit, _moddeinit,
	"$Id: modunload.c 5700 2006-07-03 22:56:53Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void os_cmd_modunload(char *origin);

command_t os_modunload = { "MODUNLOAD", "Unloads a module.",
			 PRIV_ADMIN, os_cmd_modunload };

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

static void os_cmd_modunload(char *origin)
{
	char *module;

	while((module = strtok(NULL, " ")))
	{
		module_t *m = module_find_published(module);

		if (!m)
		{
			notice(opersvs.nick, origin, "\2%s\2 is not loaded; "
					"it cannot be unloaded.", module);
			continue;
		}

		if (m->header->norestart)
		{
			slog(LG_INFO, "%s tried to unload a permanent module",
				origin);
			notice(opersvs.nick, origin, "\2%s\2 is an permanent module; "
					"it cannot be unloaded.", module);
			continue;
		}

		if (!strcmp(m->header->name, "operserv/main") || !strcmp(m->header->name, "operserv/modload") || !strcmp(m->header->name, "operserv/modunload"))
		{
			notice(opersvs.nick, origin, "Refusing to unload \2%s\2.",
					module);
			continue;
		}

		module_unload(m);

		logcommand(opersvs.me, user_find_named(origin), CMDLOG_ADMIN, "MODUNLOAD %s", module);
		notice(opersvs.nick, origin, "Module \2%s\2 unloaded.", module);
	}
}
