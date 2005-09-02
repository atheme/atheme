/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Protocol handling stuff.
 *
 * $Id: pmodule.c 1961 2005-08-30 17:08:23Z nenolod $
 */

#include "atheme.h"

list_t pcommands[HASHSIZE];
static BlockHeap *pcommand_heap;
static BlockHeap *messagetree_heap;
struct cmode_ *mode_list;
struct cmode_ *ignore_mode_list;
struct cmode_ *status_mode_list;
struct cmode_ *prefix_mode_list;
ircd_t *ircd;
boolean_t pmodule_loaded = FALSE;
boolean_t backend_loaded = FALSE;

void pcommand_init(void)
{
	pcommand_heap = BlockHeapCreate(sizeof(pcommand_t), 64);

	if (!pcommand_heap)
	{
		slog(LG_INFO, "pcommand_init(): block allocator failed.");
		exit(EXIT_FAILURE);
	}
}

void pcommand_add(char *token,
	void (*handler)(char *origin, uint8_t parc, char *parv[]))
{
	node_t *n;
	pcommand_t *pcmd;

	if ((pcmd = pcommand_find(token)))
	{
		slog(LG_INFO, "pcommand_add(): token %s is already registered",
				token);
		return;
	}

	pcmd = BlockHeapAlloc(pcommand_heap);
	pcmd->token = sstrdup(token);
	pcmd->handler = handler;

	n = node_create();

	node_add(pcmd, n, &pcommands[shash(token)]);
}

void pcommand_delete(char *token)
{
	pcommand_t *pcmd;
	node_t *n;

	if (!(pcmd = pcommand_find(token)))
	{
		slog(LG_INFO, "pcommand_delete(): token %s is not registered",
				token);
		return;
	}

	n = node_find(pcmd, &pcommands[shash(token)]);
	node_del(n, &pcommands[shash(token)]);

	free(pcmd->token);
	pcmd->handler = NULL;

	BlockHeapFree(pcommand_heap, pcmd);
}

pcommand_t *pcommand_find(char *token)
{
	pcommand_t *pcmd;
	node_t *n;

	if (!token)
	{
		slog(LG_INFO, "pcommand_find(): no token given to locate");
		return NULL;
	}

	LIST_FOREACH(n, pcommands[shash(token)].head)
	{
		pcmd = n->data;

		if (!strcmp(pcmd->token, token))
			return pcmd;
	}

	return NULL;
}

