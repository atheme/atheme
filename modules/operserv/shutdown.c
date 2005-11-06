/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains functionality which implements the OService RAW command.
 *
 * $Id: shutdown.c 3601 2005-11-06 23:36:34Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/shutdown", FALSE, _modinit, _moddeinit,
	"$Id: shutdown.c 3601 2005-11-06 23:36:34Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void os_cmd_shutdown(char *origin);

command_t os_shutdown = { "SHUTDOWN", "Shuts down services.",
                          AC_SRA, os_cmd_shutdown };

list_t *os_cmdtree;
list_t *os_helptree;

void _modinit(module_t *m)
{
	os_cmdtree = module_locate_symbol("operserv/main", "os_cmdtree");
	os_helptree = module_locate_symbol("operserv/main", "os_helptree");

        command_add(&os_shutdown, os_cmdtree);
	help_addentry(os_helptree, "SHUTDOWN", "help/oservice/shutdown", NULL);
}

void _moddeinit()
{
	command_delete(&os_shutdown, os_cmdtree);
	help_delentry(os_helptree, "SHUTDOWN");
}

static void os_cmd_shutdown(char *origin)
{
	snoop("UPDATE: \2%s\2", origin);
	wallops("Updating database by request of \2%s\2.", origin);
	expire_check(NULL);
	db_save(NULL);

	logcommand(opersvs.me, user_find(origin), CMDLOG_ADMIN, "SHUTDOWN");
	snoop("SHUTDOWN: \2%s\2", origin);
	wallops("Shutting down by request of \2%s\2.", origin);

	runflags |= RF_SHUTDOWN;
}
