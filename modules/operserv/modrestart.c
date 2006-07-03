/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Module listing.
 *
 * $Id: modrestart.c 5696 2006-07-03 22:40:19Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/modrestart", TRUE, _modinit, _moddeinit,
	"$Id: modrestart.c 5696 2006-07-03 22:40:19Z jilles $",
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
	MODULE_USE_SYMBOL(os_cmdtree, "operserv/main", "os_cmdtree");
	MODULE_USE_SYMBOL(os_helptree, "operserv/main", "os_helptree");

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
	int loadedbefore, kept;
	boolean_t old_silent;
	boolean_t fail1 = FALSE;
	boolean_t unloaded_something;

	snoop("MODRESTART: \2%s\2", origin);
	logcommand(opersvs.me, user_find_named(origin), CMDLOG_ADMIN, "MODRESTART");
	wallops("Restarting modules by request of \2%s\2", origin);

	old_silent = config_options.silent;
	config_options.silent = TRUE; /* no wallops */

	loadedbefore = modules.count;

	/* unload everything we can */
	/* this contorted loop is necessary because unloading a module
	 * may unload other modules */
	do
	{
		unloaded_something = FALSE;
		LIST_FOREACH(n, modules.head)
		{
			module_t *m = n->data;

			/* don't touch core modules */
			/* don't touch ourselves either (ugly way) */
			if (!m->header->norestart && strcmp(m->header->name, "operserv/main") && strcmp(m->header->name, "operserv/modrestart"))
			{
				module_unload(m);
				unloaded_something = TRUE;
				break;
			}
		}
	} while (unloaded_something);

	kept = modules.count;

	/* now reload again */
	module_load_dir(MODDIR "/modules");
	cold_start = TRUE; /* XXX */
	fail1 = !conf_rehash();
	cold_start = FALSE;

	config_options.silent = old_silent;

	if (fail1)
	{
		wallops("Module restart failed, functionality will be very limited");
		notice(opersvs.nick, origin, "Module restart failed, fix it and try again or restart");
	}
	else
	{
		wallops("Module restart: %d modules unloaded; %d kept; %d modules now loaded", loadedbefore - kept, kept, modules.count);
		notice(opersvs.nick, origin, "Module restart: %d modules unloaded; %d kept; %d modules now loaded", loadedbefore - kept, kept, modules.count);
	}
}
