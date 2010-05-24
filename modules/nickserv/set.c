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

static void _ns_setproperty(sourceinfo_t *si, int parc, char *parv[])
{
	char *property = strtok(parv[0], " ");
	char *value = strtok(NULL, "");
	unsigned int count;
	node_t *n;
	metadata_t *md;
	hook_metadata_change_t mdchange;

	if (si->smu == NULL)
		return;

	if (!property)
	{
		command_fail(si, fault_needmoreparams, _("Syntax: SET PROPERTY <property> [value]"));
		return;
	}

	if (strchr(property, ':') && !has_priv(si, PRIV_METADATA))
	{
		command_fail(si, fault_badparams, _("Invalid property name."));
		return;
	}

	if (strchr(property, ':'))
		logcommand(si, CMDLOG_SET, "SET:PROPERTY: \2%s\2: \2%s\2/\2%s\2", si->smu->name, property, value);

	if (!value)
	{
		md = metadata_find(si->smu, property);

		if (!md)
		{
			command_fail(si, fault_nosuch_target, _("Metadata entry \2%s\2 was not set."), property);
			return;
		}

		mdchange.target = si->smu;
		mdchange.name = md->name;
		mdchange.value = md->value;
		hook_call_metadata_change(&mdchange);

		metadata_delete(si->smu, property);
		logcommand(si, CMDLOG_SET, "SET:PROPERTY: \2%s\2 (deleted)", property);
		command_success_nodata(si, _("Metadata entry \2%s\2 has been deleted."), property);
		return;
	}

	count = 0;
	LIST_FOREACH(n, object(si->smu)->metadata.head)
	{
		md = n->data;
		if (strchr(property, ':') ? md->private : !md->private)
			count++;
	}
	if (count >= me.mdlimit)
	{
		command_fail(si, fault_toomany, _("Cannot add \2%s\2 to \2%s\2 metadata table, it is full."),
					property, si->smu->name);
		return;
	}

	if (strlen(property) > 32 || strlen(value) > 300 || has_ctrl_chars(property))
	{
		command_fail(si, fault_badparams, _("Parameters are too long. Aborting."));
		return;
	}

	md = metadata_add(si->smu, property, value);
	if (md != NULL)
	{
		mdchange.target = si->smu;
		mdchange.name = md->name;
		mdchange.value = md->value;
		hook_call_metadata_change(&mdchange);
	}
	logcommand(si, CMDLOG_SET, "SET:PROPERTY: \2%s\2 to \2%s\2", property, value);
	command_success_nodata(si, _("Metadata entry \2%s\2 added."), property);
}

command_t ns_set_property = { "PROPERTY", N_("Manipulates metadata entries associated with an account."), AC_NONE, 2, _ns_setproperty };

static void _ns_setpassword(sourceinfo_t *si, int parc, char *parv[])
{
	char *password = strtok(parv[0], " ");

	if (si->smu == NULL)
		return;

	if (auth_module_loaded)
	{
		command_fail(si, fault_noprivs, _("You must change the password in the external system."));
		return;
	}

	if (password == NULL)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "PASSWORD");
		return;
	}

	if (strlen(password) > 32)
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "PASSWORD");
		return;
	}

	if (!strcasecmp(password, si->smu->name))
	{
		command_fail(si, fault_badparams, _("You cannot use your nickname as a password."));
		command_fail(si, fault_badparams, _("Syntax: SET PASSWORD <new password>"));
		return;
	}

	logcommand(si, CMDLOG_SET, "SET:PASSWORD");

	set_password(si->smu, password);

	command_success_nodata(si, _("The password for \2%s\2 has been changed to \2%s\2."), si->smu->name, password);

	return;
}

command_t ns_set_password = { "PASSWORD", N_("Changes the password associated with your account."), AC_NONE, 1, _ns_setpassword };

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
	&ns_set_password,
	&ns_set_property,
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
	help_addentry(ns_helptree, "SET PASSWORD", "help/nickserv/set_password", NULL);
	help_addentry(ns_helptree, "SET PROPERTY", "help/nickserv/set_property", NULL);
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
	help_delentry(ns_helptree, "SET PASSWORD");
	help_delentry(ns_helptree, "SET PROPERTY");
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
