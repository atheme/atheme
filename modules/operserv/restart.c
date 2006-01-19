/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains functionality which implements the OService RAW command.
 *
 * $Id: restart.c 4613 2006-01-19 23:52:30Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/restart", FALSE, _modinit, _moddeinit,
	"$Id: restart.c 4613 2006-01-19 23:52:30Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void os_cmd_restart(char *origin);

command_t os_restart = { "RESTART", "Restart services.",
                          PRIV_ADMIN, os_cmd_restart };

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

	logcommand(opersvs.me, user_find_named(origin), CMDLOG_ADMIN, "RESTART");
	snoop("RESTART: \2%s\2", origin);
	wallops("Restarting by request of \2%s\2.", origin);

	runflags |= RF_RESTART;
}
