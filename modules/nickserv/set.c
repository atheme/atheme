/*
 * Copyright (c) 2006 William Pitcock, et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains routines to handle the CService SET command.
 *
 * $Id: set.c 5830 2006-07-10 10:30:21Z nenolod $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"nickserv/set", FALSE, _modinit, _moddeinit,
	"$Id: set.c 5830 2006-07-10 10:30:21Z nenolod $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void ns_cmd_set(char *origin);

list_t *ns_cmdtree, *ns_helptree;

command_t ns_set = { "SET", "Sets various control flags.", AC_NONE, ns_cmd_set };

list_t ns_set_cmdtree;

/* HELP SET */
static void ns_help_set(char *origin)
{
	notice(nicksvs.nick, origin, "Help for \2SET\2:");
	notice(nicksvs.nick, origin, " ");
	notice(nicksvs.nick, origin, "SET allows you to set various control flags");
	notice(nicksvs.nick, origin, "for nicknames that change the way certain operations");
	notice(nicksvs.nick, origin, "are performed on them.");
	notice(nicksvs.nick, origin, " ");
	command_help(nicksvs.nick, origin, &ns_set_cmdtree);
	notice(nicksvs.nick, origin, " ");
	notice(nicksvs.nick, origin, "For more information, use \2/msg %s HELP SET \37command\37\2.", nicksvs.nick);
}

/* SET <setting> <parameters> */
static void ns_cmd_set(char *origin)
{
	char *setting = strtok(NULL, " ");
	user_t *u;

	u = user_find_named(origin);
	if (u == NULL)
		return;
	if (u->myuser == NULL)
	{
		notice(nicksvs.nick, origin, "You are not logged in.");
		return;
	}

	if (setting == NULL)
	{
		notice(nicksvs.nick, origin, STR_INSUFFICIENT_PARAMS, "SET");
		notice(nicksvs.nick, origin, "Syntax: SET <setting> <parameters>");
		return;
	}

        /* take the command through the hash table */
        command_exec(nicksvs.me, origin, setting, &ns_set_cmdtree);
}

/* SET EMAIL <new address> */
static void _ns_setemail(char *origin)
{
	user_t *u = user_find_named(origin);
	char *email = strtok(NULL, " ");

	if (u == NULL || u->myuser == NULL)
		return;

	if (email == NULL)
	{
		notice(nicksvs.nick, origin, STR_INSUFFICIENT_PARAMS, "EMAIL");
		notice(nicksvs.nick, origin, "Syntax: SET EMAIL <new e-mail>");
		return;
	}

	if (strlen(email) >= EMAILLEN)
	{
		notice(nicksvs.nick, origin, STR_INVALID_PARAMS, "EMAIL");
		return;
	}

	if (u->myuser->flags & MU_WAITAUTH)
	{
		notice(nicksvs.nick, origin, "Please verify your original registration before changing your e-mail address.");
		return;
	}

	if (!validemail(email))
	{
		notice(nicksvs.nick, origin, "\2%s\2 is not a valid email address.", email);
		return;
	}

	if (!strcasecmp(u->myuser->email, email))
	{
		notice(nicksvs.nick, origin, "The email address for \2%s\2 is already set to \2%s\2.", u->myuser->name, u->myuser->email);
		return;
	}

	snoop("SET:EMAIL: \2%s\2 (\2%s\2 -> \2%s\2)", u->myuser->name, u->myuser->email, email);

	if (me.auth == AUTH_EMAIL)
	{
		unsigned long key = makekey();

		metadata_add(u->myuser, METADATA_USER, "private:verify:emailchg:key", itoa(key));
		metadata_add(u->myuser, METADATA_USER, "private:verify:emailchg:newemail", email);
		metadata_add(u->myuser, METADATA_USER, "private:verify:emailchg:timestamp", itoa(time(NULL)));

		if (!sendemail(u, EMAIL_SETEMAIL, u->myuser, itoa(key)))
		{
			notice(nicksvs.nick, origin, "Sending email failed, sorry! Your email address is unchanged.");
			metadata_delete(u->myuser, METADATA_USER, "private:verify:emailchg:key");
			metadata_delete(u->myuser, METADATA_USER, "private:verify:emailchg:newemail");
			metadata_delete(u->myuser, METADATA_USER, "private:verify:emailchg:timestamp");
			return;
		}

		logcommand(nicksvs.me, u, CMDLOG_SET, "SET EMAIL %s (awaiting verification)", email);
		notice(nicksvs.nick, origin, "An email containing email changing instructions has been sent to \2%s\2.", email);
		notice(nicksvs.nick, origin, "Your email address will not be changed until you follow these instructions.");

		return;
	}

	strlcpy(u->myuser->email, email, EMAILLEN);

	logcommand(nicksvs.me, u, CMDLOG_SET, "SET EMAIL %s", email);
	notice(nicksvs.nick, origin, "The email address for \2%s\2 has been changed to \2%s\2.", u->myuser->name, u->myuser->email);
}

