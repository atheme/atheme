/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains functionality which implements the OService RAW command.
 *
 * $Id: restart.c 2559 2005-10-04 06:56:29Z nenolod $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/restart", FALSE, _modinit, _moddeinit,
	"$Id: restart.c 2559 2005-10-04 06:56:29Z nenolod $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void os_cmd_restart(char *origin);

command_t os_restart = { "RESTART", "Restart services.",
                          AC_SRA, os_cmd_restart };

list_t *os_cmdtree;
list_t *os_helptree;

void _modinit(module_t *m)
{
	os_cmdtree = module_locate_symbol("operserv/main", "os_cmdtree");
	os_helptree = module_locate_symbol("operserv/main", "os_helptree");

        command_add(&os_restart, os_cmdtree);
	help_addentry(os_helptree, "RESTART", "help/oservice/restart", NULL);
}

void _moddeinit()
{
	command_delete(&os_restart, os_cmdtree);
	help_delentry(os_helptree, "RESTART");
}

static void os_cmd_restart(char *origin)
{
	snoop("UPDATE: \2%s\2", origin);
	wallops("Updating database by request of \2%s\2.", origin);
	expire_check(NULL);
	db_save(NULL);

	snoop("RESTART: \2%s\2", origin);
	wallops("Restarting in \2%d\2 seconds by request of \2%s\2.", me.restarttime, origin);

	runflags |= RF_RESTART;
}
