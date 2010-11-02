/*
 * Copyright (c) 2006 William Pitcock, et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains routines to handle the CService SET command.
 *
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"nickserv/set_core", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void ns_help_set(sourceinfo_t *si, const char *subcmd);
static void ns_cmd_set(sourceinfo_t *si, int parc, char *parv[]);

command_t ns_set = { "SET", N_("Sets various control flags."), AC_NONE, 2, ns_cmd_set, { .func = ns_help_set } };

mowgli_patricia_t *ns_set_cmdtree;

/* HELP SET */
static void ns_help_set(sourceinfo_t *si, const char *subcmd)
{
	if (!subcmd)
	{
		command_success_nodata(si, _("***** \2%s Help\2 *****"), nicksvs.nick);
		command_success_nodata(si, _("Help for \2SET\2:"));
		command_success_nodata(si, " ");
		if (nicksvs.no_nick_ownership)
			command_success_nodata(si, _("SET allows you to set various control flags\n"
						"for accounts that change the way certain\n"
						"operations are performed on them."));
		else
			command_success_nodata(si, _("SET allows you to set various control flags\n"
						"for nicknames that change the way certain\n"
						"operations are performed on them."));
		command_success_nodata(si, " ");
		command_help(si, ns_set_cmdtree);
		command_success_nodata(si, " ");
		command_success_nodata(si, _("For more information, use \2/msg %s HELP SET \37command\37\2."), nicksvs.nick);
		command_success_nodata(si, _("***** \2End of Help\2 *****"));
	}
	else
		help_display(si, si->service, subcmd, ns_set_cmdtree);
}

/* SET <setting> <parameters> */
static void ns_cmd_set(sourceinfo_t *si, int parc, char *parv[])
{
	char *setting = parv[0];
	command_t *c;

	if (si->smu == NULL)
	{
		command_fail(si, fault_noprivs, _("You are not logged in."));
		return;
	}

	if (setting == NULL)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SET");
		command_fail(si, fault_needmoreparams, _("Syntax: SET <setting> <parameters>"));
		return;
	}

	/* take the command through the hash table */
        if ((c = command_find(ns_set_cmdtree, setting)))
	{
		command_exec(si->service, si, c, parc - 1, parv + 1);
	}
	else
	{
		command_fail(si, fault_badparams, _("Invalid set command. Use \2/%s%s HELP SET\2 for a command listing."), (ircd->uses_rcommand == false) ? "msg " : "", nicksvs.nick);
	}
}

void _modinit(module_t *m)
{
	service_named_bind_command("nickserv", &ns_set);

	ns_set_cmdtree = mowgli_patricia_create(strcasecanon);
}

void _moddeinit()
{
	service_named_unbind_command("nickserv", &ns_set);
	mowgli_patricia_destroy(ns_set_cmdtree, NULL, NULL);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
