/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2011 Alexandria Wolcott <alyx@sporksmoo.net>
 * Copyright (C) 2018 Atheme Development Group (https://atheme.github.io/)
 *
 * Gather information about networked servers.
 */

#include <atheme.h>

static mowgli_patricia_t *ss_server_cmds = NULL;

static void
ss_cmd_server(struct sourceinfo *const restrict si, const int parc, char **const restrict parv)
{
	if (parc < 1)
	{
		(void) command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SERVER");
		(void) command_fail(si, fault_needmoreparams, _("Syntax: SERVER [INFO|LIST|COUNT] [parameters]"));
		return;
	}

	(void) subcommand_dispatch_simple(si->service, si, parc, parv, ss_server_cmds, "SERVER");
}

static void
ss_cmd_server_list(struct sourceinfo *const restrict si, const int ATHEME_VATTR_UNUSED parc,
                   char ATHEME_VATTR_UNUSED **const restrict parv)
{
	mowgli_patricia_iteration_state_t state;
	struct server *s;

	unsigned int i = 0;

	MOWGLI_PATRICIA_FOREACH(s, &state, servlist)
	{
		if ((! (s->flags & SF_HIDE)) || has_priv(si, PRIV_SERVER_AUSPEX))
		{
			i++;

			(void) command_success_nodata(si, "%u: %s [%s]", i, s->name, s->desc);
		}
	}

	(void) command_success_nodata(si, _("End of server list."));
}

static void
ss_cmd_server_info(struct sourceinfo *const restrict si, const int parc, char **const restrict parv)
{
	if (parc < 1)
	{
		(void) command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SERVER INFO");
		(void) command_fail(si, fault_needmoreparams, _("Syntax: SERVER INFO <server>"));
		return;
	}

	const struct server *const s = mowgli_patricia_retrieve(servlist, parv[0]);

	if (! s)
	{
		(void) command_fail(si, fault_nosuch_target, _("Server \2%s\2 does not exist."), parv[0]);
		return;
	}

	if ((s->flags & SF_HIDE) && ! has_priv(si, PRIV_SERVER_AUSPEX))
	{
		(void) command_fail(si, fault_nosuch_target, _("Server \2%s\2 does not exist."), parv[0]);
		return;
	}

	(void) command_success_nodata(si, _("Information for server %s:"), s->name);
	(void) command_success_nodata(si, _("Server description: %s"), s->desc);
	(void) command_success_nodata(si, _("Current users: %u (%u invisible)"), s->users, s->invis);
	(void) command_success_nodata(si, _("Online operators: %u"), s->opers);

	if (has_priv(si, PRIV_SERVER_AUSPEX))
	{
		if (s->uplink && s->uplink->name)
			(void) command_success_nodata(si, _("Server uplink: %s"), s->uplink->name);

		(void) command_success_nodata(si, _("Servers linked from %s: %u"), s->name,
		                              (unsigned int) s->children.count);
	}

	(void) command_success_nodata(si, _("End of server info."));
}

static void
ss_cmd_server_count(struct sourceinfo *const restrict si, const int ATHEME_VATTR_UNUSED parc,
                    char ATHEME_VATTR_UNUSED **const restrict parv)
{
	// TRANSLATORS: cannot ever be singular; is always plural
	(void) command_success_nodata(si, _("Network size: %u servers"), mowgli_patricia_size(servlist));
}

static struct command ss_server = {
	.name           = "SERVER",
	.desc           = N_("Obtain information about servers on the network."),
	.access         = AC_NONE,
	.maxparc        = 3,
	.cmd            = &ss_cmd_server,
	.help           = { .path = "statserv/server" },
};

static struct command ss_server_list = {
	.name           = "LIST",
	.desc           = N_("Obtain a list of servers."),
	.access         = AC_NONE,
	.maxparc        = 1,
	.cmd            = &ss_cmd_server_list,
	.help           = { .path = "" },
};

static struct command ss_server_count = {
	.name           = "COUNT",
	.desc           = N_("Count the amount of servers connected to the network."),
	.access         = AC_NONE,
	.maxparc        = 1,
	.cmd            = &ss_cmd_server_count,
	.help           = { .path = "" },
};

static struct command ss_server_info = {
	.name           = "INFO",
	.desc           = N_("Obtain information about a specified server."),
	.access         = AC_NONE,
	.maxparc        = 2,
	.cmd            = &ss_cmd_server_info,
	.help           = { .path = "" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "statserv/main")

	if (! (ss_server_cmds = mowgli_patricia_create(&strcasecanon)))
	{
		(void) slog(LG_ERROR, "%s: mowgli_patricia_create() failed", m->name);

		m->mflags |= MODFLAG_FAIL;
		return;
	}

	(void) command_add(&ss_server_list, ss_server_cmds);
	(void) command_add(&ss_server_count, ss_server_cmds);
	(void) command_add(&ss_server_info, ss_server_cmds);

	(void) service_named_bind_command("statserv", &ss_server);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	(void) service_named_unbind_command("statserv", &ss_server);

	(void) mowgli_patricia_destroy(ss_server_cmds, &command_delete_trie_cb, ss_server_cmds);
}

SIMPLE_DECLARE_MODULE_V1("statserv/server", MODULE_UNLOAD_CAPABILITY_OK)
