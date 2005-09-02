/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains functionality which implements the OService RAW command.
 *
 * $Id: inject.c 2037 2005-09-02 05:13:29Z nenolod $
 */

#include "atheme.h"

DECLARE_MODULE_V1("operserv/inject", FALSE, _modinit, _moddeinit);

static void os_cmd_inject(char *origin);

command_t os_inject = { "INJECT", "Fakes data from the uplink (debugging tool).",
                        AC_SRA, os_cmd_inject };

list_t *os_cmdtree;

void _modinit(module_t *m)
{
	os_cmdtree = module_locate_symbol("operserv/main", "os_cmdtree");
        command_add(&os_inject, os_cmdtree);
}

void _moddeinit()
{
	command_delete(&os_inject, os_cmdtree);
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
