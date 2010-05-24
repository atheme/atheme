/*
 * Copyright (c) 2006 William Pitcock, et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains routines to handle the CService SET command.
 *
 * $Id: set.c 7907 2007-03-06 23:10:26Z pippijn $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"nickserv/set", false, _modinit, _moddeinit,
	"$Id: set.c 7907 2007-03-06 23:10:26Z pippijn $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void ns_cmd_set(sourceinfo_t *si, int parc, char *parv[]);

list_t *ns_cmdtree, *ns_helptree;

command_t ns_set = { "SET", N_("Sets various control flags."), AC_NONE, 2, ns_cmd_set };

list_t ns_set_cmdtree;

/* HELP SET */
static void ns_help_set(sourceinfo_t *si)
{
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
	command_help(si, &ns_set_cmdtree);
	command_success_nodata(si, " ");
	command_success_nodata(si, _("For more information, use \2/msg %s HELP SET \37command\37\2."), nicksvs.nick);
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
        if ((c = command_find(&ns_set_cmdtree, setting)))
	{
		command_exec(si->service, si, c, parc - 1, parv + 1);
	}
	else
	{
		command_fail(si, fault_badparams, _("Invalid set command. Use \2/%s%s HELP SET\2 for a command listing."), (ircd->uses_rcommand == false) ? "msg " : "", nicksvs.nick);
	}
}

#ifdef ENABLE_NLS
static void _ns_setlanguage(sourceinfo_t *si, int parc, char *parv[])
{
	char *language = strtok(parv[0], " ");
	language_t *lang;

	if (si->smu == NULL)
		return;

	if (language == NULL)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "LANGUAGE");
		command_fail(si, fault_needmoreparams, _("Valid languages are: %s"), language_names());
		return;
	}

	lang = language_find(language);

	if (strcmp(language, "default") &&
			(lang == NULL || !language_is_valid(lang)))
	{
		command_fail(si, fault_badparams, _("Invalid language \2%s\2."), language);
		command_fail(si, fault_badparams, _("Valid languages are: %s"), language_names());
		return;
	}

	logcommand(si, CMDLOG_SET, "SET:LANGUAGE: \2%s\2", language_get_name(lang));

	si->smu->language = lang;

	command_success_nodata(si, _("The language for \2%s\2 has been changed to \2%s\2."), si->smu->name, language_get_name(lang));

	return;
}
command_t ns_set_language = { "LANGUAGE", N_("Changes the language services uses to talk to you."), AC_NONE, 1, _ns_setlanguage };
#endif /* ENABLE_NLS */

command_t *ns_set_commands[] = {
#ifdef ENABLE_NLS
	&ns_set_language,
#endif
	NULL
};

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(ns_cmdtree, "nickserv/main", "ns_cmdtree");
	MODULE_USE_SYMBOL(ns_helptree, "nickserv/main", "ns_helptree");
	command_add(&ns_set, ns_cmdtree);

	help_addentry(ns_helptree, "SET", NULL, ns_help_set);
#ifdef ENABLE_NLS
	help_addentry(ns_helptree, "SET LANGUAGE", "help/nickserv/set_language", NULL);
#endif

	/* populate ns_set_cmdtree */
	command_add_many(ns_set_commands, &ns_set_cmdtree);
}

void _moddeinit()
{
	command_delete(&ns_set, ns_cmdtree);
	help_delentry(ns_helptree, "SET");
#ifdef ENABLE_NLS
	help_delentry(ns_helptree, "SET LANGUAGE");
#endif

	/* clear ns_set_cmdtree */
	command_delete_many(ns_set_commands, &ns_set_cmdtree);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
