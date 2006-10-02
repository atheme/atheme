/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains functionality which implements the OService RAW command.
 *
 * $Id: restart.c 6631 2006-10-02 10:24:13Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/restart", FALSE, _modinit, _moddeinit,
	"$Id: restart.c 6631 2006-10-02 10:24:13Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void os_cmd_restart(sourceinfo_t *si, int parc, char *parv[]);

command_t os_restart = { "RESTART", "Restart services.", PRIV_ADMIN, 0, os_cmd_restart };

list_t *os_cmdtree;
list_t *os_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(os_cmdtree, "operserv/main", "os_cmdtree");
	MODULE_USE_SYMBOL(os_helptree, "operserv/main", "os_helptree");

        command_add(&os_restart, os_cmdtree);
	help_addentry(os_helptree, "RESTART", "help/oservice/restart", NULL);
}

void _moddeinit()
{
	command_delete(&os_restart, os_cmdtree);
	help_delentry(os_helptree, "RESTART");
}

static void os_cmd_restart(sourceinfo_t *si, int parc, char *parv[])
{
	snoop("UPDATE: \2%s\2", get_oper_name(si));
	wallops("Updating database by request of \2%s\2.", get_oper_name(si));
	expire_check(NULL);
	db_save(NULL);

	logcommand(si, CMDLOG_ADMIN, "RESTART");
	snoop("RESTART: \2%s\2", get_oper_name(si));
	wallops("Restarting by request of \2%s\2.", get_oper_name(si));

	runflags |= RF_RESTART;
}
