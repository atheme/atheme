/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains functionality which implements the OService UPDATE command.
 *
 * $Id: update.c 2135 2005-09-05 01:28:25Z nenolod $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/update", FALSE, _modinit, _moddeinit,
	"$Id: update.c 2135 2005-09-05 01:28:25Z nenolod $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void os_cmd_update(char *origin);

command_t os_update = { "UPDATE", "Flushes services database to disk.",
                        AC_SRA, os_cmd_update };

list_t *os_cmdtree;

void _modinit(module_t *m)
{
	os_cmdtree = module_locate_symbol("operserv/main", "os_cmdtree");
        command_add(&os_update, os_cmdtree);
}

void _moddeinit()
{
	command_delete(&os_update, os_cmdtree);
}

void os_cmd_update(char *origin)
{
	snoop("UPDATE: \2%s\2", origin);
	wallops("Updating database by request of \2%s\2.", origin);
	expire_check(NULL);
	db_save(NULL);
}
