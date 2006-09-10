/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains functionality which implements the OService RAW command.
 *
 * $Id: raw.c 6337 2006-09-10 15:54:41Z pippijn $
 */

#include "atheme.h"
#include "uplink.h"

DECLARE_MODULE_V1
(
	"operserv/raw", FALSE, _modinit, _moddeinit,
	"$Id: raw.c 6337 2006-09-10 15:54:41Z pippijn $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void os_cmd_raw(sourceinfo_t *si, int parc, char *parv[]);

command_t os_raw = { "RAW", "Sends data to the uplink.", PRIV_ADMIN, 1, os_cmd_raw };

list_t *os_cmdtree;
list_t *os_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(os_cmdtree, "operserv/main", "os_cmdtree");
	MODULE_USE_SYMBOL(os_helptree, "operserv/main", "os_helptree");

        command_add(&os_raw, os_cmdtree);
	help_addentry(os_helptree, "RAW", "help/oservice/raw", NULL);
}

void _moddeinit()
{
	command_delete(&os_raw, os_cmdtree);
	help_delentry(os_helptree, "RAW");
}

static void os_cmd_raw(sourceinfo_t *si, int parc, char *parv[])
{
	char *s = parv[0];

	if (!config_options.raw)
		return;

	if (!s)
	{
		notice(opersvs.nick, si->su->nick, STR_INSUFFICIENT_PARAMS, "RAW");
		notice(opersvs.nick, si->su->nick, "Syntax: RAW <parameters>");
		return;
	}

	snoop("RAW: \"%s\" by \2%s\2", s, si->su->nick);
	logcommand(opersvs.me, si->su, CMDLOG_ADMIN, "RAW %s", s);
	sts("%s", s);
}
