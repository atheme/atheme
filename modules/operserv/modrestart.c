/*
 * Copyright (c) 2005-2006 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Module restart.
 *
 * $Id: modrestart.c 8079 2007-04-02 17:37:39Z nenolod $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/modrestart", TRUE, _modinit, _moddeinit,
	"$Id: modrestart.c 8079 2007-04-02 17:37:39Z nenolod $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void os_cmd_modrestart(sourceinfo_t *si, int parc, char *parv[]);

command_t os_modrestart = { "MODRESTART", N_("Restarts loaded modules."), PRIV_ADMIN, 0, os_cmd_modrestart };

list_t *os_cmdtree;
list_t *os_helptree;
extern list_t modules;

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

static void os_cmd_modrestart(sourceinfo_t *si, int parc, char *parv[])
{
	node_t *n;
	int loadedbefore, kept;
	boolean_t old_silent;
	boolean_t fail1 = FALSE;
	boolean_t unloaded_something;

	snoop("MODRESTART: \2%s\2", get_oper_name(si));
	logcommand(si, CMDLOG_ADMIN, "MODRESTART");
	wallops("Restarting modules by request of \2%s\2", get_oper_name(si));

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
		command_fail(si, fault_nosuch_target, _("Module restart failed, fix it and try again or restart"));
	}
	else
	{
		wallops("Module restart: %d modules unloaded; %d kept; %d modules now loaded", loadedbefore - kept, kept, modules.count);
		command_success_nodata(si, _("Module restart: %d modules unloaded; %d kept; %d modules now loaded"), loadedbefore - kept, kept, modules.count);
	}
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
