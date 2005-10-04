/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains functionality which implements the OService RAW command.
 *
 * $Id: inject.c 2559 2005-10-04 06:56:29Z nenolod $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/inject", FALSE, _modinit, _moddeinit,
	"$Id: inject.c 2559 2005-10-04 06:56:29Z nenolod $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void os_cmd_inject(char *origin);

command_t os_inject = { "INJECT", "Fakes data from the uplink (debugging tool).",
                        AC_SRA, os_cmd_inject };

list_t *os_cmdtree;
list_t *os_helptree;

void _modinit(module_t *m)
{
	os_cmdtree = module_locate_symbol("operserv/main", "os_cmdtree");
	os_helptree = module_locate_symbol("operserv/main", "os_helptree");

        command_add(&os_inject, os_cmdtree);
	help_addentry(os_helptree, "INJECT", "help/oservice/inject", NULL);
}

void _moddeinit()
{
	command_delete(&os_inject, os_cmdtree);
	help_delentry(os_helptree, "INJECT");
}

static void os_cmd_inject(char *origin)
{
	char *inject;
	static boolean_t injecting = FALSE;
	inject = strtok(NULL, "");

	if (!config_options.raw)
		return;

	if (!inject)
	{
		notice(opersvs.nick, origin, "Insufficient parameters for \2INJECT\2.");
		notice(opersvs.nick, origin, "Syntax: INJECT <parameters>");
		return;
	}

	/* looks like someone INJECT'd an INJECT command.
	 * this is probably a bad thing.
	 */
	if (injecting == TRUE)
	{
		notice(opersvs.nick, origin, "You cannot inject an INJECT command.");
		return;
	}

	injecting = TRUE;
	irc_parse(inject);
	injecting = FALSE;
}
