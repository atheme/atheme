/*
 * Copyright (c) 2006 Robin Burchell <surreal.w00t@gmail.com>
 * Rights to this code are documented in doc/LICENCE.
 *
 * This file contains functionality implementing OperServ RNC.
 *
 * $Id: rnc.c 6119 2006-08-18 11:41:46Z w00t $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/rnc", FALSE, _modinit, _moddeinit,
	"$Id: rnc.c 6119 2006-08-18 11:41:46Z w00t $",
	"Robin Burchell <surreal.w00t@gmail.com>"
);

static void os_cmd_rnc(char *origin);

command_t os_rnc = { "RNC", "Shows the most frequent realnames on the network", PRIV_SERVER_AUSPEX, os_cmd_rnc };

list_t *os_cmdtree;
list_t *os_helptree;

typedef struct rnc_t_ rnc_t;
struct rnc_t_
{
	char gecos[GECOSLEN];
	int count;
};

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(os_cmdtree, "operserv/main", "os_cmdtree");
	MODULE_USE_SYMBOL(os_helptree, "operserv/main", "os_helptree");

	command_add(&os_rnc, os_cmdtree);
	help_addentry(os_helptree, "RNC", "help/oservice/rnc", NULL);
}

void _moddeinit(void)
{
	command_delete(&os_rnc, os_cmdtree);
	help_delentry(os_helptree, "RNC");
}

static void os_cmd_rnc(char *origin)
{
	char *param = strtok(NULL, " ");
	int count = param ? atoi(param) : 20;
	node_t *n1, *n2, *biggest = NULL;
	user_t *u;
	rnc_t *rnc;
	list_t realnames;
	int i, found = 0;

	realnames.head = NULL;
	realnames.tail = NULL;
	realnames.count = 0;

	for (i = 0; i < HASHSIZE; i++)
	{
		LIST_FOREACH(n1, userlist[i].head)
		{
			u = n1->data;

			LIST_FOREACH(n2, realnames.head)
			{
				rnc = n2->data;

				if (strcmp(rnc->gecos, u->gecos) == 0)
				{
					/* existing match */
					rnc->count++;
					found = 1;
				}
			}

			if (found == 0)
			{
				/* new realname */
				rnc = (rnc_t *)malloc(sizeof(rnc_t));
				sprintf(rnc->gecos, "%s", u->gecos);
				rnc->count = 1;
				node_add(rnc, node_create(), &realnames);
			}

			found = 0;
		}
	}

	found = 0;

	/* this is ugly to the max :P */
	for (i = 1; i <= count; i++)
	{
		LIST_FOREACH(n1, realnames.head)
		{
			rnc = n1->data;

			if (rnc->count > found)
			{
				found = rnc->count;
				biggest = n1;
			}
		}

		if (biggest == NULL)
			break;

		notice(opersvs.nick, origin, "\2%d\2: \2%d\2 matches for realname \2%s\2", i, ((rnc_t *)(biggest->data))->count, ((rnc_t *)biggest->data)->gecos);
		free(biggest->data);
		node_del(biggest, &realnames);
		node_free(biggest);

		found = 0;
		biggest = NULL;
	}

	/* cleanup*/
	LIST_FOREACH_SAFE(n1, n2, realnames.head)
	{
		rnc = n1->data;

		free(rnc);
		node_del(n1, &realnames);
		node_free(n1);
	}
}

