/* * Copyright (c) 2005 Patrick Fish
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains functionality which implements the OService SPECS command.
 *
 * $Id: specs.c 4347 2005-12-30 08:45:01Z pfish $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/specs", FALSE, _modinit, _moddeinit,
	"$Id: specs.c 4347 2005-12-30 08:45:01Z pfish $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void os_cmd_specs(char *origin);

command_t os_specs = { "SPECS", "Shows your oper flags.",
                        AC_NONE, os_cmd_specs };

list_t *os_cmdtree;
list_t *os_helptree;

void _modinit(module_t *m)
{
	os_cmdtree = module_locate_symbol("operserv/main", "os_cmdtree");
	os_helptree = module_locate_symbol("operserv/main", "os_helptree");

        command_add(&os_specs, os_cmdtree);
	help_addentry(os_helptree, "SPECS", "help/oservice/SPECS", NULL);
}

void _moddeinit()
{
	command_delete(&os_specs, os_cmdtree);
	help_delentry(os_helptree, "SPECS");
}

static void os_cmd_specs(char *origin)
{
	user_t *u = user_find(origin);
	char nprivs[BUFSIZE], cprivs[BUFSIZE], oprivs[BUFSIZE];

	if (!has_any_privs(u))
	{
		notice(opersvs.nick, origin, "You are not authorized to use %s.", opersvs.nick);
		return;
	}

	/* NickServ/UserServ */

	*nprivs = '\0';

	if (has_priv(u, PRIV_USER_AUSPEX))
	{
		strcat(nprivs, "view concealed information");
	}

	if (has_priv(u, PRIV_USER_ADMIN))
	{
		if (*nprivs)
			strcat(nprivs, ", ");

		strcpy(nprivs, "drop accounts, freeze accounts, reset passswords, send passwords");
	}

	if (has_priv(u, PRIV_USER_VHOST))
	{
		if (*nprivs)
			strcat(nprivs, ", ");

		strcat(nprivs, "set vhosts");
	}

	/* ChanServ */
	*cprivs = '\0';

	if (has_priv(u, PRIV_CHAN_AUSPEX))
	{
		strcpy(cprivs, "view concealed information");
	}

	if (has_priv(u, PRIV_CHAN_ADMIN))
	{
		if (*cprivs)
			strcat(cprivs, ", ");

		strcat(cprivs, "drop channels, close channels, transfer ownership");
	}

	if (has_priv(u, PRIV_CHAN_CMODES))
	{
		if (*cprivs)
			strcat(cprivs, ", ");

		strcat(cprivs, "mlock operator modes");
	}

	if (has_priv(u, PRIV_JOIN_STAFFONLY))
	{
		if (*cprivs)
			strcat(cprivs, ", ");

		strcat(cprivs, "join staff channels");
	}

	/* NickServ/UserServ+ChanServ */

	if (has_priv(u, PRIV_MARK))
	{
		if (*nprivs)
			strcat(nprivs, ", ");
		if (*cprivs)
			strcat(cprivs, ", ");

		strcat(nprivs, "mark accounts");
		strcat(cprivs, "mark channels");
	}

	if (has_priv(u, PRIV_HOLD))
	{
		if (*nprivs)
			strcat(nprivs, ", ");
		if (*cprivs)
			strcat(cprivs, ", ");

		strcat(nprivs, "hold accounts");
		strcat(cprivs, "hold channels");
	}

	

	/* OperServ */

	*oprivs = '\0';

	if (has_priv(u, PRIV_OMODE))
		strcpy(oprivs, "set channel modes");

	if (has_priv(u, PRIV_AKILL))
	{
		if (*oprivs)
			strcat(oprivs, ", ");

		strcat(oprivs, "add and remove autokills");
	}

	if (has_priv(u, PRIV_JUPE))
	{
		if (*oprivs)
			strcat(oprivs, ", ");

		strcat(oprivs, "jupe servers");
	}

	if (has_priv(u, PRIV_NOOP))
	{
		if (*oprivs)
			strcat(oprivs, ", ");

		strcat(oprivs, "NOOP access");
	}

	if (has_priv(u, PRIV_GLOBAL))
	{
		if (*oprivs)
			strcat(oprivs, ", ");

		strcat(oprivs, "send global notices");
	}

	notice(opersvs.nick, origin, "\2Nicknames/accounts\2: %s", nprivs);
	notice(opersvs.nick, origin, "\2Channels\2: %s", cprivs);
	notice(opersvs.nick, origin, "\2OperServ\2: %s", oprivs);

	snoop("%s: SPECS", origin);
	logcommand(opersvs.me, user_find(origin), CMDLOG_ADMIN, "SPECS");
}
