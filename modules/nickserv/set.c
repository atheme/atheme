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

/* SET EMAIL <new address> */
static void _ns_setemail(sourceinfo_t *si, int parc, char *parv[])
{
	char *email = parv[0];
	metadata_t *md;

	if (si->smu == NULL)
		return;

	if (email == NULL)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "EMAIL");
		command_fail(si, fault_needmoreparams, _("Syntax: SET EMAIL <new e-mail>"));
		return;
	}

	if (si->smu->flags & MU_WAITAUTH)
	{
		command_fail(si, fault_noprivs, _("Please verify your original registration before changing your e-mail address."));
		return;
	}

	if (!strcasecmp(si->smu->email, email))
	{
		md = metadata_find(si->smu, "private:verify:emailchg:newemail");
		if (md != NULL)
		{
			command_success_nodata(si, _("The email address change to \2%s\2 has been cancelled."), md->value);
			metadata_delete(si->smu, "private:verify:emailchg:key");
			metadata_delete(si->smu, "private:verify:emailchg:newemail");
			metadata_delete(si->smu, "private:verify:emailchg:timestamp");
		}
		else
			command_fail(si, fault_nochange, _("The email address for account \2%s\2 is already set to \2%s\2."), si->smu->name, si->smu->email);
		return;
	}

	if (!validemail(email))
	{
		command_fail(si, fault_badparams, _("\2%s\2 is not a valid email address."), email);
		return;
	}

	snoop("SET:EMAIL: \2%s\2 (\2%s\2 -> \2%s\2)", si->smu->name, si->smu->email, email);

	if (me.auth == AUTH_EMAIL)
	{
		unsigned long key = makekey();

		metadata_add(si->smu, "private:verify:emailchg:key", itoa(key));
		metadata_add(si->smu, "private:verify:emailchg:newemail", email);
		metadata_add(si->smu, "private:verify:emailchg:timestamp", itoa(time(NULL)));

		if (!sendemail(si->su != NULL ? si->su : si->service->me, EMAIL_SETEMAIL, si->smu, itoa(key)))
		{
			command_fail(si, fault_emailfail, _("Sending email failed, sorry! Your email address is unchanged."));
			metadata_delete(si->smu, "private:verify:emailchg:key");
			metadata_delete(si->smu, "private:verify:emailchg:newemail");
			metadata_delete(si->smu, "private:verify:emailchg:timestamp");
			return;
		}

		logcommand(si, CMDLOG_SET, "SET EMAIL %s (awaiting verification)", email);
		command_success_nodata(si, _("An email containing email changing instructions has been sent to \2%s\2."), email);
		command_success_nodata(si, _("Your email address will not be changed until you follow these instructions."));

		return;
	}

	myuser_set_email(si->smu, email);

	logcommand(si, CMDLOG_SET, "SET EMAIL %s", email);
	command_success_nodata(si, _("The email address for account \2%s\2 has been changed to \2%s\2."), si->smu->name, si->smu->email);
}

command_t ns_set_email = { "EMAIL", N_("Changes your e-mail address."), AC_NONE, 1, _ns_setemail };

/* SET HIDEMAIL [ON|OFF] */
static void _ns_sethidemail(sourceinfo_t *si, int parc, char *parv[])
{
	char *params = strtok(parv[0], " ");

	if (si->smu == NULL)
		return;

	if (params == NULL)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "HIDEMAIL");
		return;
	}

	if (!strcasecmp("ON", params))
	{
		if (MU_HIDEMAIL & si->smu->flags)
		{
			command_fail(si, fault_nochange, _("The \2%s\2 flag is already set for account \2%s\2."), "HIDEMAIL", si->smu->name);
			return;
		}

		logcommand(si, CMDLOG_SET, "SET HIDEMAIL ON");

		si->smu->flags |= MU_HIDEMAIL;

		command_success_nodata(si, _("The \2%s\2 flag has been set for account \2%s\2."), "HIDEMAIL" ,si->smu->name);

		return;
	}
	else if (!strcasecmp("OFF", params))
	{
		if (!(MU_HIDEMAIL & si->smu->flags))
		{
			command_fail(si, fault_nochange, _("The \2%s\2 flag is not set for account \2%s\2."), "HIDEMAIL", si->smu->name);
			return;
		}

		logcommand(si, CMDLOG_SET, "SET HIDEMAIL OFF");

		si->smu->flags &= ~MU_HIDEMAIL;

		command_success_nodata(si, _("The \2%s\2 flag has been removed for account \2%s\2."), "HIDEMAIL", si->smu->name);

		return;
	}
	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "HIDEMAIL");
		return;
	}
}

