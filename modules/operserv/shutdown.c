/*
 * Copyright (c) 2005-2006 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains functionality which implements the OService SHUTDOWN command.
 *
 * $Id: shutdown.c 7855 2007-03-06 00:43:08Z pippijn $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/shutdown", false, _modinit, _moddeinit,
	"$Id: shutdown.c 7855 2007-03-06 00:43:08Z pippijn $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void os_cmd_shutdown(sourceinfo_t *si, int parc, char *parv[]);

command_t os_shutdown = { "SHUTDOWN", N_("Shuts down services."), PRIV_ADMIN, 0, os_cmd_shutdown };

list_t *os_cmdtree;
list_t *os_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(os_cmdtree, "operserv/main", "os_cmdtree");
	MODULE_USE_SYMBOL(os_helptree, "operserv/main", "os_helptree");

        command_add(&os_shutdown, os_cmdtree);
	help_addentry(os_helptree, "SHUTDOWN", "help/oservice/shutdown", NULL);
}

void _moddeinit()
{
	command_delete(&os_shutdown, os_cmdtree);
	help_delentry(os_helptree, "SHUTDOWN");
}

static void os_cmd_shutdown(sourceinfo_t *si, int parc, char *parv[])
{
	logcommand(si, CMDLOG_ADMIN, "SHUTDOWN");
	snoop("SHUTDOWN: \2%s\2", get_oper_name(si));
	wallops("Shutting down by request of \2%s\2.", get_oper_name(si));

	runflags |= RF_SHUTDOWN;
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
