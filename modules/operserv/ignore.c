/*
 * Copyright (c) 2006 Patrick Fish
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains functionality which implements the OService IGNORE command.
 *
 * $Id: ignore.c 5686 2006-07-03 16:25:03Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/ignore", FALSE, _modinit, _moddeinit,
	"$Id: ignore.c 5686 2006-07-03 16:25:03Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void os_cmd_ignore(char *origin);
static void os_cmd_ignore_add(char *origin, char *target);
static void os_cmd_ignore_del(char *origin, char *target);
static void os_cmd_ignore_list(char *origin, char *arg);
static void os_cmd_ignore_clear(char *origin, char *arg);

command_t os_ignore = { "IGNORE", "Ignore a mask from services.",
                        PRIV_ADMIN, os_cmd_ignore };
fcommand_t os_ignore_add = { "ADD", PRIV_ADMIN, os_cmd_ignore_add };
fcommand_t os_ignore_del = { "DEL", PRIV_ADMIN, os_cmd_ignore_del };
fcommand_t os_ignore_list = { "LIST", PRIV_ADMIN, os_cmd_ignore_list };
fcommand_t os_ignore_clear = { "CLEAR", PRIV_ADMIN, os_cmd_ignore_clear };

list_t *os_cmdtree;
list_t *os_helptree;
list_t os_ignore_cmds;
list_t svs_ignore_list;


void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(os_cmdtree, "operserv/main", "os_cmdtree");
	MODULE_USE_SYMBOL(os_helptree, "operserv/main", "os_helptree");

        command_add(&os_ignore, os_cmdtree);
	help_addentry(os_helptree, "IGNORE", "help/oservice/ignore", NULL);

	/* Sub-commands */
	fcommand_add(&os_ignore_add, &os_ignore_cmds);
	fcommand_add(&os_ignore_del, &os_ignore_cmds);
	fcommand_add(&os_ignore_clear, &os_ignore_cmds);
	fcommand_add(&os_ignore_list, &os_ignore_cmds);
}

void _moddeinit()
{
	command_delete(&os_ignore, os_cmdtree);
	help_delentry(os_helptree, "IGNORE");

	/* Sub-commands */
	fcommand_delete(&os_ignore_add, &os_ignore_cmds);
	fcommand_delete(&os_ignore_del, &os_ignore_cmds);
	fcommand_delete(&os_ignore_list, &os_ignore_cmds);
	fcommand_delete(&os_ignore_clear, &os_ignore_cmds);

}

static void os_cmd_ignore(char *origin)
{
	user_t *u = user_find_named(origin);
	char *cmd = strtok(NULL, " ");
	char *arg = strtok(NULL, " ");

	if (!cmd)
	{
		notice(opersvs.nick, origin, STR_INSUFFICIENT_PARAMS, "IGNORE");
		notice(opersvs.nick, origin, "Syntax: IGNORE ADD|DEL|LIST|CLEAR <mask>");
		return;
	}

	fcommand_exec(opersvs.me, arg, origin, cmd, &os_ignore_cmds);

}

static void os_cmd_ignore_add(char *origin, char *target)
{
	user_t *u = user_find_named(origin);
	node_t *n,*node;
	char *reason = strtok(NULL, "");
	svsignore_t *svsignore, *s;

	if (target == NULL)
	{
		notice(opersvs.nick, origin, STR_INSUFFICIENT_PARAMS, "IGNORE");
		notice(opersvs.nick, origin, "Syntax: IGNORE ADD|DEL|LIST|CLEAR <mask>");
		return;
	}

	if (!validhostmask(target))
	{
		notice(opersvs.nick, origin, "Invalid host mask, %s", target);
		return;
	}

	/* Are we already ignoring this mask? */
	LIST_FOREACH(n, svs_ignore_list.head)
	{
		svsignore = (svsignore_t *)n->data;

		/* We're here */
		if (!strcasecmp(svsignore->mask, target))
		{
			notice(opersvs.nick, origin, "The mask \2%s\2 already exists on the services ignore list.", svsignore->mask);
			return;
		}
	}

	svsignore = svsignore_add(target, reason);
	svsignore->setby = sstrdup(origin);
	svsignore->settime = CURRTIME;

	notice(opersvs.nick, origin, "\2%s\2 has been added to the services ignore list.", target);

	logcommand(opersvs.me, u, CMDLOG_ADMIN, "IGNORE ADD %s");
	wallops("%s added a services ignore for \2%s\2.", origin, target);

	return;
}

