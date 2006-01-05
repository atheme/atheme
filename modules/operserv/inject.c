/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains functionality which implements the OService RAW command.
 *
 * $Id: inject.c 4491 2006-01-05 00:06:26Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/inject", FALSE, _modinit, _moddeinit,
	"$Id: inject.c 4491 2006-01-05 00:06:26Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void os_cmd_inject(char *origin);

command_t os_inject = { "INJECT", "Fakes data from the uplink (debugging tool).",
                        PRIV_ADMIN, os_cmd_inject };

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
		notice(opersvs.nick, origin, STR_INSUFFICIENT_PARAMS, "INJECT");
		notice(opersvs.nick, origin, "Syntax: INJECT <parameters>");
		return;
	}

	logcommand(opersvs.me, user_find(origin), CMDLOG_ADMIN, "INJECT %s", inject);

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
