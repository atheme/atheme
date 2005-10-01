/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Commandtree manipulation routines.
 *
 * $Id: commandtree.c 2497 2005-10-01 04:35:25Z nenolod $
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

void command_exec(char *mynick, char *origin, char *cmd, list_t *commandtree)
{
	node_t *n;

	LIST_FOREACH(n, commandtree->head)
	{
		command_t *c = n->data;

		if (!strcasecmp(cmd, c->name))
		{
			user_t *u = user_find(origin);

			if (c->access == AC_NONE)
			{
				c->cmd(origin);
				return;
			}

			if ((c->access == AC_SRA) && (is_sra(u->myuser)))
			{
				c->cmd(origin);
				return;
			}

			if ((c->access == AC_IRCOP) && (is_sra(u->myuser) || (is_ircop(u))))
			{
				c->cmd(origin);
				return;
			}

			notice(mynick, origin, "You are not authorized to perform this operation.");
			snoop("DENIED CMD: \2%s\2 used %s", origin, cmd);
			return;
		}
	}

	notice(mynick, origin, "Invalid command. Use \2/%s%s help\2 for a command listing.", (ircd->uses_rcommand == FALSE) ? "msg " : "", mynick);
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
		if ((c->access == AC_NONE) || ((c->access == AC_SRA) && is_sra(u->myuser)) || ((c->access == AC_IRCOP) && (is_sra(u->myuser) || is_ircop(u))))
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

void fcommand_exec(char *channel, char *origin, char *cmd, list_t *commandtree)
{
	node_t *n;

	LIST_FOREACH(n, commandtree->head)
	{
		fcommand_t *c = n->data;

		if (!strcasecmp(cmd, c->name))
		{
			user_t *u = user_find(origin);

			if (c->access == AC_NONE)
			{
				c->cmd(origin, channel);
				return;
			}

			if ((c->access == AC_SRA) && (is_sra(u->myuser)))
			{
				c->cmd(origin, channel);
				return;
			}

			if ((c->access == AC_IRCOP) && (is_sra(u->myuser) || (is_ircop(u))))
			{
				c->cmd(origin, channel);
				return;
			}

			notice(chansvs.nick, origin, "You are not authorized to perform this operation.");
			return;
		}
	}
}