command_t ns_set_email = { "EMAIL", "Changes the e-mail address associated with a nickname.", AC_NONE, _ns_setemail };

/* SET HIDEMAIL [ON|OFF] */
static void _ns_sethidemail(char *origin)
{
	user_t *u = user_find_named(origin);
	char *params = strtok(NULL, " ");

	if (u == NULL || u->myuser == NULL)
		return;

	if (params == NULL)
	{
		notice(nicksvs.nick, origin, STR_INSUFFICIENT_PARAMS, "HIDEMAIL");
		return;
	}

	if (!strcasecmp("ON", params))
	{
		if (MU_HIDEMAIL & u->myuser->flags)
		{
			notice(nicksvs.nick, origin, "The \2HIDEMAIL\2 flag is already set for \2%s\2.", u->myuser->name);
			return;
		}

		logcommand(nicksvs.me, u, CMDLOG_SET, "SET HIDEMAIL ON");

		u->myuser->flags |= MU_HIDEMAIL;

		notice(nicksvs.nick, origin, "The \2HIDEMAIL\2 flag has been set for \2%s\2.", u->myuser->name);

		return;
	}
	else if (!strcasecmp("OFF", params))
	{
		if (!(MU_HIDEMAIL & u->myuser->flags))
		{
			notice(nicksvs.nick, origin, "The \2HIDEMAIL\2 flag is not set for \2%s\2.", u->myuser->name);
			return;
		}

		logcommand(nicksvs.me, u, CMDLOG_SET, "SET HIDEMAIL OFF");

		u->myuser->flags &= ~MU_HIDEMAIL;

		notice(nicksvs.nick, origin, "The \2HIDEMAIL\2 flag has been removed for \2%s\2.", u->myuser->name);

		return;
	}

	else
	{
		notice(nicksvs.nick, origin, STR_INVALID_PARAMS, "HIDEMAIL");
		return;
	}
}

command_t ns_set_hidemail = { "HIDEMAIL", "Hides the e-mail address associated with a nickname.", AC_NONE, _ns_sethidemail };

static void _ns_setemailmemos(char *origin)
{
	user_t *u = user_find_named(origin);
	char *params = strtok(NULL, " ");

	if (u == NULL || u->myuser == NULL)
		return;

	if (u->myuser->flags & MU_WAITAUTH)
	{
		notice(nicksvs.nick, origin, "You have to verify your email address before you can enable emailing memos.");
		return;
	}

	if (params == NULL)
	{
		notice(nicksvs.nick, origin, STR_INSUFFICIENT_PARAMS, "EMAILMEMOS");
		return;
	}

	if (!strcasecmp("ON", params))
	{
		if (me.mta == NULL)
		{
			notice(nicksvs.nick, origin, "Sending email is administratively disabled.");
			return;
		}
		if (MU_EMAILMEMOS & u->myuser->flags)
		{
			notice(nicksvs.nick, origin, "The \2EMAILMEMOS\2 flag is already set for \2%s\2.", u->myuser->name);
			return;
		}

		logcommand(nicksvs.me, u, CMDLOG_SET, "SET EMAILMEMOS ON");
		u->myuser->flags |= MU_EMAILMEMOS;
		notice(nicksvs.nick, origin, "The \2EMAILMEMOS\2 flag has been set for \2%s\2.", u->myuser->name);
		return;
	}

        else if (!strcasecmp("OFF", params))
        {
                if (!(MU_EMAILMEMOS & u->myuser->flags))
                {
                        notice(nicksvs.nick, origin, "The \2EMAILMEMOS\2 flag is not set for \2%s\2.", u->myuser->name);
                        return;
                }

		logcommand(nicksvs.me, u, CMDLOG_SET, "SET EMAILMEMOS OFF");
                u->myuser->flags &= ~MU_EMAILMEMOS;
                notice(nicksvs.nick, origin, "The \2EMAILMEMOS\2 flag has been removed for \2%s\2.", u->myuser->name);
                return;
        }

        else
        {
                notice(nicksvs.nick, origin, STR_INVALID_PARAMS, "EMAILMEMOS");
                return;
        }
}

