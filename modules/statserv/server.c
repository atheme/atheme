/*
 * Copyright (c) 2011 Alexandria Wolcott
 * Released under the same terms as Atheme itself.
 *
 * Gather information about networked servers.
 *
 */

#include "atheme.h"

DECLARE_MODULE_V1("statserv/server", false, _modinit, _moddeinit,
		  PACKAGE_STRING, "Alexandria Wolcott <alyx@sporksmoo.net>");

static void ss_cmd_server(sourceinfo_t * si, int parc, char *parv[]);
static void ss_cmd_server_list(sourceinfo_t * si, int parc, char *parv[]);
static void ss_cmd_server_count(sourceinfo_t * si, int parc, char *parv[]);

command_t ss_server =
	{ "SERVER", N_("Obtain information about servers on the network."), AC_NONE, 2, ss_cmd_server, {.path = "statserv/server"} };

command_t ss_server_list =
	{ "LIST", N_("Obtain a list of servers."), AC_NONE, 1, ss_cmd_server_list, {.path = ""} };

command_t ss_server_count =
	{ "COUNT", N_("Count the amount of servers connected to the network."), AC_NONE, 1, ss_cmd_server_count, {.path = ""} };

mowgli_patricia_t *ss_server_cmds;

void _modinit(module_t * m)
{
	service_named_bind_command("statserv", &ss_server);
	ss_server_cmds = mowgli_patricia_create(strcasecanon);
	command_add(&ss_server_list, ss_server_cmds);
	command_add(&ss_server_count, ss_server_cmds);
}

void _moddeinit(module_unload_intent_t intent)
{
	service_named_unbind_command("statserv", &ss_server);
	command_delete(&ss_server_list, ss_server_cmds);
	command_delete(&ss_server_count, ss_server_cmds);
	mowgli_patricia_destroy(ss_server_cmds, NULL, NULL);
}

static void ss_cmd_server(sourceinfo_t * si, int parc, char *parv[])
{
	command_t *c;
	char *cmd = parv[0];

	if (!cmd)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SERVER");
		command_fail(si, fault_needmoreparams, _("Syntax: SERVER [LIST] [parameters]"));
		return;
	}

	c = command_find(ss_server_cmds, cmd);
	if (c == NULL)
	{
		command_fail(si, fault_badparams,
			     _("Invalid command. Use \2/%s%s help\2 for a command listing."),
			     (ircd->uses_rcommand == false) ? "msg " : "", si->service->disp);
		return;
	}

	command_exec(si->service, si, c, parc - 1, parv + 1);
}

static void ss_cmd_server_list(sourceinfo_t * si, int parc, char *parv[])
{
	server_t *s;
	int i = 0;
	mowgli_patricia_iteration_state_t state;
	MOWGLI_PATRICIA_FOREACH(s, &state, servlist)
	{
		i++;
		command_success_nodata(si, _("%d: %s [%s]"), i, s->name, s->desc);
	}
	command_success_nodata(si, _("End of server list."));
}

static void ss_cmd_server_count(sourceinfo_t * si, int parc, char *parv[])
{
	int count;
	count = mowgli_patricia_size(servlist);
	command_success_nodata(si, _("Network size: %d servers"), count);
}
