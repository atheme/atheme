/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains a generic help system implementation.
 *
 * $Id: help.c 5658 2006-07-02 05:10:39Z nenolod $
 */

#include "atheme.h"

helpentry_t *help_cmd_find(char *svs, char *origin, char *cmd, list_t *list)
{
	node_t *n;
	helpentry_t *c;

	LIST_FOREACH(n, list->head)
	{
		c = n->data;

		if (!strcasecmp(c->name, cmd))
			return c;
	}

	notice(svs, origin, "No help available for \2%s\2.", cmd);
	return NULL;
}

void help_display(char *svsnick, char *svsdisp, char *origin, char *command, list_t *list)
{
	helpentry_t *c;
	FILE *help_file;
	char buf[BUFSIZE];

	/* take the command through the hash table */
	if ((c = help_cmd_find(svsnick, origin, command, list)))
	{
		if (c->file)
		{
			if (*c->file == '/')
				help_file = fopen(c->file, "r");
			else
			{
				snprintf(buf, sizeof buf, "%s/%s", SHAREDIR, c->file);
				help_file = fopen(buf, "r");
			}

			if (!help_file)
			{
				notice(svsnick, origin, "Could not get help file for \2%s\2.", command);
				return;
			}

			notice(svsnick, origin, "***** \2%s Help\2 *****", svsnick);

			while (fgets(buf, BUFSIZE, help_file))
			{
				strip(buf);

				replace(buf, sizeof(buf), "&nick&", svsdisp);

				if (buf[0])
					notice(svsnick, origin, "%s", buf);
				else
					notice(svsnick, origin, " ");
			}

			fclose(help_file);

			notice(svsnick, origin, "***** \2End of Help\2 *****");
		}
		else if (c->func)
		{
			notice(svsnick, origin, "***** \2%s Help\2 *****", svsnick);

			c->func(origin);

			notice(svsnick, origin, "***** \2End of Help\2 *****");
		}
		else
			notice(svsnick, origin, "No help available for \2%s\2.", command);
	}
}

void help_addentry(list_t *list, char *topic, char *fname,
	void (*func)(char *origin))
{
	helpentry_t *he = smalloc(sizeof(helpentry_t));
	node_t *n;

	memset(he, 0, sizeof(helpentry_t));

	if (!list && !topic && !func && !fname)
	{
		slog(LG_DEBUG, "help_addentry(): invalid params");
		return;
	}

	/* further paranoia */
	if (!func && !fname)
	{
		slog(LG_DEBUG, "help_addentry(): invalid params");
		return;
	}

	he->name = sstrdup(topic);

	if (func != NULL)
		he->func = func;
	else if (fname != NULL)
		he->file = sstrdup(fname);

	n = node_create();

	node_add(he, n, list);
}

void help_delentry(list_t *list, char *name)
{
	node_t *n, *tn;
	helpentry_t *he;

	LIST_FOREACH_SAFE(n, tn, list->head)
	{
		he = n->data;

		if (!strcasecmp(he->name, name))
		{
			free(he->name);

			if (he->file != NULL)
				free(he->file);

			he->func = NULL;
			free(he);

			node_del(n, list);
		}
	}
}