command_t ns_set_emailmemos = { "EMAILMEMOS", "Forwards incoming memos to your account's e-mail address.", AC_NONE, _ns_setemailmemos };

static void _ns_setnomemo(char *origin)
{
	user_t *u = user_find_named(origin);
	char *params = strtok(NULL, " ");

	if (u == NULL || u->myuser == NULL)
		return;

	if (params == NULL)
	{
		notice(nicksvs.nick, origin, STR_INSUFFICIENT_PARAMS, "NOMEMO");
		return;
	}

	if (!strcasecmp("ON", params))
	{
		if (MU_NOMEMO & u->myuser->flags)
		{
			notice(nicksvs.nick, origin, "The \2NOMEMO\2 flag is already set for \2%s\2.", u->myuser->name);
			return;
                }

		logcommand(nicksvs.me, u, CMDLOG_SET, "SET NOMEMO ON");
                u->myuser->flags |= MU_NOMEMO;
                notice(nicksvs.nick, origin, "The \2NOMEMO\2 flag has been set for \2%s\2.", u->myuser->name);
                return;
	}

	else if (!strcasecmp("OFF", params))
	{
		if (!(MU_NOMEMO & u->myuser->flags))
		{
			notice(nicksvs.nick, origin, "The \2NOMEMO\2 flag is not set for \2%s\2.", u->myuser->name);
			return;
		}

		logcommand(nicksvs.me, u, CMDLOG_SET, "SET NOMEMO OFF");
		u->myuser->flags &= ~MU_NOMEMO;
		notice(nicksvs.nick, origin, "The \2NOMEMO\2 flag has been removed for \2%s\2.", u->myuser->name);
		return;
        }
	
	else
	{
		notice(nicksvs.nick, origin, STR_INVALID_PARAMS, "NOMEMO");
		return;
	}
}

command_t ns_set_nomemo = { "NOMEMO", "Disables the ability to recieve memos.", AC_NONE, _ns_setnomemo };

static void _ns_setneverop(char *origin)
{
	user_t *u = user_find_named(origin);
	char *params = strtok(NULL, " ");

	if (u == NULL || u->myuser == NULL)
		return;

	if (params == NULL)
	{
		notice(nicksvs.nick, origin, STR_INSUFFICIENT_PARAMS, "NEVEROP");
		return;
	}

	if (!strcasecmp("ON", params))
	{
		if (MU_NEVEROP & u->myuser->flags)
		{
			notice(nicksvs.nick, origin, "The \2NEVEROP\2 flag is already set for \2%s\2.", u->myuser->name);
			return;
		}

		logcommand(nicksvs.me, u, CMDLOG_SET, "SET NEVEROP ON");

		u->myuser->flags |= MU_NEVEROP;

		notice(nicksvs.nick, origin, "The \2NEVEROP\2 flag has been set for \2%s\2.", u->myuser->name);

		return;
	}

	else if (!strcasecmp("OFF", params))
	{
		if (!(MU_NEVEROP & u->myuser->flags))
		{
			notice(nicksvs.nick, origin, "The \2NEVEROP\2 flag is not set for \2%s\2.", u->myuser->name);
			return;
		}

		logcommand(nicksvs.me, u, CMDLOG_SET, "SET NEVEROP OFF");

		u->myuser->flags &= ~MU_NEVEROP;

		notice(nicksvs.nick, origin, "The \2NEVEROP\2 flag has been removed for \2%s\2.", u->myuser->name);

		return;
	}

	else
	{
		notice(nicksvs.nick, origin, STR_INVALID_PARAMS, "NEVEROP");
		return;
	}
}

command_t ns_set_neverop = { "NEVEROP", "Prevents you from being added to access lists.", AC_NONE, _ns_setneverop };

