/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Jupiters a server.
 *
 * $Id: jupe.c 5686 2006-07-03 16:25:03Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/jupe", FALSE, _modinit, _moddeinit,
	"$Id: jupe.c 5686 2006-07-03 16:25:03Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void os_cmd_jupe(char *origin);

command_t os_jupe = { "JUPE", "Jupiters a server.", PRIV_JUPE, os_cmd_jupe };

list_t *os_cmdtree;
list_t *os_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(os_cmdtree, "operserv/main", "os_cmdtree");
	MODULE_USE_SYMBOL(os_helptree, "operserv/main", "os_helptree");

	command_add(&os_jupe, os_cmdtree);
	help_addentry(os_helptree, "JUPE", "help/oservice/jupe", NULL);
}

void _moddeinit()
{
	command_delete(&os_jupe, os_cmdtree);
	help_delentry(os_helptree, "JUPE");
}

static void os_cmd_jupe(char *origin)
{
	char *server = strtok(NULL, " ");
	char *reason = strtok(NULL, "");

	if (!server || !reason)
	{
		notice(opersvs.nick, origin, STR_INSUFFICIENT_PARAMS, "JUPE");
		notice(opersvs.nick, origin, "Usage: JUPE <server> <reason>");
		return;
	}

	if (!strchr(server, '.'))
	{
		notice(opersvs.nick, origin, "\2%s\2 is not a valid server name.", server);
		return;
	}

	if (!irccasecmp(server, me.name))
	{
		notice(opersvs.nick, origin, "\2%s\2 is the services server; it cannot be jupitered!", server);
		return;
	}

	if (!irccasecmp(server, curr_uplink->name))
	{
		notice(opersvs.nick, origin, "\2%s\2 is the current uplink; it cannot be jupitered!", server);
		return;
	}

	logcommand(opersvs.me, user_find_named(origin), CMDLOG_SET, "JUPE %s %s", server, reason);

	server_delete(server);
	jupe(server, reason);

	notice(opersvs.nick, origin, "\2%s\2 has been jupitered.", server);
}
