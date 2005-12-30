/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Module listing.
 *
 * $Id: modrestart.c 4375 2005-12-30 15:48:59Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/modrestart", TRUE, _modinit, _moddeinit,
	"$Id: modrestart.c 4375 2005-12-30 15:48:59Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void os_cmd_modrestart(char *origin);

command_t os_modrestart = { "MODRESTART", "Restarts loaded modules.",
			    PRIV_ADMIN, os_cmd_modrestart };

list_t *os_cmdtree;
list_t *os_helptree;
#ifdef _WIN32
extern __declspec (dllimport) list_t modules;
#else
extern list_t modules;
#endif

void _modinit(module_t *m)
{
	os_cmdtree = module_locate_symbol("operserv/main", "os_cmdtree");
	os_helptree = module_locate_symbol("operserv/main", "os_helptree");
	command_add(&os_modrestart, os_cmdtree);
	help_addentry(os_helptree, "MODRESTART", "help/oservice/modrestart", NULL);
}

void _moddeinit()
{
	command_delete(&os_modrestart, os_cmdtree);
	help_delentry(os_helptree, "MODRESTART");
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
