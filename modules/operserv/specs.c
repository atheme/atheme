/*
 * Copyright (c) 2005-2006 Patrick Fish, et al
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains functionality which implements the OService SPECS command.
 *
 * $Id: specs.c 5686 2006-07-03 16:25:03Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/specs", FALSE, _modinit, _moddeinit,
	"$Id: specs.c 5686 2006-07-03 16:25:03Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void os_cmd_specs(char *origin);

command_t os_specs = { "SPECS", "Shows oper flags.",
                        AC_NONE, os_cmd_specs };

list_t *os_cmdtree;
list_t *os_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(os_cmdtree, "operserv/main", "os_cmdtree");
	MODULE_USE_SYMBOL(os_helptree, "operserv/main", "os_helptree");

        command_add(&os_specs, os_cmdtree);
	help_addentry(os_helptree, "SPECS", "help/oservice/specs", NULL);
}

void _moddeinit()
{
	command_delete(&os_specs, os_cmdtree);
	help_delentry(os_helptree, "SPECS");
}

struct
{
	char *priv;
	char *npriv, *cpriv, *gpriv, *opriv;
} privnames[] =
{
	/* NickServ/UserServ */
	{ PRIV_USER_AUSPEX, "view concealed information", NULL, NULL, NULL },
	{ PRIV_USER_ADMIN, "drop accounts, freeze accounts, reset passwords, send passwords", NULL, NULL, NULL },
	{ PRIV_USER_VHOST, "set vhosts", NULL, NULL, NULL },
	/* ChanServ */
	{ PRIV_CHAN_AUSPEX, NULL, "view concealed information", NULL, NULL },
	{ PRIV_CHAN_ADMIN, NULL, "drop channels, close channels, transfer ownership", NULL, NULL },
	{ PRIV_CHAN_CMODES, NULL, "mlock operator modes", NULL, NULL },
	{ PRIV_JOIN_STAFFONLY, NULL, "join staff channels", NULL, NULL },
	/* NickServ/UserServ+ChanServ */
	{ PRIV_MARK, "mark accounts", "mark channels", NULL, NULL },
	{ PRIV_HOLD, "hold accounts", "hold channels", NULL, NULL },
	{ PRIV_REG_NOLIMIT, NULL, "bypass registration limits", NULL, NULL },
	/* general */
	{ PRIV_SERVER_AUSPEX, NULL, NULL, "view concealed information", NULL },
	{ PRIV_VIEWPRIVS, NULL, NULL, "view privileges of other users", NULL },
	{ PRIV_FLOOD, NULL, NULL, "exempt from flood control", NULL },
	{ PRIV_ADMIN, NULL, NULL, "administer services", NULL },
	{ PRIV_METADATA, NULL, NULL, "edit private metadata", NULL },
	/* OperServ */
	{ PRIV_OMODE, NULL, NULL, NULL, "set channel modes" },
	{ PRIV_AKILL, NULL, NULL, NULL, "add and remove autokills" },
	{ PRIV_JUPE, NULL, NULL, NULL, "jupe servers" },
	{ PRIV_NOOP, NULL, NULL, NULL, "NOOP access" },
	{ PRIV_GLOBAL, NULL, NULL, NULL, "send global notices" },
	/* -- */
	{ NULL, NULL, NULL, NULL, NULL }
};

static void os_cmd_specs(char *origin)
{
	user_t *u = user_find_named(origin), *tu = NULL;
	operclass_t *cl = NULL;
	char *targettype = strtok(NULL, " ");
	char *target = strtok(NULL, " ");
	char nprivs[BUFSIZE], cprivs[BUFSIZE], gprivs[BUFSIZE], oprivs[BUFSIZE];
	int i;

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
		if (target == NULL)
			target = "?";
		if (!strcasecmp(targettype, "USER"))
		{
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
		else if (!strcasecmp(targettype, "OPERCLASS") || !strcasecmp(targettype, "CLASS"))
		{
			cl = operclass_find(target);
			if (cl == NULL)
			{
				notice(opersvs.nick, origin, "No such oper class \2%s\2.", target);
				return;
			}
		}
		else
		{
			notice(opersvs.nick, origin, "Valid target types: USER, OPERCLASS.");
			return;
		}
	}
	else
		tu = u;

	i = 0;
	*nprivs = *cprivs = *gprivs = *oprivs = '\0';
	while (privnames[i].priv != NULL)
	{
		if (tu ? has_priv(tu, privnames[i].priv) : has_priv_operclass(cl, privnames[i].priv))
		{
			if (privnames[i].npriv != NULL)
			{
				if (*nprivs)
					strcat(nprivs, ", ");
				strcat(nprivs, privnames[i].npriv);
			}
			if (privnames[i].cpriv != NULL)
			{
				if (*cprivs)
					strcat(cprivs, ", ");
				strcat(cprivs, privnames[i].cpriv);
			}
			if (privnames[i].gpriv != NULL)
			{
				if (*gprivs)
					strcat(gprivs, ", ");
				strcat(gprivs, privnames[i].gpriv);
			}
			if (privnames[i].opriv != NULL)
			{
				if (*oprivs)
					strcat(oprivs, ", ");
				strcat(oprivs, privnames[i].opriv);
			}
		}
		i++;
	}

	if (tu)
		notice(opersvs.nick, origin, "Privileges for \2%s\2:", tu->nick);
	else
		notice(opersvs.nick, origin, "Privileges for oper class \2%s\2:", cl->name);

	if (*nprivs)
		notice(opersvs.nick, origin, "\2Nicknames/accounts\2: %s", nprivs);
	if (*cprivs)
		notice(opersvs.nick, origin, "\2Channels\2: %s", cprivs);
	if (*gprivs)
		notice(opersvs.nick, origin, "\2General\2: %s", gprivs);
	if (*oprivs)
		notice(opersvs.nick, origin, "\2OperServ\2: %s", oprivs);
	notice(opersvs.nick, origin, "End of privileges");

	if (tu)
		logcommand(opersvs.me, user_find_named(origin), CMDLOG_ADMIN, "SPECS USER %s!%s@%s", tu->nick, tu->user, tu->vhost);
	else
		logcommand(opersvs.me, user_find_named(origin), CMDLOG_ADMIN, "SPECS OPERCLASS %s", cl->name);
}
