/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Commandtree manipulation routines.
 *
 * $Id: commandtree.c 6487 2006-09-26 15:32:49Z jilles $
 */

#include "atheme.h"

static int parse1(char *text, int maxparc, char **parv);

void command_add(command_t *cmd, list_t *commandtree)
{
	node_t *n;

	if ((n = node_find(cmd, commandtree)))
	{
		slog(LG_INFO, "command_add(): command %s already in the list", cmd->name);
		return;
	}

	n = node_create();
	node_add(cmd, n, commandtree);
}

/*
 * command_add_many()
 *
 * Inputs:
 *       array of commands to add, list to add them to.
 *
 * Output:
 *       none
 *
 * Side Effects:
 *       adds an array of commands to a command list,
 *       via command_add().
 */
void command_add_many(command_t **cmd, list_t *commandtree)
{
	uint32_t i;

	for (i = 0; cmd[i] != NULL; i++)
		command_add(cmd[i], commandtree);
}

void command_delete(command_t *cmd, list_t *commandtree)
{
	node_t *n;

	if (!(n = node_find(cmd, commandtree)))
	{
		slog(LG_INFO, "command_delete(): command %s was not registered.", cmd->name);
		return;
	}

	node_del(n, commandtree);
	node_free(n);
}

/*
 * command_delete_many()
 *
 * Inputs:
 *       array of commands to delete, list to delete them from.
 *
 * Output:
 *       none
 *
 * Side Effects:
 *       deletes an array of commands from a command list,
 *       via command_delete().
 */
void command_delete_many(command_t **cmd, list_t *commandtree)
{
	uint32_t i;

	for (i = 0; cmd[i] != NULL; i++)
		command_delete(cmd[i], commandtree);
}

command_t *command_find(list_t *commandtree, const char *command)
{
	node_t *n;

	LIST_FOREACH(n, commandtree->head)
	{
		command_t *c = n->data;

		if (!strcasecmp(command, c->name))
		{
			return c;
		}
	}
	return NULL;
}

void command_exec(service_t *svs, sourceinfo_t *si, command_t *c, int parc, char *parv[])
{
	if (has_priv(si->su, c->access))
	{
		c->cmd(si, parc, parv);
		return;
	}

	if (has_any_privs(si->su))
		command_fail(si, fault_noprivs, "You do not have %s privilege.", c->access);
	else
		command_fail(si, fault_noprivs, "You are not authorized to perform this operation.");
	/*snoop("DENIED CMD: \2%s\2 used %s %s", origin, svs->name, cmd);*/
}

void command_exec_split(service_t *svs, sourceinfo_t *si, char *cmd, char *text, list_t *commandtree)
{
	int parc, i;
	char *parv[20];
        command_t *c;

	if ((c = command_find(commandtree, cmd)))
	{
		parc = parse1(text, c->maxparc, parv);
		for (i = parc; i < sizeof(parv) / sizeof (parv[0]); i++)
			parv[i] = NULL;
		command_exec(svs, si, c, parc, parv);
	}
	else
	{
		notice(svs->name, si->su->nick, "Invalid command. Use \2/%s%s help\2 for a command listing.", (ircd->uses_rcommand == FALSE) ? "msg " : "", svs->disp);
	}
}

/*
 * command_help
 *     Iterates the command tree and lists available commands.
 *
 * inputs -
 *     mynick:      The nick of the services bot sending out the notices.
 *     origin:      The origin of the request.
 *     commandtree: The command tree being listed.
 * 
 * outputs -
 *     A list of available commands.
 */
void command_help(char *mynick, char *origin, list_t *commandtree)
{
	user_t *u = user_find_named(origin);
	node_t *n;

	notice(mynick, origin, "The following commands are available:");

	LIST_FOREACH(n, commandtree->head)
	{
		command_t *c = n->data;

		/* show only the commands we have access to
		 * (taken from command_exec())
		 */
		if (has_priv(u, c->access))
			notice(mynick, origin, "\2%-15s\2 %s", c->name, translation_get(c->desc));
	}
}

/* name1 name2 name3... */
static boolean_t string_in_list(const char *str, const char *name)
{
	char *p;
	int l;

	if (str == NULL)
		return FALSE;
	l = strlen(name);
	while (*str != '\0')
	{
		p = strchr(str, ' ');
		if (p != NULL ? p - str == l && !strncasecmp(str, name, p - str) : !strcasecmp(str, name))
			return TRUE;
		if (p == NULL)
			return FALSE;
		str = p;
		while (*str == ' ')
			str++;
	}
	return FALSE;
}

/*
 * command_help_short
 *     Iterates over the command tree and lists available commands.
 *
 * inputs -
 *     mynick:      The nick of the services bot sending out the notices.
 *     origin:      The origin of the request.
 *     commandtree: The command tree being listed.
 *     maincmds:    The commands to list verbosely.
 * 
 * outputs -
 *     A list of available commands.
 */
