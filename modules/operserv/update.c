/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains functionality which implements the OService UPDATE command.
 *
 * $Id: update.c 5686 2006-07-03 16:25:03Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/update", FALSE, _modinit, _moddeinit,
	"$Id: update.c 5686 2006-07-03 16:25:03Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void os_cmd_update(char *origin);

command_t os_update = { "UPDATE", "Flushes services database to disk.",
                        PRIV_ADMIN, os_cmd_update };

list_t *os_cmdtree;
list_t *os_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(os_cmdtree, "operserv/main", "os_cmdtree");
	MODULE_USE_SYMBOL(os_helptree, "operserv/main", "os_helptree");

        command_add(&os_update, os_cmdtree);
	help_addentry(os_helptree, "UPDATE", "help/oservice/update", NULL);
}

void _moddeinit()
{
	command_delete(&os_update, os_cmdtree);
	help_delentry(os_helptree, "UPDATE");
}

void os_cmd_update(char *origin)
{
	snoop("UPDATE: \2%s\2", origin);
	logcommand(opersvs.me, user_find_named(origin), CMDLOG_ADMIN, "UPDATE");
	wallops("Updating database by request of \2%s\2.", origin);
	expire_check(NULL);
	db_save(NULL);
}
