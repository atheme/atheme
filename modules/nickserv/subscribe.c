/*
 * atheme-services: A collection of minimalist IRC services
 * subscribe.c: NickServ/SUBSCRIBE command implementation.
 *
 * Copyright (c) 2005-2007 Atheme Project (http://www.atheme.org)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

DECLARE_MODULE_V1
(
	"nickserv/subscribe", FALSE, _modinit, _moddeinit,
	"$Id$",
	"Atheme Development Group <http://www.atheme.org>"
);

static void cmd_subscribe(sourceinfo_t *si, int parc, char *parv[])
{
	myuser_t *mu = si->smu, *tmu;
	char *name = parv[1], *tag;
	boolean_t remove = FALSE;
	metadata_subscription_t *md;
	node_t *i;

	if (parc < 1)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SUBSCRIBE");
		command_fail(si, fault_needmoreparams, _("Syntax: SUBSCRIBE [-]username [tags]"));

		return;
	}

	if (mu == NULL)
	{
		command_fail(si, fault_noprivs, _("You are not logged in."));

		return;
	}

	if (*name == '-')
	{
		remove = TRUE;
		name++;
	}

	if ((tmu = myuser_find_ext(name)) == NULL)
	{
		command_fail(si, fault_badparams, _("\2%s\2 is not registered."), name);

		return;
	}

	if (remove)
	{
		node_t *n;
		boolean_t found = FALSE;

		LIST_FOREACH(n, tmu->subscriptions.head)
		{
			md = (metadata_subscription_t *) n->data;

			if (md->mu == mu)
			{
				found = TRUE;
				break;
			}
		}

		if (found)
		{
			node_t *tn;

			LIST_FOREACH_SAFE(n, tn, md->taglist.head)
			{
				free(n->data);

				node_del(n, &md->taglist);
				node_free(n);
			}

			free(md);

			node_del(n, &tmu->subscriptions);
			node_free(n);

			command_success_nodata(si, _("\2%s\2 has been removed from your subscriptions."), name);
			return;
		}

		command_fail(si, fault_badparams, _("\2%s\2 was not found on your subscription list."), name);
		return;
	}

	/* tags are mandatory for new subscriptions */
	if (parc < 2)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SUBSCRIBE");
		command_fail(si, fault_needmoreparams, _("Syntax: SUBSCRIBE [-]username tags"));

		return;
	}

	md = smalloc(sizeof(metadata_subscription_t));
	md->mu = mu;

	tag = strtok(parv[2], ",");
	do
	{
		slog(LG_DEBUG, "subscription: parsing tag %s", tag);
		node_add(sstrdup(tag), node_create(), &md->taglist);
	} while ((tag = strtok(NULL, ",")) != NULL);

	node_add(md, node_create(), &tmu->subscriptions);
	command_success_nodata(si, _("\2%s\2 has been added to your subscriptions."));
}

command_t ns_subscribe = { "SUBSCRIBE", N_("Manages your subscription list."), AC_NONE, 1, cmd_subscribe };

list_t *ns_cmdtree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(ns_cmdtree, "nickserv/main", "ns_cmdtree");

	command_add(&ns_subscribe, ns_cmdtree);
}

void _moddeinit(void)
{
	command_delete(&ns_subscribe, ns_cmdtree);
}
