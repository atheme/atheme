/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2026 Atheme Development Group (https://atheme.github.io/)
 * Copyright (C) 2026 syk <syk@localhost>
 *
 * This file contains the main() routine for the Serv compatibility router.
 */

#include <atheme.h>

struct serv_namespace {
	const char *public_name;
	const char *target_service;
};

static const struct serv_namespace serv_namespaces[] = {
	{ "NICK",    "nickserv" },
	{ "ACCOUNT", "nickserv" },
	{ "CHAN",    "chanserv" },
	{ "CHANNEL", "chanserv" },
	{ "MEMO",    "memoserv" },
	{ "GROUP",   "groupserv" },
	{ "OPER",    "operserv" },
	{ "INFO",    "infoserv" },
	{ "GLOBAL",  "global"   },
	{ NULL,      NULL       },
};

static struct service *servsvs = NULL;

static const char *
serv_namespace_service(const char *name)
{
	const struct serv_namespace *ns;

	if (name == NULL)
		return NULL;

	for (ns = serv_namespaces; ns->public_name != NULL; ns++)
		if (irccasecmp(ns->public_name, name) == 0)
			return ns->target_service;

	return NULL;
}

static void
serv_show_root_help(struct sourceinfo *si)
{
	(void) help_display_prefix(si, si->service);

	(void) command_success_nodata(si, _("%s provides one entry point for network services."), si->service->disp);
	(void) help_display_newline(si);
	(void) command_success_nodata(si, _("Account commands:  /msg %s NICK <command>"), si->service->disp);
	(void) command_success_nodata(si, _("Channel commands:  /msg %s CHAN <command>"), si->service->disp);
	(void) command_success_nodata(si, _("Memo commands:     /msg %s MEMO <command>"), si->service->disp);
	(void) command_success_nodata(si, _("Group commands:    /msg %s GROUP <command>"), si->service->disp);
	(void) command_success_nodata(si, _("Oper commands:     /msg %s OPER <command>"), si->service->disp);
	(void) command_success_nodata(si, _("Info commands:     /msg %s INFO <command>"), si->service->disp);
	(void) command_success_nodata(si, _("Global commands:   /msg %s GLOBAL <command>"), si->service->disp);
	(void) help_display_newline(si);
	(void) command_success_nodata(si, _("For details: /msg %s HELP NICK"), si->service->disp);
	(void) command_success_nodata(si, _("             /msg %s HELP CHAN"), si->service->disp);
	(void) command_success_nodata(si, _("             /msg %s HELP MEMO"), si->service->disp);
	(void) command_success_nodata(si, _("             /msg %s HELP GROUP"), si->service->disp);
	(void) command_success_nodata(si, _("             /msg %s HELP OPER"), si->service->disp);
	(void) command_success_nodata(si, _("             /msg %s HELP INFO"), si->service->disp);
	(void) command_success_nodata(si, _("             /msg %s HELP GLOBAL"), si->service->disp);

	(void) help_display_suffix(si);
}

static void
serv_show_ns_help(struct sourceinfo *si, const char *ns, const char *service)
{
	(void) command_success_nodata(si, _("The \2%s\2 namespace provides \2%s\2 commands."), ns, service);
	(void) command_success_nodata(si, _("Syntax: /msg %s %s <command> [arguments...]"), si->service->disp, ns);
}

static void
serv_cmd_help(struct sourceinfo *const restrict si, const int ATHEME_VATTR_UNUSED parc, char **const restrict parv)
{
	struct service *const saved = si->service;
	char *ns, *subcommand;
	const char *service;
	struct service *dest;

	if (parv[0] == NULL)
	{
		serv_show_root_help(si);
		return;
	}

	ns = strtok(parv[0], " ");
	subcommand = strtok(NULL, "");

	if (ns == NULL)
	{
		serv_show_root_help(si);
		return;
	}

	if (strcasecmp(ns, "COMMANDS") == 0)
	{
		(void) command_help(si, si->service->commands);
		return;
	}

	service = serv_namespace_service(ns);
	if (service != NULL)
	{
		if (subcommand == NULL)
		{
			(void) help_display(si, si->service, ns, si->service->commands);
			return;
		}

		dest = service_find(service);
		if (dest == NULL)
		{
			(void) command_fail(si, fault_nosuch_target, _("The \2%s\2 service is currently unavailable."), service);
			return;
		}

		si->service = dest;
		(void) command_exec_split(dest, si, "HELP", subcommand, dest->commands);
		si->service = saved;
		return;
	}

	(void) help_display_invalid(si, si->service, ns);
}

static void
serv_cmd_ns(struct sourceinfo *const restrict si, const int ATHEME_VATTR_UNUSED parc, char **const restrict parv)
{
	const char *const ns = si->command->name;
	const char *const service = serv_namespace_service(ns);
	struct service *const dest = service_find(service);
	struct service *const saved = si->service;
	char *cmd, *text;

	if (parv[0] == NULL)
	{
		(void) command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, ns);
		(void) command_fail(si, fault_needmoreparams, _("Syntax: %s <command> [arguments...]"), ns);
		serv_show_ns_help(si, ns, service);
		return;
	}

	if (dest == NULL)
	{
		(void) command_fail(si, fault_nosuch_target, _("The \2%s\2 service is currently unavailable."), service);
		return;
	}

	cmd = strtok(parv[0], " ");
	text = strtok(NULL, "");

	if (cmd == NULL)
	{
		(void) command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, ns);
		(void) command_fail(si, fault_needmoreparams, _("Syntax: %s <command> [arguments...]"), ns);
		serv_show_ns_help(si, ns, service);
		return;
	}

	si->service = dest;
	(void) command_exec_split(dest, si, cmd, text, dest->commands);
	si->service = saved;
}

