/*
 * atheme-services: A collection of minimalist IRC services
 * pmodule.c: Protocol command management.
 *
 * Copyright (c) 2005-2007 Atheme Project (http://www.atheme.org)
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "atheme.h"
#include "pmodule.h"

mowgli_patricia_t *pcommands;

mowgli_heap_t *pcommand_heap;
mowgli_heap_t *messagetree_heap;

struct cmode_ *mode_list;
struct extmode *ignore_mode_list;
size_t ignore_mode_list_size = 0;
struct cmode_ *status_mode_list;
struct cmode_ *prefix_mode_list;
struct cmode_ *user_mode_list;
ircd_t *ircd;
bool pmodule_loaded = false;
bool backend_loaded = false;

void pcommand_init(void)
{
	pcommand_heap = mowgli_heap_create(sizeof(pcommand_t), 64, BH_NOW);

	if (!pcommand_heap)
	{
		slog(LG_INFO, "pcommand_init(): block allocator failed.");
		exit(EXIT_FAILURE);
	}

	pcommands = mowgli_patricia_create(noopcanon);
}

void pcommand_add(const char *token, void (*handler) (sourceinfo_t *si, int parc, char *parv[]), int minparc, int sourcetype)
{
	pcommand_t *pcmd;

	if (pcommand_find(token))
	{
		slog(LG_INFO, "pcommand_add(): token %s is already registered", token);
		return;
	}
	
	pcmd = mowgli_heap_alloc(pcommand_heap);
	pcmd->token = sstrdup(token);
	pcmd->handler = handler;
	pcmd->minparc = minparc;
	pcmd->sourcetype = sourcetype;

	mowgli_patricia_add(pcommands, pcmd->token, pcmd);
}

void pcommand_delete(const char *token)
{
	pcommand_t *pcmd;

	if (!(pcmd = pcommand_find(token)))
	{
		slog(LG_INFO, "pcommand_delete(): token %s is not registered", token);
		return;
	}

	mowgli_patricia_delete(pcommands, pcmd->token);

	free(pcmd->token);
	pcmd->handler = NULL;
	mowgli_heap_free(pcommand_heap, pcmd);
}

pcommand_t *pcommand_find(const char *token)
{
	return mowgli_patricia_retrieve(pcommands, token);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
