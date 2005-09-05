/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains functionality which implements the OService RAW command.
 *
 * $Id: restart.c 2135 2005-09-05 01:28:25Z nenolod $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/restart", FALSE, _modinit, _moddeinit,
	"$Id: restart.c 2135 2005-09-05 01:28:25Z nenolod $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void os_cmd_restart(char *origin);

command_t os_restart = { "RESTART", "Restart services.",
                          AC_SRA, os_cmd_restart };

list_t *os_cmdtree;

void _modinit(module_t *m)
{
	os_cmdtree = module_locate_symbol("operserv/main", "os_cmdtree");
        command_add(&os_restart, os_cmdtree);
}

void _moddeinit()
{
	command_delete(&os_restart, os_cmdtree);
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
