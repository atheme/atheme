/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains functionality which implements the OService REHASH command.
 *
 * $Id: rehash.c 5686 2006-07-03 16:25:03Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/rehash", FALSE, _modinit, _moddeinit,
	"$Id: rehash.c 5686 2006-07-03 16:25:03Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void os_cmd_rehash(char *origin);

command_t os_rehash = { "REHASH", "Reload the configuration data.",
                        PRIV_ADMIN, os_cmd_rehash };

list_t *os_cmdtree;
list_t *os_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(os_cmdtree, "operserv/main", "os_cmdtree");
	MODULE_USE_SYMBOL(os_helptree, "operserv/main", "os_helptree");

        command_add(&os_rehash, os_cmdtree);
	help_addentry(os_helptree, "REHASH", "help/oservice/rehash", NULL);
}

void _moddeinit()
{
	command_delete(&os_rehash, os_cmdtree);
	help_delentry(os_helptree, "REHASH");
}

/* REHASH */
void os_cmd_rehash(char *origin)
{
	snoop("UPDATE: \2%s\2", origin);
	wallops("Updating database by request of \2%s\2.", origin);
	expire_check(NULL);
	db_save(NULL);

	snoop("REHASH: \2%s\2", origin);
	logcommand(opersvs.me, user_find_named(origin), CMDLOG_ADMIN, "REHASH");
	wallops("Rehashing \2%s\2 by request of \2%s\2.", config_file, origin);

	if (!conf_rehash())
	{
		notice(opersvs.nick, origin, "REHASH of \2%s\2 failed. Please corrrect any errors in the " "file and try again.", config_file);
	}
}