static void os_cmd_ignore_del(char *origin, char *target)
{
	user_t *u = user_find_named(origin);
	node_t *n, *tn;
	svsignore_t *svsignore;

	if (target == NULL)
	{
		notice(opersvs.nick, origin, STR_INSUFFICIENT_PARAMS, "IGNORE");
		notice(opersvs.nick, origin, "Syntax: IGNORE ADD|DEL|LIST|CLEAR <mask>");
		return;
	}

	LIST_FOREACH_SAFE(n, tn, svs_ignore_list.head)
	{
		svsignore = (svsignore_t *)n->data;

		if (!strcasecmp(svsignore->mask,target))
		{
			notice(opersvs.nick, origin, "\2%s\2 has been removed from the services ignore list.", svsignore->mask);

			node_del(n,&svs_ignore_list);
			node_free(n);
			free(svsignore->mask);
			free(svsignore->setby);
			free(svsignore->reason);

			wallops("%s removed \2%s\2 from the services ignore list.", origin, target);
			logcommand(opersvs.me, u, CMDLOG_ADMIN, "IGNORE DEL %s", target);

			return;
		}
	}

	notice(opersvs.nick, origin, "\2%s\2 was not found on the services ignore list.", target);
	return;
}

static void os_cmd_ignore_clear(char *origin, char *arg)
{
	user_t *u = user_find_named(origin);
	node_t *n, *tn;
	svsignore_t *svsignore;

	if (LIST_LENGTH(&svs_ignore_list) == 0)
	{
		notice(opersvs.nick, origin, "Services ignore list is empty.");
		return;
	}

	LIST_FOREACH_SAFE(n, tn, svs_ignore_list.head)
	{
		svsignore = (svsignore_t *)n->data;

		notice(opersvs.nick, origin, "\2%s\2 has been removed from the services ignore list.", svsignore->mask);
		node_del(n,&svs_ignore_list);
		node_free(n);
		free(svsignore->mask);
		free(svsignore->setby);
		free(svsignore->reason);

	}

	notice(opersvs.nick, origin, "Services ignore list has been wiped!");

	wallops("\2%s\2 wiped the services ignore list.", origin);
	logcommand(opersvs.me, u, CMDLOG_ADMIN, "IGNORE CLEAR");

	return;
}


static void os_cmd_ignore_list(char *origin, char *arg)
{
	user_t *u = user_find_named(origin);
	node_t *n;
	uint8_t i = 1;
	svsignore_t *svsignore;
	char strfbuf[32];
	struct tm tm;

	if (LIST_LENGTH(&svs_ignore_list) == 0)
	{
		notice(opersvs.nick, origin, "The services ignore list is empty.");
		return;
	}

	notice(opersvs.nick, origin, "Current Ignore list entries:");
	notice(opersvs.nick, origin, "-------------------------");

	LIST_FOREACH(n, svs_ignore_list.head)
	{
		svsignore = (svsignore_t *)n->data;

		tm = *localtime(&svsignore->settime);
		strftime(strfbuf, sizeof(strfbuf) - 1, "%b %d %H:%M:%S %Y", &tm);

		notice(opersvs.nick, origin, "%d: %s by %s on %s (Reason: %s)", i, svsignore->mask, svsignore->setby, strfbuf, svsignore->reason);
		i++;
	}

	notice(opersvs.nick, origin, "-------------------------");

	logcommand(opersvs.me, u, CMDLOG_ADMIN, "IGNORE LIST");

	return;
}
