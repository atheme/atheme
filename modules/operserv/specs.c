/*
 * Copyright (c) 2005 Patrick Fish, et al
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains functionality which implements the OService SPECS command.
 *
 * $Id: specs.c 4357 2005-12-30 14:05:28Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/specs", FALSE, _modinit, _moddeinit,
	"$Id: specs.c 4357 2005-12-30 14:05:28Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void os_cmd_specs(char *origin);

command_t os_specs = { "SPECS", "Shows oper flags.",
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
	user_t *u = user_find(origin), *tu;
	char *targettype = strtok(NULL, " ");
	char *target = strtok(NULL, " ");
	char nprivs[BUFSIZE], cprivs[BUFSIZE], gprivs[BUFSIZE], oprivs[BUFSIZE];

	if (!has_any_privs(u))
	{
		notice(opersvs.nick, origin, "You are not authorized to use %s.", opersvs.nick);
		return;
	}

	if (targettype != NULL)
	{
		if (!has_priv(u, PRIV_VIEWPRIVS))
		{
			notice(opersvs.nick, origin, "You do not have %s privilege.", PRIV_VIEWPRIVS);
			return;
		}
		if (strcasecmp(targettype, "USER"))
		{
			notice(opersvs.nick, origin, "Only the USER type is currently supported.");
			return;
		}
		if (target == NULL)
			target = "?";
		tu = user_find_named(target);
		if (tu == NULL)
		{
			notice(opersvs.nick, origin, "\2%s\2 is not on IRC.", target);
			return;
		}
		if (!has_any_privs(tu))
		{
			notice(opersvs.nick, origin, "\2%s\2 is unprivileged.", tu->nick);
			return;
		}
		if (is_internal_client(tu))
		{
			notice(opersvs.nick, origin, "\2%s\2 is an internal client.", tu->nick);
			return;
		}
	}
	else
		tu = u;

	/* NickServ/UserServ */

	*nprivs = '\0';

	if (has_priv(tu, PRIV_USER_AUSPEX))
		strcat(nprivs, "view concealed information");

	if (has_priv(tu, PRIV_USER_ADMIN))
	{
		if (*nprivs)
			strcat(nprivs, ", ");

		strcpy(nprivs, "drop accounts, freeze accounts, reset passswords, send passwords");
	}

	if (has_priv(tu, PRIV_USER_VHOST))
	{
		if (*nprivs)
			strcat(nprivs, ", ");

		strcat(nprivs, "set vhosts");
	}

	/* ChanServ */
	*cprivs = '\0';

	if (has_priv(tu, PRIV_CHAN_AUSPEX))
		strcpy(cprivs, "view concealed information");

	if (has_priv(tu, PRIV_CHAN_ADMIN))
	{
		if (*cprivs)
			strcat(cprivs, ", ");

		strcat(cprivs, "drop channels, close channels, transfer ownership");
	}

	if (has_priv(tu, PRIV_CHAN_CMODES))
	{
		if (*cprivs)
			strcat(cprivs, ", ");

		strcat(cprivs, "mlock operator modes");
	}

	if (has_priv(tu, PRIV_JOIN_STAFFONLY))
	{
		if (*cprivs)
			strcat(cprivs, ", ");

		strcat(cprivs, "join staff channels");
	}

	/* NickServ/UserServ+ChanServ */

	if (has_priv(tu, PRIV_MARK))
	{
		if (*nprivs)
			strcat(nprivs, ", ");
		if (*cprivs)
			strcat(cprivs, ", ");

		strcat(nprivs, "mark accounts");
		strcat(cprivs, "mark channels");
	}

	if (has_priv(tu, PRIV_HOLD))
	{
		if (*nprivs)
			strcat(nprivs, ", ");
		if (*cprivs)
			strcat(cprivs, ", ");

		strcat(nprivs, "hold accounts");
		strcat(cprivs, "hold channels");
	}

	/* general */
	*gprivs = '\0';

	if (has_priv(tu, PRIV_SERVER_AUSPEX))
		strcat(gprivs, "view concealed information");

	if (has_priv(tu, PRIV_VIEWPRIVS))
	{
		if (*gprivs)
			strcat(gprivs, ", ");

		strcat(gprivs, "view privileges of other users");
	}

	if (has_priv(tu, PRIV_FLOOD))
	{
		if (*gprivs)
			strcat(gprivs, ", ");

		strcat(gprivs, "exempt from flood control");
	}

	if (has_priv(tu, PRIV_METADATA))
	{
		if (*gprivs)
			strcat(gprivs, ", ");

		strcat(gprivs, "edit private metadata");
	}

	if (has_priv(tu, PRIV_ADMIN))
	{
		if (*gprivs)
			strcat(gprivs, ", ");

		strcat(gprivs, "administer services");
	}

	/* OperServ */

	*oprivs = '\0';

	if (has_priv(tu, PRIV_OMODE))
		strcpy(oprivs, "set channel modes");

	if (has_priv(tu, PRIV_AKILL))
	{
		if (*oprivs)
			strcat(oprivs, ", ");

		strcat(oprivs, "add and remove autokills");
	}

	if (has_priv(tu, PRIV_JUPE))
	{
		if (*oprivs)
			strcat(oprivs, ", ");

		strcat(oprivs, "jupe servers");
	}

	if (has_priv(tu, PRIV_NOOP))
	{
		if (*oprivs)
			strcat(oprivs, ", ");

		strcat(oprivs, "NOOP access");
	}

	if (has_priv(tu, PRIV_GLOBAL))
	{
		if (*oprivs)
			strcat(oprivs, ", ");

		strcat(oprivs, "send global notices");
	}

	notice(opersvs.nick, origin, "Privileges for \2%s\2:", tu->nick);
	if (*nprivs)
		notice(opersvs.nick, origin, "\2Nicknames/accounts\2: %s", nprivs);
	if (*cprivs)
		notice(opersvs.nick, origin, "\2Channels\2: %s", cprivs);
	if (*gprivs)
		notice(opersvs.nick, origin, "\2General\2: %s", gprivs);
	if (*oprivs)
		notice(opersvs.nick, origin, "\2OperServ\2: %s", oprivs);
	notice(opersvs.nick, origin, "End of privileges");

	logcommand(opersvs.me, user_find(origin), CMDLOG_ADMIN, "SPECS %s!%s@%s", tu->nick, tu->user, tu->vhost);
}
