/*
 * Copyright (c) 2005-2006 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains a generic help system implementation.
 *
 * $Id: help.c 7771 2007-03-03 12:46:36Z pippijn $
 */

#include "atheme.h"

static helpentry_t *help_cmd_find(sourceinfo_t *si, char *cmd, list_t *list)
{
	node_t *n;
	helpentry_t *c;

	LIST_FOREACH(n, list->head)
	{
		c = n->data;

		if (!strcasecmp(c->name, cmd))
			return c;
	}

	command_fail(si, fault_nosuch_target, "No help available for \2%s\2.", cmd);
	return NULL;
}

void help_display(sourceinfo_t *si, char *command, list_t *list)
{
	helpentry_t *c;
	FILE *help_file;
	char buf[BUFSIZE];

	/* take the command through the hash table */
	if ((c = help_cmd_find(si, command, list)))
	{
		if (c->file)
		{
			if (*c->file == '/')
				help_file = fopen(c->file, "r");
			else
			{
				snprintf(buf, sizeof buf, "%s/%s", SHAREDIR, c->file);
				if (nicksvs.no_nick_ownership && !strncmp(c->file, "help/nickserv/", 14))
					memcpy(buf + (sizeof(SHAREDIR) - 1) + 6, "userserv", 8);
				help_file = fopen(buf, "r");
			}

			if (!help_file)
			{
				command_fail(si, fault_nosuch_target, "Could not get help file for \2%s\2.", command);
				return;
			}

			command_success_nodata(si, "***** \2%s Help\2 *****", si->service->name);

			while (fgets(buf, BUFSIZE, help_file))
			{
				strip(buf);

				replace(buf, sizeof(buf), "&nick&", si->service->disp);

				if (buf[0])
					command_success_nodata(si, "%s", buf);
				else
					command_success_nodata(si, " ");
			}

			fclose(help_file);

			command_success_nodata(si, "***** \2End of Help\2 *****");
		}
		else if (c->func)
		{
			command_success_nodata(si, "***** \2%s Help\2 *****", si->service->name);

			c->func(si);

			command_success_nodata(si, "***** \2End of Help\2 *****");
		}
		else
			command_fail(si, fault_nosuch_target, "No help available for \2%s\2.", command);
	}
}

void help_addentry(list_t *list, char *topic, char *fname,
	void (*func)(sourceinfo_t *si))
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
			node_free(n);
		}
	}
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:noexpandtab
 */