command_t ns_set_hidemail = { "HIDEMAIL", N_("Hides your e-mail address."), AC_NONE, 1, _ns_sethidemail };

/* SET HIDEMAIL [ON|OFF] */
static void _ns_setquietchg(sourceinfo_t *si, int parc, char *parv[])
{
	char *params = strtok(parv[0], " ");

	if (si->smu == NULL)
		return;

	if (params == NULL)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "HIDEMAIL");
		return;
	}

	if (!strcasecmp("ON", params))
	{
		if (MU_QUIETCHG & si->smu->flags)
		{
			command_fail(si, fault_nochange, _("The \2%s\2 flag is already set for account \2%s\2."), "QUIETCHG", si->smu->name);
			return;
		}

		logcommand(si, CMDLOG_SET, "SET QUIETCHG ON");

		si->smu->flags |= MU_QUIETCHG;

		command_success_nodata(si, _("The \2%s\2 flag has been set for account \2%s\2."), "QUIETCHG" ,si->smu->name);

		return;
	}
	else if (!strcasecmp("OFF", params))
	{
		if (!(MU_QUIETCHG & si->smu->flags))
		{
			command_fail(si, fault_nochange, _("The \2%s\2 flag is not set for account \2%s\2."), "QUIETCHG", si->smu->name);
			return;
		}

		logcommand(si, CMDLOG_SET, "SET QUIETCHG OFF");

		si->smu->flags &= ~MU_QUIETCHG;

		command_success_nodata(si, _("The \2%s\2 flag has been removed for account \2%s\2."), "QUIETCHG", si->smu->name);

		return;
	}
	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "QUIETCHG");
		return;
	}
}

command_t ns_set_quietchg = { "QUIETCHG", N_("Allows you to opt-out of channel change messages."), AC_NONE, 1, _ns_setquietchg };

static void _ns_setemailmemos(sourceinfo_t *si, int parc, char *parv[])
{
	char *params = strtok(parv[0], " ");

	if (si->smu == NULL)
		return;

	if (si->smu->flags & MU_WAITAUTH)
	{
		command_fail(si, fault_noprivs, _("You have to verify your email address before you can enable emailing memos."));
		return;
	}

	if (params == NULL)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "EMAILMEMOS");
		return;
	}

	if (!strcasecmp("ON", params))
	{
		if (me.mta == NULL)
		{
			command_fail(si, fault_emailfail, _("Sending email is administratively disabled."));
			return;
		}
		if (MU_EMAILMEMOS & si->smu->flags)
		{
			command_fail(si, fault_nochange, _("The \2%s\2 flag is already set for account \2%s\2."), "EMAILMEMOS", si->smu->name);
			return;
		}

		logcommand(si, CMDLOG_SET, "SET EMAILMEMOS ON");
		si->smu->flags |= MU_EMAILMEMOS;
		command_success_nodata(si, _("The \2%s\2 flag has been set for account \2%s\2."), "EMAILMEMOS", si->smu->name);
		return;
	}

	else if (!strcasecmp("OFF", params))
	{
		if (!(MU_EMAILMEMOS & si->smu->flags))
		{
			command_fail(si, fault_nochange, _("The \2%s\2 flag is not set for account \2%s\2."), "EMAILMEMOS", si->smu->name);
			return;
		}

		logcommand(si, CMDLOG_SET, "SET EMAILMEMOS OFF");
		si->smu->flags &= ~MU_EMAILMEMOS;
		command_success_nodata(si, _("The \2%s\2 flag has been removed for account \2%s\2."), "EMAILMEMOS", si->smu->name);
		return;
	}
	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "EMAILMEMOS");
		return;
	}
}