static void _ns_setnoop(char *origin)
{
	user_t *u = user_find_named(origin);
	char *params = strtok(NULL, " ");

	if (u == NULL || u->myuser == NULL)
		return;

	if (params == NULL)
	{
		notice(nicksvs.nick, origin, STR_INSUFFICIENT_PARAMS, "NOOP");
		return;
	}

	if (!strcasecmp("ON", params))
	{
		if (MU_NOOP & u->myuser->flags)
		{
			notice(nicksvs.nick, origin, "The \2NOOP\2 flag is already set for \2%s\2.", u->myuser->name);
			return;
		}

		logcommand(nicksvs.me, u, CMDLOG_SET, "SET NOOP ON");

		u->myuser->flags |= MU_NOOP;

		notice(nicksvs.nick, origin, "The \2NOOP\2 flag has been set for \2%s\2.", u->myuser->name);

		return;
	}
	else if (!strcasecmp("OFF", params))
	{
		if (!(MU_NOOP & u->myuser->flags))
		{
			notice(nicksvs.nick, origin, "The \2NOOP\2 flag is not set for \2%s\2.", u->myuser->name);
			return;
		}

		logcommand(nicksvs.me, u, CMDLOG_SET, "SET NOOP OFF");

		u->myuser->flags &= ~MU_NOOP;

		notice(nicksvs.nick, origin, "The \2NOOP\2 flag has been removed for \2%s\2.", u->myuser->name);

		return;
	}

	else
	{
		notice(nicksvs.nick, origin, STR_INVALID_PARAMS, "NOOP");
		return;
	}
}

command_t ns_set_noop = { "NOOP", "Prevents services from setting modes upon you automatically.", AC_NONE, _ns_setnoop };

static void _ns_setproperty(char *origin)
{
	user_t *u = user_find_named(origin);
	char *property = strtok(NULL, " ");
	char *value = strtok(NULL, "");

	if (u == NULL || u->myuser == NULL)
		return;

	if (!property)
	{
		notice(nicksvs.nick, origin, "Syntax: SET PROPERTY <property> [value]");
		return;
	}

	if (strchr(property, ':') && !has_priv(u, PRIV_METADATA))
	{
		notice(nicksvs.nick, origin, "Invalid property name.");
		return;
	}

	if (strchr(property, ':'))
		snoop("SET:PROPERTY: \2%s\2: \2%s\2/\2%s\2", u->myuser->name, property, value);

	if (u->myuser->metadata.count >= me.mdlimit)
	{
		notice(nicksvs.nick, origin, "Cannot add \2%s\2 to \2%s\2 metadata table, it is full.",
					property, u->myuser->name);
		return;
	}

	if (!value)
	{
		metadata_t *md = metadata_find(u->myuser, METADATA_USER, property);

		if (!md)
		{
			notice(nicksvs.nick, origin, "Metadata entry \2%s\2 was not set.", property);
			return;
		}

		metadata_delete(u->myuser, METADATA_USER, property);
		logcommand(nicksvs.me, u, CMDLOG_SET, "SET PROPERTY %s (deleted)", property);
		notice(nicksvs.nick, origin, "Metadata entry \2%s\2 has been deleted.", property);
		return;
	}

	if (strlen(property) > 32 || strlen(value) > 300)
	{
		notice(nicksvs.nick, origin, "Parameters are too long. Aborting.");
		return;
	}

	metadata_add(u->myuser, METADATA_USER, property, value);
	logcommand(nicksvs.me, u, CMDLOG_SET, "SET PROPERTY %s to %s", property, value);
	notice(nicksvs.nick, origin, "Metadata entry \2%s\2 added.", property);
}

command_t ns_set_property = { "PROPERTY", "Manipulates metadata entries associated with a nickname.", AC_NONE, _ns_setproperty };

static void _ns_setpassword(char *origin)
{
	char *password = strtok(NULL, " ");
	user_t *u = user_find_named(origin);

	if (u == NULL || u->myuser == NULL)
		return;

	if (password == NULL)
	{
		notice(nicksvs.nick, origin, STR_INSUFFICIENT_PARAMS, "PASSWORD");
		return;
	}

	if (strlen(password) > 32)
	{
		notice(nicksvs.nick, origin, STR_INVALID_PARAMS, "PASSWORD");
		return;
	}

	if (!strcasecmp(password, u->myuser->name))
	{
		notice(nicksvs.nick, origin, "You cannot use your nickname as a password.");
		notice(nicksvs.nick, origin, "Syntax: SET PASSWORD <new password>");
		return;
	}

	/*snoop("SET:PASSWORD: \2%s\2 as \2%s\2 for \2%s\2", u->nick, u->myuser->name, u->myuser->name);*/
	logcommand(nicksvs.me, u, CMDLOG_SET, "SET PASSWORD");

	set_password(u->myuser, password);

	notice(nicksvs.nick, origin, "The password for \2%s\2 has been changed to \2%s\2. Please write this down for future reference.", u->myuser->name, password);

	return;
}

command_t ns_set_password = { "PASSWORD", "Changes the password associated with your nickname.", AC_NONE, _ns_setpassword };

command_t *ns_set_commands[] = {
	&ns_set_email,
	&ns_set_emailmemos,
	&ns_set_hidemail,
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
