/*
 * Copyright (c) 2005 Atheme Development Group
 *
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains a generic help system implementation.
 *
 * $Id: help.c 2541 2005-10-04 05:06:05Z nenolod $
 */

#include "atheme.h"

helpentry_t *help_cmd_find(char *svs, char *origin, char *cmd, list_t *list)
{
	user_t *u = user_find(origin);
	node_t *n;
	helpentry_t *c;

	LIST_FOREACH(n, list->head)
	{
		c = n->data;

		if (!strcasecmp(c->name, cmd))
		{
			if (c->access == AC_NONE)
				return c;

			if (c->access == AC_SRA && is_sra(u->myuser))
				return c;

			if ((c->access == AC_IRCOP) && (is_sra(u->myuser) || (is_ircop(u))))
				return c;
			else
			{
				notice(svs, origin, "You are not authorized to perform this operation.");
				return NULL;
			}
		}
	}

	notice(svs, origin, "No help available for \2%s\2.", cmd);
	return NULL;
}

void help_addentry(list_t *list, char *topic, char *fname,
	void (*func)(char *origin))
{
	helpentry_t *he = smalloc(sizeof(helpentry_t));
	node_t *n;

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
