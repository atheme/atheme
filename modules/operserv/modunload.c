/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Loads a new module in.
 *
 * $Id: modunload.c 3601 2005-11-06 23:36:34Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/modunload", FALSE, _modinit, _moddeinit,
	"$Id: modunload.c 3601 2005-11-06 23:36:34Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void os_cmd_modunload(char *origin);

command_t os_modunload = { "MODUNLOAD", "Unloads a module.",
			 AC_SRA, os_cmd_modunload };

list_t *os_cmdtree;
extern list_t modules;

void _modinit(module_t *m)
{
	os_cmdtree = module_locate_symbol("operserv/main", "os_cmdtree");	
	command_add(&os_modunload, os_cmdtree);
}

void _moddeinit()
{
	command_delete(&os_modunload, os_cmdtree);
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
			slog(LG_INFO, "%s tried to unload an unloadable module",
				origin);
			notice(opersvs.nick, origin, "\2%s\2 is an unloadable module; "
					"it cannot be unloaded.", module);
			continue;
		}

		module_unload(m);

		logcommand(opersvs.me, user_find(origin), CMDLOG_ADMIN, "MODUNLOAD %s", module);
		notice(opersvs.nick, origin, "Module \2%s\2 unloaded.", module);
	}
}