static void
serv_handler(struct sourceinfo *si, int parc, char *parv[])
{
	char *cmd, *text;

	if (parv[parc - 1] == NULL || parv[parc - 1][0] == '\0')
	{
		serv_show_root_help(si);
		return;
	}

	cmd = strtok(parv[parc - 1], " ");
	text = strtok(NULL, "");

	if (!cmd)
	{
		serv_show_root_help(si);
		return;
	}

	if (*parv[parc - 1] == '\001')
	{
		(void) handle_ctcp_common(si, cmd, text);
		return;
	}

	(void) command_exec_split(si->service, si, cmd, text, si->service->commands);
}

static struct command serv_help = {
	.name           = "HELP",
	.desc           = STR_HELP_DESCRIPTION,
	.access         = AC_NONE,
	.maxparc        = 1,
	.cmd            = &serv_cmd_help,
	.help           = { .path = "serv/help" },
};

static struct command serv_nick = {
	.name           = "NICK",
	.desc           = N_("Provides account and nickname commands."),
	.access         = AC_NONE,
	.maxparc        = 1,
	.cmd            = &serv_cmd_ns,
	.help           = { .path = "serv/nick" },
};

static struct command serv_account = {
	.name           = "ACCOUNT",
	.desc           = N_("Provides account commands."),
	.access         = AC_NONE,
	.maxparc        = 1,
	.cmd            = &serv_cmd_ns,
	.help           = { .path = "serv/nick" },
};

static struct command serv_chan = {
	.name           = "CHAN",
	.desc           = N_("Provides channel commands."),
	.access         = AC_NONE,
	.maxparc        = 1,
	.cmd            = &serv_cmd_ns,
	.help           = { .path = "serv/chan" },
};

static struct command serv_channel = {
	.name           = "CHANNEL",
	.desc           = N_("Provides channel commands."),
	.access         = AC_NONE,
	.maxparc        = 1,
	.cmd            = &serv_cmd_ns,
	.help           = { .path = "serv/chan" },
};

static struct command serv_memo = {
	.name           = "MEMO",
	.desc           = N_("Provides memo commands."),
	.access         = AC_NONE,
	.maxparc        = 1,
	.cmd            = &serv_cmd_ns,
	.help           = { .path = "serv/memo" },
};

static struct command serv_group = {
	.name           = "GROUP",
	.desc           = N_("Provides group commands."),
	.access         = AC_NONE,
	.maxparc        = 1,
	.cmd            = &serv_cmd_ns,
	.help           = { .path = "serv/group" },
};

static struct command serv_oper = {
	.name           = "OPER",
	.desc           = N_("Provides operator commands."),
	.access         = AC_NONE,
	.maxparc        = 1,
	.cmd            = &serv_cmd_ns,
	.help           = { .path = "serv/oper" },
};

static struct command serv_info = {
	.name           = "INFO",
	.desc           = N_("Provides information commands."),
	.access         = AC_NONE,
	.maxparc        = 1,
	.cmd            = &serv_cmd_ns,
	.help           = { .path = "serv/info" },
};

static struct command serv_global = {
	.name           = "GLOBAL",
	.desc           = N_("Provides global announcement commands."),
	.access         = AC_NONE,
	.maxparc        = 1,
	.cmd            = &serv_cmd_ns,
	.help           = { .path = "serv/global" },
};

static void
mod_init(struct module *const restrict m)
{
	if (! (servsvs = service_add("serv", serv_handler)))
	{
		(void) slog(LG_ERROR, "%s: service_add() failed", m->name);

		m->mflags |= MODFLAG_FAIL;
		return;
	}

	(void) service_bind_command(servsvs, &serv_help);
	(void) service_bind_command(servsvs, &serv_nick);
	(void) service_bind_command(servsvs, &serv_account);
	(void) service_bind_command(servsvs, &serv_chan);
	(void) service_bind_command(servsvs, &serv_channel);
	(void) service_bind_command(servsvs, &serv_memo);
	(void) service_bind_command(servsvs, &serv_group);
	(void) service_bind_command(servsvs, &serv_oper);
	(void) service_bind_command(servsvs, &serv_info);
	(void) service_bind_command(servsvs, &serv_global);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	(void) service_unbind_command(servsvs, &serv_channel);
	(void) service_unbind_command(servsvs, &serv_chan);
	(void) service_unbind_command(servsvs, &serv_account);
	(void) service_unbind_command(servsvs, &serv_nick);
	(void) service_unbind_command(servsvs, &serv_help);
	(void) service_unbind_command(servsvs, &serv_group);
	(void) service_unbind_command(servsvs, &serv_oper);
	(void) service_unbind_command(servsvs, &serv_info);
	(void) service_unbind_command(servsvs, &serv_global);
	(void) service_unbind_command(servsvs, &serv_memo);

	(void) service_delete(servsvs);
	servsvs = NULL;
}

SIMPLE_DECLARE_MODULE_V1("serv/main", MODULE_UNLOAD_CAPABILITY_OK)
