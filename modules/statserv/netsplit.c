/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2011 Alexandria Wolcott <alyx@sporksmoo.net>
 * Copyright (C) 2018 Atheme Development Group (https://atheme.github.io/)
 *
 * Netsplit monitor
 */

#include <atheme.h>

struct netsplit
{
	char *          name;
	time_t          since;
};

static mowgli_patricia_t *ss_netsplit_cmds = NULL;
static mowgli_patricia_t *splitlist = NULL;
static mowgli_heap_t *split_heap = NULL;

static void
netsplit_delete_serv(struct netsplit *const restrict s)
{
	(void) mowgli_patricia_delete(splitlist, s->name);

	(void) sfree(s->name);

	(void) mowgli_heap_free(split_heap, s);
}

static void
netsplit_server_add(struct server *const restrict s)
{
	struct netsplit *const serv = mowgli_patricia_retrieve(splitlist, s->name);

	if (serv)
		(void) netsplit_delete_serv(serv);
}

static void
netsplit_server_delete(struct hook_server_delete *const restrict serv)
{
	struct netsplit *const s = mowgli_heap_alloc(split_heap);

	s->name  = sstrdup(serv->s->name);
	s->since = CURRTIME;

	(void) mowgli_patricia_add(splitlist, s->name, s);
}

static void
ss_cmd_netsplit(struct sourceinfo *const restrict si, const int parc, char **const restrict parv)
{
	if (parc < 1)
	{
		(void) command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "NETSPLIT");
		(void) command_fail(si, fault_needmoreparams, _("Syntax: NETSPLIT [LIST|REMOVE] [parameters]"));
		return;
	}

	(void) subcommand_dispatch_simple(si->service, si, parc, parv, ss_netsplit_cmds, "NETSPLIT");
}

static void
ss_cmd_netsplit_list(struct sourceinfo *const restrict si, const int ATHEME_VATTR_UNUSED parc,
                     char ATHEME_VATTR_UNUSED **const restrict parv)
{
	struct netsplit *s;
	mowgli_patricia_iteration_state_t state;
	unsigned int i = 0;

	MOWGLI_PATRICIA_FOREACH(s, &state, splitlist)
	{
		i++;

		(void) command_success_nodata(si, _("%u: %s [split %s ago]"), i, s->name, time_ago(s->since));
	}

	(void) command_success_nodata(si, _("End of netsplit list."));
}

static void
ss_cmd_netsplit_remove(struct sourceinfo *const restrict si, const int parc, char **const restrict parv)
{
	if (parc < 1)
	{
		(void) command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "NETSPLIT REMOVE");
		(void) command_fail(si, fault_needmoreparams, _("Syntax: NETSPLIT REMOVE <server>"));
		return;
	}

	struct netsplit *const s = mowgli_patricia_retrieve(splitlist, parv[0]);

	if (! s)
	{
		(void) command_fail(si, fault_nosuch_target, _("The server \2%s\2 is not a split server."), parv[0]);
		return;
	}

	(void) netsplit_delete_serv(s);
	(void) command_success_nodata(si, _("%s removed from the netsplit list."), parv[0]);
}

static struct command ss_netsplit = {
	.name           = "NETSPLIT",
	.desc           = N_("Monitor network splits."),
	.access         = PRIV_SERVER_AUSPEX,
	.maxparc        = 2,
	.cmd            = &ss_cmd_netsplit,
	.help           = { .path = "statserv/netsplit" },
};

static struct command ss_netsplit_list = {
	.name           = "LIST",
	.desc           = N_("List currently split servers."),
	.access         = PRIV_SERVER_AUSPEX,
	.maxparc        = 1,
	.cmd            = &ss_cmd_netsplit_list,
	.help           = { .path = "" },
};

static struct command ss_netsplit_remove = {
	.name           = "REMOVE",
	.desc           = N_("Remove a server from the netsplit list."),
	.access         = PRIV_JUPE,
	.maxparc        = 2,
	.cmd            = &ss_cmd_netsplit_remove,
	.help           = { .path = "" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "statserv/main")

	if (! (ss_netsplit_cmds = mowgli_patricia_create(&strcasecanon)))
	{
		(void) slog(LG_ERROR, "%s: mowgli_patricia_create() failed", m->name);

		m->mflags |= MODFLAG_FAIL;
		return;
	}

	if (! (splitlist = mowgli_patricia_create(&irccasecanon)))
	{
		(void) slog(LG_ERROR, "%s: mowgli_patricia_create() failed", m->name);

		(void) mowgli_patricia_destroy(ss_netsplit_cmds, NULL, NULL);

		m->mflags |= MODFLAG_FAIL;
		return;
	}

	if (! (split_heap = mowgli_heap_create(sizeof(struct netsplit), 32, BH_NOW)))
	{
		(void) slog(LG_ERROR, "%s: mowgli_heap_create() failed", m->name);

		(void) mowgli_patricia_destroy(ss_netsplit_cmds, NULL, NULL);
		(void) mowgli_patricia_destroy(splitlist, NULL, NULL);

		m->mflags |= MODFLAG_FAIL;
		return;
	}

	(void) command_add(&ss_netsplit_list, ss_netsplit_cmds);
	(void) command_add(&ss_netsplit_remove, ss_netsplit_cmds);

	(void) service_named_bind_command("statserv", &ss_netsplit);

	(void) hook_add_server_add(&netsplit_server_add);
	(void) hook_add_server_delete(&netsplit_server_delete);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	mowgli_patricia_iteration_state_t state;
	struct netsplit *s;

	MOWGLI_PATRICIA_FOREACH(s, &state, splitlist)
		(void) netsplit_delete_serv(s);

	(void) mowgli_heap_destroy(split_heap);

	(void) service_named_unbind_command("statserv", &ss_netsplit);

	(void) mowgli_patricia_destroy(ss_netsplit_cmds, &command_delete_trie_cb, ss_netsplit_cmds);
	(void) mowgli_patricia_destroy(splitlist, NULL, NULL);

	(void) hook_del_server_add(&netsplit_server_add);
	(void) hook_del_server_delete(&netsplit_server_delete);
}

SIMPLE_DECLARE_MODULE_V1("statserv/netsplit", MODULE_UNLOAD_CAPABILITY_OK)