command_t ns_set_emailmemos = { "EMAILMEMOS", N_("Forwards incoming memos to your e-mail address."), AC_NONE, 1, _ns_setemailmemos };

static void _ns_setnomemo(sourceinfo_t *si, int parc, char *parv[])
{
	char *params = strtok(parv[0], " ");

	if (si->smu == NULL)
		return;

	if (params == NULL)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "NOMEMO");
		return;
	}

	if (!strcasecmp("ON", params))
	{
		if (MU_NOMEMO & si->smu->flags)
		{
			command_fail(si, fault_nochange, _("The \2%s\2 flag is already set for account \2%s\2."), "NOMEMO", si->smu->name);
			return;
		}

		logcommand(si, CMDLOG_SET, "SET NOMEMO ON");
		si->smu->flags |= MU_NOMEMO;
		command_success_nodata(si, _("The \2%s\2 flag has been set for account \2%s\2."), "NOMEMO", si->smu->name);
		return;
	}

	else if (!strcasecmp("OFF", params))
	{
		if (!(MU_NOMEMO & si->smu->flags))
		{
			command_fail(si, fault_nochange, _("The \2%s\2 flag is not set for account \2%s\2."), "NOMEMO", si->smu->name);
			return;
		}

		logcommand(si, CMDLOG_SET, "SET NOMEMO OFF");
		si->smu->flags &= ~MU_NOMEMO;
		command_success_nodata(si, _("The \2%s\2 flag has been removed for account \2%s\2."), "NOMEMO", si->smu->name);
		return;
	}
	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "NOMEMO");
		return;
	}
}

command_t ns_set_nomemo = { "NOMEMO", N_("Disables the ability to recieve memos."), AC_NONE, 1, _ns_setnomemo };

static void _ns_setneverop(sourceinfo_t *si, int parc, char *parv[])
{
	char *params = strtok(parv[0], " ");

	if (si->smu == NULL)
		return;

	if (params == NULL)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "NEVEROP");
		return;
	}

	if (!strcasecmp("ON", params))
	{
		if (MU_NEVEROP & si->smu->flags)
		{
			command_fail(si, fault_nochange, _("The \2%s\2 flag is already set for account \2%s\2."), "NEVEROP", si->smu->name);
			return;
		}

		logcommand(si, CMDLOG_SET, "SET NEVEROP ON");

		si->smu->flags |= MU_NEVEROP;

		command_success_nodata(si, _("The \2%s\2 flag has been set for account \2%s\2."), "NEVEROP", si->smu->name);

		return;
	}

	else if (!strcasecmp("OFF", params))
	{
		if (!(MU_NEVEROP & si->smu->flags))
		{
			command_fail(si, fault_nochange, _("The \2%s\2 flag is not set for account \2%s\2."), "NEVEROP", si->smu->name);
			return;
		}

		logcommand(si, CMDLOG_SET, "SET NEVEROP OFF");

		si->smu->flags &= ~MU_NEVEROP;

		command_success_nodata(si, _("The \2%s\2 flag has been removed for account \2%s\2."), "NEVEROP", si->smu->name);

		return;
	}

	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "NEVEROP");
		return;
	}
}

command_t ns_set_neverop = { "NEVEROP", N_("Prevents you from being added to access lists."), AC_NONE, 1, _ns_setneverop };

static void _ns_setnoop(sourceinfo_t *si, int parc, char *parv[])
{
	char *params = strtok(parv[0], " ");

	if (si->smu == NULL)
		return;

	if (params == NULL)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "NOOP");
		return;
	}

	if (!strcasecmp("ON", params))
	{
		if (MU_NOOP & si->smu->flags)
		{
			command_fail(si, fault_nochange, _("The \2%s\2 flag is already set for account \2%s\2."), "NOOP", si->smu->name);
			return;
		}

		logcommand(si, CMDLOG_SET, "SET NOOP ON");

		si->smu->flags |= MU_NOOP;

		command_success_nodata(si, _("The \2%s\2 flag has been set for account \2%s\2."), "NOOP", si->smu->name);

		return;
	}
	else if (!strcasecmp("OFF", params))
	{
		if (!(MU_NOOP & si->smu->flags))
		{
			command_fail(si, fault_nochange, _("The \2%s\2 flag is not set for account \2%s\2."), "NOOP", si->smu->name);
			return;
		}

		logcommand(si, CMDLOG_SET, "SET NOOP OFF");

		si->smu->flags &= ~MU_NOOP;

		command_success_nodata(si, _("The \2%s\2 flag has been removed for account \2%s\2."), "NOOP", si->smu->name);

		return;
	}

	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "NOOP");
		return;
	}
}

