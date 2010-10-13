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
	"nickserv/subscribe", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void cmd_subscribe(sourceinfo_t *si, int parc, char *parv[])
{
	myuser_t *mu = si->smu, *tmu;
	char *name = parv[0], *tag;
	bool do_remove = false;
	metadata_subscription_t *md;

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
		do_remove = true;
		name++;
	}

	if ((tmu = myuser_find_ext(name)) == NULL)
	{
		command_fail(si, fault_badparams, _("\2%s\2 is not registered."), name);

		return;
	}

	if (do_remove)
	{
		mowgli_node_t *n;
		bool found = false;

		MOWGLI_ITER_FOREACH(n, tmu->subscriptions.head)
		{
			md = (metadata_subscription_t *) n->data;

			if (md->mu == mu)
			{
				found = true;
				break;
			}
		}

		if (found)
		{
			mowgli_node_t *tn;

			MOWGLI_ITER_FOREACH_SAFE(n, tn, md->taglist.head)
			{
				free(n->data);

				mowgli_node_delete(n, &md->taglist);
				mowgli_node_free(n);
			}

			free(md);

			mowgli_node_delete(n, &tmu->subscriptions);
			mowgli_node_free(n);

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

	tag = strtok(parv[1], ",");
	do
	{
		slog(LG_DEBUG, "subscription: parsing tag %s", tag);
		mowgli_node_add(sstrdup(tag), mowgli_node_create(), &md->taglist);
	} while ((tag = strtok(NULL, ",")) != NULL);

	mowgli_node_add(md, mowgli_node_create(), &tmu->subscriptions);
	command_success_nodata(si, _("\2%s\2 has been added to your subscriptions."), name);
}

command_t ns_subscribe = { "SUBSCRIBE", N_("Manages your subscription list."), AC_NONE, 2, cmd_subscribe, { .path = "" } };

static void hook_metadata_change(hook_metadata_change_t *md)
{
	myuser_t *mu;
	mowgli_node_t *n, *tn;

	/* don't allow private metadata to be exposed to users. */
	if (strchr(md->name, ':'))
		return;

#if 0
	if (md->type != METADATA_USER)
		return;
#endif

	mu = (myuser_t *) md->target;

	MOWGLI_ITER_FOREACH(n, mu->subscriptions.head)
	{
		metadata_subscription_t *mds = n->data;

		MOWGLI_ITER_FOREACH(tn, mds->taglist.head)
		{
			if (!match(tn->data, md->name) && md->value != NULL)
				myuser_notice(nicksvs.nick, mds->mu, "\2%s\2 has changed \2%s\2 to \2%s\2.",
					entity(mu)->name, md->name, md->value);
		}
	}
}

void _modinit(module_t *m)
{

	service_named_bind_command("nickserv", &ns_subscribe);

	hook_add_event("metadata_change");
	hook_add_metadata_change(hook_metadata_change);
}

void _moddeinit(void)
{
	service_named_unbind_command("nickserv", &ns_subscribe);
	hook_del_metadata_change(hook_metadata_change);
}
