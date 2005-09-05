/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains functionality which implements the OService RAW command.
 *
 * $Id: raw.c 2135 2005-09-05 01:28:25Z nenolod $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/raw", FALSE, _modinit, _moddeinit,
	"$Id: raw.c 2135 2005-09-05 01:28:25Z nenolod $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void os_cmd_raw(char *origin);

command_t os_raw = { "RAW", "Sends data to the uplink.",
                        AC_SRA, os_cmd_raw };

list_t *os_cmdtree;

void _modinit(module_t *m)
{
	os_cmdtree = module_locate_symbol("operserv/main", "os_cmdtree");
        command_add(&os_raw, os_cmdtree);
}

void _moddeinit()
{
	command_delete(&os_raw, os_cmdtree);
}

static void os_cmd_raw(char *origin)
{
	char *s = strtok(NULL, "");

	if (!config_options.raw)
		return;

	if (!s)
	{
		notice(opersvs.nick, origin, "Insufficient parameters for \2RAW\2.");
		notice(opersvs.nick, origin, "Syntax: RAW <parameters>");
		return;
	}

	snoop("RAW: \"%s\" by \2%s\2", s, origin);
	sts("%s", s);
}
