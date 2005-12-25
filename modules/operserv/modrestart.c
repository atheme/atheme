/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Module listing.
 *
 * $Id: modrestart.c 4183 2005-12-25 21:01:34Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/modrestart", TRUE, _modinit, _moddeinit,
	"$Id: modrestart.c 4183 2005-12-25 21:01:34Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void os_cmd_modrestart(char *origin);

command_t os_modrestart = { "MODRESTART", "Restarts loaded modules.",
			    AC_SRA, os_cmd_modrestart };

list_t *os_cmdtree;
#ifdef _WIN32
extern __declspec (dllimport) list_t modules;
#else
extern list_t modules;
#endif

void _modinit(module_t *m)
{
	os_cmdtree = module_locate_symbol("operserv/main", "os_cmdtree");
	command_add(&os_modrestart, os_cmdtree);
}

void _moddeinit()
{
	command_delete(&os_modrestart, os_cmdtree);
}

static void os_cmd_modrestart(char *origin)
{
	node_t *n;
	uint32_t iter = 0;
	uint32_t reloaded = 0;

	snoop("MODRESTART: \2%s\2", origin);
	logcommand(opersvs.me, user_find(origin), CMDLOG_ADMIN, "MODRESTART");
	wallops("Restarting modules by request of \2%s\2", origin);

	LIST_FOREACH(n, modules.head)
	{
		module_t *m = n->data;

		/* don't touch core modules */
		if (!m->header->norestart)
		{
			char *modpath = sstrdup(m->modpath);
			module_unload(m);
			module_load(modpath);
			free(modpath);
			reloaded++;
		}

		/* Have we unloaded all the modules? */
		if (iter == (modules.count - 1))
			break;

		iter++;
	}

	module_load_dir(MODDIR "/modules");

	notice(opersvs.nick, origin, "Module restart: %d modules reloaded; %d modules now loaded", reloaded, modules.count);
}
