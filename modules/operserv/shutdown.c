/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains functionality which implements the OService RAW command.
 *
 * $Id: shutdown.c 2135 2005-09-05 01:28:25Z nenolod $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/shutdown", FALSE, _modinit, _moddeinit,
	"$Id: shutdown.c 2135 2005-09-05 01:28:25Z nenolod $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void os_cmd_shutdown(char *origin);

command_t os_shutdown = { "SHUTDOWN", "Shuts down services.",
                          AC_SRA, os_cmd_shutdown };

list_t *os_cmdtree;

void _modinit(module_t *m)
{
	os_cmdtree = module_locate_symbol("operserv/main", "os_cmdtree");
        command_add(&os_shutdown, os_cmdtree);
}

void _moddeinit()
{
	command_delete(&os_shutdown, os_cmdtree);
}

static void os_cmd_shutdown(char *origin)
{
	snoop("UPDATE: \2%s\2", origin);
	wallops("Updating database by request of \2%s\2.", origin);
	expire_check(NULL);
	db_save(NULL);

	snoop("SHUTDOWN: \2%s\2", origin);
	wallops("Shutting down by request of \2%s\2.", origin);

	runflags |= RF_SHUTDOWN;
}
