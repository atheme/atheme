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

const struct cmode *mode_list = NULL;
struct extmode *ignore_mode_list;
size_t ignore_mode_list_size = 0;
const struct cmode *status_mode_list = NULL;
const struct cmode *prefix_mode_list = NULL;
const struct cmode *user_mode_list = NULL;
struct ircd *ircd = NULL;
bool backend_loaded = false;

void
pcommand_init(void)
{
	pcommand_heap = sharedheap_get(sizeof(struct proto_cmd));

	if (!pcommand_heap)
	{
		slog(LG_INFO, "pcommand_init(): block allocator failed.");
		exit(EXIT_FAILURE);
	}

	pcommands = mowgli_patricia_create(noopcanon);
}

void
pcommand_add(const char *token, void (*handler) (struct sourceinfo *si, int parc, char *parv[]), int minparc, int sourcetype)
{
	struct proto_cmd *pcmd;

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

void
pcommand_delete(const char *token)
{
	struct proto_cmd *pcmd;

	if (!(pcmd = pcommand_find(token)))
	{
		slog(LG_INFO, "pcommand_delete(): token %s is not registered", token);
		return;
	}

	mowgli_patricia_delete(pcommands, pcmd->token);

	sfree(pcmd->token);
	pcmd->handler = NULL;
	mowgli_heap_free(pcommand_heap, pcmd);
}

struct proto_cmd *
pcommand_find(const char *token)
{
	return mowgli_patricia_retrieve(pcommands, token);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
