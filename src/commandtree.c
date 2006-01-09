/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Commandtree manipulation routines.
 *
 * $Id: commandtree.c 4549 2006-01-09 23:27:17Z jilles $
 */

#include "atheme.h"

void command_add(command_t * cmd, list_t *commandtree)
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
void command_add_many(command_t ** cmd, list_t *commandtree)
{
	command_t *cptr;

	for (cptr = cmd[0]; cptr; cptr++)
		command_add(cptr, commandtree);
}

void command_delete(command_t * cmd, list_t *commandtree)
{
	node_t *n;

	if (!(n = node_find(cmd, commandtree)))
	{
		slog(LG_INFO, "command_delete(): command %s was not registered.", cmd->name);
		return;
	}

	node_del(n, commandtree);
}

void command_exec(service_t *svs, char *origin, char *cmd, list_t *commandtree)
{
	node_t *n;

	LIST_FOREACH(n, commandtree->head)
	{
		command_t *c = n->data;

		if (!strcasecmp(cmd, c->name))
		{
			user_t *u = user_find(origin);

			if (has_priv(u, c->access))
			{
				c->cmd(origin);
				return;
			}

			if (has_any_privs(u))
				notice(svs->name, origin, "You do not have %s privilege.", c->access);
			else
				notice(svs->name, origin, "You are not authorized to perform this operation.");
			/*snoop("DENIED CMD: \2%s\2 used %s %s", origin, svs->name, cmd);*/
			return;
		}
	}

	notice(svs->name, origin, "Invalid command. Use \2/%s%s help\2 for a command listing.", (ircd->uses_rcommand == FALSE) ? "msg " : "", svs->disp);
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
	user_t *u = user_find(origin);
	node_t *n;

	notice(mynick, origin, "The following commands are available:");

	LIST_FOREACH(n, commandtree->head)
	{
		command_t *c = n->data;

		/* show only the commands we have access to
		 * (taken from command_exec())
		 */
		if (has_priv(u, c->access))
			notice(mynick, origin, "\2%-16s\2 %s", c->name, c->desc);
	}
}

void fcommand_add(fcommand_t * cmd, list_t *commandtree)
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

void fcommand_delete(fcommand_t * cmd, list_t *commandtree)
{
	node_t *n;

	if (!(n = node_find(cmd, commandtree)))
	{
		slog(LG_INFO, "command_delete(): command %s was not registered.", cmd->name);
		return;
	}

	node_del(n, commandtree);
}

void fcommand_exec(service_t *svs, char *channel, char *origin, char *cmd, list_t *commandtree)
{
	node_t *n;

	LIST_FOREACH(n, commandtree->head)
	{
		fcommand_t *c = n->data;

		if (!strcasecmp(cmd, c->name))
		{
			user_t *u = user_find(origin);

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