command_t ns_set_noop = { "NOOP", N_("Prevents services from setting modes upon you automatically."), AC_NONE, 1, _ns_setnoop };

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
		snoop("SET:PROPERTY: \2%s\2: \2%s\2/\2%s\2", si->smu->name, property, value);

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
		hook_call_event("metadata_change", &mdchange);

		metadata_delete(si->smu, property);
		logcommand(si, CMDLOG_SET, "SET PROPERTY %s (deleted)", property);
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
		hook_call_event("metadata_change", &mdchange);
	}
	logcommand(si, CMDLOG_SET, "SET PROPERTY %s to %s", property, value);
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

	/*snoop("SET:PASSWORD: \2%s\2 as \2%s\2 for \2%s\2", si->su->user, si->smu->name, si->smu->name);*/
	logcommand(si, CMDLOG_SET, "SET PASSWORD");

	set_password(si->smu, password);

	command_success_nodata(si, _("The password for \2%s\2 has been changed to \2%s\2."), si->smu->name, password);

	return;
}

command_t ns_set_password = { "PASSWORD", N_("Changes the password associated with your account."), AC_NONE, 1, _ns_setpassword };

command_t *ns_set_commands[] = {
	&ns_set_email,
	&ns_set_emailmemos,
	&ns_set_hidemail,
	&ns_set_quietchg,
	&ns_set_nomemo,
	&ns_set_noop,
	&ns_set_neverop,
	&ns_set_password,
	&ns_set_property,
	NULL
};

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(ns_cmdtree, "nickserv/main", "ns_cmdtree");
	MODULE_USE_SYMBOL(ns_helptree, "nickserv/main", "ns_helptree");
	command_add(&ns_set, ns_cmdtree);

	help_addentry(ns_helptree, "SET", NULL, ns_help_set);
	help_addentry(ns_helptree, "SET EMAIL", "help/nickserv/set_email", NULL);
	help_addentry(ns_helptree, "SET EMAILMEMOS", "help/nickserv/set_emailmemos", NULL);
	help_addentry(ns_helptree, "SET HIDEMAIL", "help/nickserv/set_hidemail", NULL);
	help_addentry(ns_helptree, "SET NOMEMO", "help/nickserv/set_nomemo", NULL);
	help_addentry(ns_helptree, "SET NEVEROP", "help/nickserv/set_neverop", NULL);
	help_addentry(ns_helptree, "SET QUIETCHG", "help/nickserv/set_quietchg", NULL);
	help_addentry(ns_helptree, "SET NOOP", "help/nickserv/set_noop", NULL);
	help_addentry(ns_helptree, "SET PASSWORD", "help/nickserv/set_password", NULL);
	help_addentry(ns_helptree, "SET PROPERTY", "help/nickserv/set_property", NULL);

	/* populate ns_set_cmdtree */
	command_add_many(ns_set_commands, &ns_set_cmdtree);
}

void _moddeinit()
{
	command_delete(&ns_set, ns_cmdtree);
	help_delentry(ns_helptree, "SET");
	help_delentry(ns_helptree, "SET EMAIL");
	help_delentry(ns_helptree, "SET EMAILMEMOS");
	help_delentry(ns_helptree, "SET HIDEMAIL");
	help_delentry(ns_helptree, "SET NOMEMO");
	help_delentry(ns_helptree, "SET NEVEROP");
	help_delentry(ns_helptree, "SET NOOP");
	help_delentry(ns_helptree, "SET PASSWORD");
	help_delentry(ns_helptree, "SET PROPERTY");

	/* clear ns_set_cmdtree */
	command_delete_many(ns_set_commands, &ns_set_cmdtree);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
