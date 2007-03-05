/*
 * Copyright (c) 2005-2006 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Protocol handling stuff.
 *
 * $Id: pmodule.c 7823 2007-03-05 23:20:25Z pippijn $
 */

#include "atheme.h"
#include "pmodule.h"

dictionary_tree_t *pcommands;
BlockHeap *pcommand_heap;
BlockHeap *messagetree_heap;
struct cmode_ *mode_list;
struct extmode *ignore_mode_list;
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
		slog(LG_INFO, gettext("pcommand_init(): block allocator failed."));
		exit(EXIT_FAILURE);
	}

	pcommands = dictionary_create("pcommand", HASH_COMMAND, strcmp);
}

void pcommand_add(char *token, void (*handler) (sourceinfo_t *si, int parc, char *parv[]), int minparc, int sourcetype)
{
	pcommand_t *pcmd;

	if ((pcmd = pcommand_find(token)))
	{
		slog(LG_INFO, gettext("pcommand_add(): token %s is already registered"), token);
		return;
	}
	
	pcmd = BlockHeapAlloc(pcommand_heap);
	pcmd->token = sstrdup(token);
	pcmd->handler = handler;
	pcmd->minparc = minparc;
	pcmd->sourcetype = sourcetype;

	dictionary_add(pcommands, pcmd->token, pcmd);
}

void pcommand_delete(char *token)
{
	pcommand_t *pcmd;

	if (!(pcmd = pcommand_find(token)))
	{
		slog(LG_INFO, gettext("pcommand_delete(): token %s is not registered"), token);
		return;
	}

	dictionary_delete(pcommands, pcmd->token);

	free(pcmd->token);
	pcmd->handler = NULL;
	BlockHeapFree(pcommand_heap, pcmd);
}

pcommand_t *pcommand_find(char *token)
{
	return dictionary_retrieve(pcommands, token);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