void command_help_short(char *mynick, char *origin, list_t *commandtree, char *maincmds)
{
	user_t *u = user_find_named(origin);
	node_t *n;
	unsigned int l;
	char buf[256];

	notice(mynick, origin, "The following commands are available:");

	LIST_FOREACH(n, commandtree->head)
	{
		command_t *c = n->data;

		/* show only the commands we have access to
		 * (taken from command_exec())
		 */
		if (string_in_list(maincmds, c->name) && has_priv(u, c->access))
			notice(mynick, origin, "\2%-15s\2 %s", c->name, translation_get(c->desc));
	}

	notice(mynick, origin, " ");
	strlcpy(buf, translation_get("Other commands: "), sizeof buf);
	l = strlen(buf);
	LIST_FOREACH(n, commandtree->head)
	{
		command_t *c = n->data;

		/* show only the commands we have access to
		 * (taken from command_exec())
		 */
		if (!string_in_list(maincmds, c->name) && has_priv(u, c->access))
		{
			if (strlen(buf) > l)
				strlcat(buf, ", ", sizeof buf);
			if (strlen(buf) > 55)
			{
				notice(mynick, origin, "%s", buf);
				buf[l] = '\0';
				while (--l > 0)
					buf[l] = ' ';
				buf[0] = ' ';
				l = strlen(buf);
			}
			strlcat(buf, c->name, sizeof buf);
		}
	}
	if (strlen(buf) > l)
		notice(mynick, origin, "%s", buf);
}

void fcommand_add(fcommand_t *cmd, list_t *commandtree)
{
	node_t *n;

	if ((n = node_find(cmd, commandtree)))
	{
		slog(LG_INFO, "fcommand_add(): command %s already in the list", cmd->name);
		return;
	}

	n = node_create();
	node_add(cmd, n, commandtree);
}

void fcommand_delete(fcommand_t *cmd, list_t *commandtree)
{
	node_t *n;

	if (!(n = node_find(cmd, commandtree)))
	{
		slog(LG_INFO, "command_delete(): command %s was not registered.", cmd->name);
		return;
	}

	node_del(n, commandtree);
	node_free(n);
}

void fcommand_exec(service_t *svs, char *channel, char *origin, char *cmd, list_t *commandtree)
{
	node_t *n;

	LIST_FOREACH(n, commandtree->head)
	{
		fcommand_t *c = n->data;

		if (!strcasecmp(cmd, c->name))
		{
			user_t *u = user_find_named(origin);

			if (has_priv(u, c->access))
			{
				c->cmd(origin, channel);
				return;
			}
			if (has_any_privs(u))
				notice(svs->name, origin, "You do not have %s privilege.", c->access);
			else
				notice(svs->name, origin, "You are not authorized to perform this operation.");
			return;
		}
	}

	if (!channel || *channel != '#')
		notice(svs->name, origin, "Invalid command. Use \2/%s%s help\2 for a command listing.", (ircd->uses_rcommand == FALSE) ? "msg " : "", svs->disp);
}

void fcommand_exec_floodcheck(service_t *svs, char *channel, char *origin, char *cmd, list_t *commandtree)
{
	node_t *n;
	user_t *u = user_find_named(origin); /* Yeah, silly */

	LIST_FOREACH(n, commandtree->head)
	{
		fcommand_t *c = n->data;

		if (!strcasecmp(cmd, c->name))
		{
			/* run it through floodcheck which also checks for services ignore */
			if (floodcheck(u, NULL))
				return;

			if (has_priv(u, c->access))
			{
				c->cmd(origin, channel);
				return;
			}
			if (has_any_privs(u))
				notice(svs->name, origin, "You do not have %s privilege.", c->access);
			else
				notice(svs->name, origin, "You are not authorized to perform this operation.");
			return;
		}
	}

	if (!channel || *channel != '#')
	{
		if (floodcheck(u, NULL))
			return;
		notice(svs->name, origin, "Invalid command. Use \2/%s%s help\2 for a command listing.", (ircd->uses_rcommand == FALSE) ? "msg " : "", svs->disp);
	}
}

static int parse1(char *text, int maxparc, char **parv)
{
	int count = 0;
	char *p;

        if (maxparc == 0)
        	return 0;

	if (!text)
		return 0;

	p = text;
	while (count < maxparc - 1 && (parv[count] = strtok(p, " ")) != NULL)
		count++, p = NULL;

	if ((parv[count] = strtok(p, "")) != NULL)
	{
		p = parv[count];
		if (*p != '\0')
		{
			p += strlen(p) - 1;
			while (*p == ' ' && p > parv[count])
				p--;
			p[1] = '\0';
		}
		count++;
	}
	return count;
}
