/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains routines to handle the GroupServ HELP command.
 *
 */

#include "atheme.h"

#define IN_GROUPSERV_SET
#include "groupserv.h"

DECLARE_MODULE_V1
(
	"groupserv/set", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void gs_help_set(sourceinfo_t *si, const char *subcmd);
static void gs_cmd_set(sourceinfo_t *si, int parc, char *parv[]);

command_t gs_set = { "SET", N_("Sets various control flags."), AC_AUTHENTICATED, 3, gs_cmd_set, { .func = gs_help_set } };

mowgli_patricia_t *gs_set_cmdtree;

void _modinit(module_t *m)
{
	use_groupserv_main_symbols(m);

	service_named_bind_command("groupserv", &gs_set);

	gs_set_cmdtree = mowgli_patricia_create(strcasecanon);
}

void _moddeinit(module_unload_intent_t intent)
{
	service_named_unbind_command("groupserv", &gs_set);

	mowgli_patricia_destroy(gs_set_cmdtree, NULL, NULL);
}

static void gs_help_set(sourceinfo_t *si, const char *subcmd)
{
	if (!subcmd)
	{
		command_success_nodata(si, _("***** \2%s Help\2 *****"), si->service->disp);
		command_success_nodata(si, _("Help for \2SET\2:"));
		command_success_nodata(si, " ");
		command_success_nodata(si, _("SET allows you to set various control flags\n"
					"for groups that change the way certain\n"
					"operations are performed on them."));
		command_success_nodata(si, " ");
		command_help(si, gs_set_cmdtree);
		command_success_nodata(si, " ");
		command_success_nodata(si, _("For more specific help use \2/msg %s HELP SET \37command\37\2."), si->service->disp);
		command_success_nodata(si, _("***** \2End of Help\2 *****"));
	}
	else
		help_display(si, si->service, subcmd, gs_set_cmdtree);
}

/* SET <!group> <setting> <parameters> */
static void gs_cmd_set(sourceinfo_t *si, int parc, char *parv[])
{
	char *group;
	char *cmd;
	command_t *c;

	if (parc < 2)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SET");
		command_fail(si, fault_needmoreparams, _("Syntax: SET <!group> <setting> [parameters]"));
		return;
	}

	if (parv[0][0] == '!')
		group = parv[0], cmd = parv[1];
	else if (parv[1][0] == '!')
		cmd = parv[0], group = parv[1];
	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "SET");
		command_fail(si, fault_badparams, _("Syntax: SET <!group> <setting> [parameters]"));
		return;
	}

	c = command_find(gs_set_cmdtree, cmd);
	if (c == NULL)
	{
		command_fail(si, fault_badparams, _("Invalid command. Use \2/%s%s help\2 for a command listing."), (ircd->uses_rcommand == false) ? "msg " : "", si->service->disp);
		return;
	}

	parv[1] = group;
	command_exec(si->service, si, c, parc - 1, parv + 1);
}


/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
