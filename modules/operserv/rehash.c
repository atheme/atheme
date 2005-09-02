/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains functionality which implements the OService REHASH command.
 *
 * $Id: rehash.c 2037 2005-09-02 05:13:29Z nenolod $
 */

#include "atheme.h"

DECLARE_MODULE_V1("operserv/rehash", FALSE, _modinit, _moddeinit);

static void os_cmd_rehash(char *origin);

command_t os_rehash = { "REHASH", "Reload the configuration data.",
                        AC_SRA, os_cmd_rehash };

list_t *os_cmdtree;

void _modinit(module_t *m)
{
	os_cmdtree = module_locate_symbol("operserv/main", "os_cmdtree");
        command_add(&os_rehash, os_cmdtree);
}

void _moddeinit()
{
	command_delete(&os_rehash, os_cmdtree);
}

/* REHASH */
void os_cmd_rehash(char *origin)
{
	snoop("UPDATE: \2%s\2", origin);
	wallops("Updating database by request of \2%s\2.", origin);
	expire_check(NULL);
	db_save(NULL);

	snoop("REHASH: \2%s\2", origin);
	wallops("Rehashing \2%s\2 by request of \2%s\2.", config_file, origin);

	if (!conf_rehash())
	{
		notice(opersvs.nick, origin, "REHASH of \2%s\2 failed. Please corrrect any errors in the " "file and try again.", config_file);
	}
}
