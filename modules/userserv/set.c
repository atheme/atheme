/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains routines to handle the CService SET command.
 *
 * $Id: set.c 3229 2005-10-28 21:17:04Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"userserv/set", FALSE, _modinit, _moddeinit,
	"$Id: set.c 3229 2005-10-28 21:17:04Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static struct set_command_ *us_set_cmd_find(char *origin, char *command);

static void us_cmd_set(char *origin);

list_t *us_cmdtree, *us_helptree;

command_t us_set = { "SET", "Sets various control flags.", AC_NONE, us_cmd_set };

void _modinit(module_t *m)
{
	us_cmdtree = module_locate_symbol("userserv/main", "us_cmdtree");
	us_helptree = module_locate_symbol("userserv/main", "us_helptree");
	command_add(&us_set, us_cmdtree);
	help_addentry(us_helptree, "SET EMAIL", "help/userserv/set_email", NULL);
	help_addentry(us_helptree, "SET EMAILMEMOS", "help/userserv/set_emailmemos", NULL);
	help_addentry(us_helptree, "SET HIDEMAIL", "help/userserv/set_hidemail", NULL);
	help_addentry(us_helptree, "SET NOMEMO", "help/userserv/set_nomemo", NULL);
	help_addentry(us_helptree, "SET NEVEROP", "help/userserv/set_neverop", NULL);
	help_addentry(us_helptree, "SET NOOP", "help/userserv/set_noop", NULL);
	help_addentry(us_helptree, "SET PASSWORD", "help/userserv/set_password", NULL);
	help_addentry(us_helptree, "SET PROPERTY", "help/userserv/set_property", NULL);
}

void _moddeinit()
{
	command_delete(&us_set, us_cmdtree);
	help_delentry(us_helptree, "SET EMAIL");
	help_delentry(us_helptree, "SET EMAILMEMOS");
	help_delentry(us_helptree, "SET HIDEMAIL");
	help_delentry(us_helptree, "SET NOMEMO");
	help_delentry(us_helptree, "SET NEVEROP");
	help_delentry(us_helptree, "SET NOOP");
	help_delentry(us_helptree, "SET PASSWORD");
	help_delentry(us_helptree, "SET PROPERTY");
}

/* SET <setting> <parameters> */
static void us_cmd_set(char *origin)
{
	char *target = strtok(NULL, " ");
	char *setting = strtok(NULL, " ");
	char *params = strtok(NULL, "");
	struct set_command_ *c;

	if (!target || !setting || !params)
	{
		notice(usersvs.nick, origin, "Insufficient parameters specified for \2SET\2.");
		notice(usersvs.nick, origin, "Syntax: SET <account> <setting> <parameters>");
		return;
	}

	/* take the command through the hash table */
	if ((c = us_set_cmd_find(origin, setting)))
	{
		if (c->func)
			c->func(origin, target, params);
		else
			notice(usersvs.nick, origin, "Invalid setting.  Please use \2HELP\2 for help.");
	}
}

static void us_set_email(char *origin, char *name, char *params)
{
	user_t *u = user_find(origin);
	char *email = strtok(params, " ");
	myuser_t *mu;

	if (!(mu = myuser_find(name)))
	{
		notice(usersvs.nick, origin, "\2%s\2 is not registered.", name);
		return;
	}

	if (mu != u->myuser)
	{
		notice(usersvs.nick, origin, "You are not authorized to perform this command.");
		return;
	}

	if (!email)
	{
		notice(usersvs.nick, origin, "Insufficient parameters specified for \2EMAIL\2.");
		notice(usersvs.nick, origin, "Syntax: SET EMAIL <new e-mail>");
		return;
	}

	if (strlen(email) >= EMAILLEN)
	{
		notice(usersvs.nick, origin, "Invalid parameters specified for \2EMAIL\2.");
		return;
	}

	if (mu->flags & MU_WAITAUTH)
	{
		notice(usersvs.nick, origin, "Please verify your original registration before changing your e-mail address.");
		return;
	}

	if (!validemail(email))
	{
		notice(usersvs.nick, origin, "\2%s\2 is not a valid email address.", email);
		return;
	}

	if (!strcasecmp(mu->email, email))
	{
		notice(usersvs.nick, origin, "The email address for \2%s\2 is already set to \2%s\2.", mu->name, mu->email);
		return;
	}

	snoop("SET:EMAIL: \2%s\2 (\2%s\2 -> \2%s\2)", mu->name, mu->email, email);

	if (me.auth == AUTH_EMAIL)
	{
		unsigned long key = makekey();

		metadata_add(mu, METADATA_USER, "private:verify:emailchg:key", itoa(key));
		metadata_add(mu, METADATA_USER, "private:verify:emailchg:newemail", email);
		metadata_add(mu, METADATA_USER, "private:verify:emailchg:timestamp", itoa(time(NULL)));

		if (!sendemail(u, EMAIL_SETEMAIL, mu, itoa(key)))
		{
			notice(usersvs.nick, origin, "Sending email failed, sorry! Your email address is unchanged.");
			metadata_delete(mu, METADATA_USER, "private:verify:emailchg:key");
			metadata_delete(mu, METADATA_USER, "private:verify:emailchg:newemail");
			metadata_delete(mu, METADATA_USER, "private:verify:emailchg:timestamp");
			return;
		}

		notice(usersvs.nick, origin, "An email containing email changing instructions " "has been sent to \2%s\2.", email);
		notice(usersvs.nick, origin, "Your email address will not be changed until you follow " "these instructions.");

		return;
	}

	strlcpy(mu->email, email, EMAILLEN);

	notice(usersvs.nick, origin, "The email address for \2%s\2 has been changed to \2%s\2.", mu->name, mu->email);
}

static void us_set_nomemo(char *origin, char *name, char *params)
{
        user_t *u = user_find(origin);
        myuser_t *mu;

        if (!(mu = myuser_find(name)))
        {
                notice(usersvs.nick,origin, "\2%s\2 is not registered.");
                return;
        }

        if (u->myuser != mu)
        {
                notice(usersvs.nick, origin, "You are not authorized to perform this command.");
                return;
        }
        if (!strcasecmp("ON", params))
        {
                if (MU_NOMEMO & mu->flags)
                {
                        notice(usersvs.nick, origin, "The \2NOMEMO\2 flag is already set for \2%s\2.", mu->name);
                        return;
                }

                snoop("SET:NOMEMO:ON: for \2%s\2 by \2%s\2", mu->name, origin);
                mu->flags |= MU_NOMEMO;
                notice(usersvs.nick, origin, "The \2NOMEMO\2 flag has been set for \2%s\2.", mu->name);
                return;
        }

        else if (!strcasecmp("OFF", params))
        {
                if (!(MU_NOMEMO & mu->flags))
                {
                        notice(usersvs.nick, origin, "The \2NOMEMO\2 flag is not set for \2%s\2.", mu->name);
                        return;
                }

                snoop("SET:NOMEMO:OFF: for \2%s\2 by \2%s\2", mu->name, origin);
                mu->flags &= ~MU_NOMEMO;
                notice(usersvs.nick, origin, "The \2NOMEMO\2 flag has been removed for \2%s\2.", mu->name);
                return;
        }
        else
        {
                notice(usersvs.nick, origin, "Invalid parameters specified for \2NOMEMO\2.");
                return;
        }
}

static void us_set_emailmemos(char *origin, char *name, char *params)
{
        user_t *u = user_find(origin);
        myuser_t *mu;

        if (!(mu = myuser_find(name)))
        {
                notice(usersvs.nick,origin, "\2%s\2 is not registered.");
                return;
        }

        if (u->myuser != mu)
        {
                notice(usersvs.nick, origin, "You are not authorized to perform this command.");
                return;
        }

        if (!strcasecmp("ON", params))
        {
                if (MU_EMAILMEMOS & mu->flags)
                {
                        notice(usersvs.nick, origin, "The \2EMAILMEMOS\2 flag is already set for \2%s\2.", mu->name);
                        return;
                }

                snoop("SET:EMAILMEMOS:ON: for \2%s\2 by \2%s\2", mu->name, origin);
                mu->flags |= MU_EMAILMEMOS;
                notice(usersvs.nick, origin, "The \2EMAILMEMOS\2 flag has been set for \2%s\2.", mu->name);
                return;
        }
        else if (!strcasecmp("OFF", params))
        {
                if (!(MU_EMAILMEMOS & mu->flags))
                {
                        notice(usersvs.nick, origin, "The \2EMAILMEMOS\2 flag is not set for \2%s\2.", mu->name);
                        return;
                }

                snoop("SET:EMAILMEMOS:OFF: for \2%s\2 by \2%s\2", mu->name, origin);
                mu->flags &= ~MU_EMAILMEMOS;
                notice(usersvs.nick, origin, "The \2EMAILMEMOS\2 flag has been removed for \2%s\2.", mu->name);
                return;
        }

        else
        {
                notice(usersvs.nick, origin, "Invalid parameters specified for \2EMAILMEMOS\2.");
                return;
        }
}

static void us_set_hidemail(char *origin, char *name, char *params)
{
	user_t *u = user_find(origin);
	myuser_t *mu;

	if (!(mu = myuser_find(name)))
	{
		notice(usersvs.nick, origin, "\2%s\2 is not registered.", name);
		return;
	}

	if (mu != u->myuser)
	{
		notice(usersvs.nick, origin, "You are not authorized to perform this command.");
		return;
	}


	if (!strcasecmp("ON", params))
	{
		if (MU_HIDEMAIL & mu->flags)
		{
			notice(usersvs.nick, origin, "The \2HIDEMAIL\2 flag is already set for \2%s\2.", mu->name);
			return;
		}

		snoop("SET:HIDEMAIL:ON: for \2%s\2", mu->name);

		mu->flags |= MU_HIDEMAIL;

		notice(usersvs.nick, origin, "The \2HIDEMAIL\2 flag has been set for \2%s\2.", mu->name);

		return;
	}

	else if (!strcasecmp("OFF", params))
	{
		if (!(MU_HIDEMAIL & mu->flags))
		{
			notice(usersvs.nick, origin, "The \2HIDEMAIL\2 flag is not set for \2%s\2.", mu->name);
			return;
		}

		snoop("SET:HIDEMAIL:OFF: for \2%s\2", mu->name);

		mu->flags &= ~MU_HIDEMAIL;

		notice(usersvs.nick, origin, "The \2HIDEMAIL\2 flag has been removed for \2%s\2.", mu->name);

		return;
	}

	else
	{
		notice(usersvs.nick, origin, "Invalid parameters specified for \2HIDEMAIL\2.");
		return;
	}
}

static void us_set_neverop(char *origin, char *name, char *params)
{
	user_t *u = user_find(origin);
	myuser_t *mu;

	if (!(mu = myuser_find(name)))
	{
		notice(usersvs.nick, origin, "\2%s\2 is not registered.", name);
		return;
	}

	if (u->myuser != mu)
	{
		notice(usersvs.nick, origin, "You are not authorized to perform this command.");
		return;
	}

	if (!strcasecmp("ON", params))
	{
		if (MU_NEVEROP & mu->flags)
		{
			notice(usersvs.nick, origin, "The \2NEVEROP\2 flag is already set for \2%s\2.", mu->name);
			return;
		}

		snoop("SET:NEVEROP:ON: for \2%s\2 by \2%s\2", mu->name, origin);

		mu->flags |= MU_NEVEROP;

		notice(usersvs.nick, origin, "The \2NEVEROP\2 flag has been set for \2%s\2.", mu->name);

		return;
	}

	else if (!strcasecmp("OFF", params))
	{
		if (!(MU_NEVEROP & mu->flags))
		{
			notice(usersvs.nick, origin, "The \2NEVEROP\2 flag is not set for \2%s\2.", mu->name);
			return;
		}

		snoop("SET:NEVEROP:OFF: for \2%s\2 by \2%s\2", mu->name, origin);

		mu->flags &= ~MU_NEVEROP;

		notice(usersvs.nick, origin, "The \2NEVEROP\2 flag has been removed for \2%s\2.", mu->name);

		return;
	}

	else
	{
		notice(usersvs.nick, origin, "Invalid parameters specified for \2NEVEROP\2.");
		return;
	}
}

static void us_set_noop(char *origin, char *name, char *params)
{
	user_t *u = user_find(origin);
	myuser_t *mu;

	if (!(mu = myuser_find(name)))
	{
		notice(usersvs.nick, origin, "\2%s\2 is not registered.", name);
		return;
	}

	if (mu != u->myuser)
	{
		notice(usersvs.nick, origin, "You are not authorized to perform this command.");
		return;
	}


	if (!strcasecmp("ON", params))
	{
		if (MU_NOOP & mu->flags)
		{
			notice(usersvs.nick, origin, "The \2NOOP\2 flag is already set for \2%s\2.", mu->name);
			return;
		}

		snoop("SET:NOOP:ON: for \2%s\2", mu->name);

		mu->flags |= MU_NOOP;

		notice(usersvs.nick, origin, "The \2NOOP\2 flag has been set for \2%s\2.", mu->name);

		return;
	}

	else if (!strcasecmp("OFF", params))
	{
		if (!(MU_NOOP & mu->flags))
		{
			notice(usersvs.nick, origin, "The \2NOOP\2 flag is not set for \2%s\2.", mu->name);
			return;
		}

		snoop("SET:NOOP:OFF: for \2%s\2", mu->name);

		mu->flags &= ~MU_NOOP;

		notice(usersvs.nick, origin, "The \2NOOP\2 flag has been removed for \2%s\2.", mu->name);

		return;
	}

	else
	{
		notice(usersvs.nick, origin, "Invalid parameters specified for \2NOOP\2.");
		return;
	}
}

static void us_set_property(char *origin, char *name, char *params)
{
	user_t *u = user_find(origin);
	myuser_t *mu;
	char *property = strtok(params, " ");
	char *value = strtok(NULL, "");

	if (!(mu = myuser_find(name)))
	{
		notice(usersvs.nick, origin, "\2%s\2 is not registered.", name);
		return;
	}

	if (u->myuser != mu)
	{
		notice(usersvs.nick, origin, "You are not authorized to perform this command.");
		return;
	}

	if (!property)
	{
		notice(usersvs.nick, origin, "Syntax: SET PROPERTY <property> [value]");
		return;
	}

	if (strchr(property, ':') && !is_ircop(u) && !is_sra(mu))
	{
		notice(usersvs.nick, origin, "Invalid property name.");
		return;
	}

	snoop("SET:PROPERTY: \2%s\2: \2%s\2/\2%s\2", mu->name, property, value);

	if (mu->metadata.count > me.mdlimit)
	{
		notice(usersvs.nick, origin, "Cannot add \2%s\2 to \2%s\2 metadata table, it is full.",
					property, name);
		return;
	}

	if (!value)
	{
		metadata_t *md = metadata_find(mu, METADATA_USER, property);

		if (!md)
		{
			notice(usersvs.nick, origin, "Metadata entry \2%s\2 was not set.", property);
			return;
		}

		metadata_delete(mu, METADATA_USER, property);
		notice(usersvs.nick, origin, "Metadata entry \2%s\2 has been deleted.", property);
		return;
	}

	if (strlen(property) > 32 || strlen(value) > 300)
	{
		notice(usersvs.nick, origin, "Parameters are too long. Aborting.");
		return;
	}

	metadata_add(mu, METADATA_USER, property, value);
	notice(usersvs.nick, origin, "Metadata entry \2%s\2 added.", property);
}

static void us_set_password(char *origin, char *name, char *params)
{
	char *password = strtok(params, " ");
	user_t *u = user_find(origin);
	myuser_t *mu;

	if (!(mu = myuser_find(name)))
	{
		notice(usersvs.nick, origin, "\2%s\2 is not registered.", name);
		return;
	}

	if (u->myuser != mu)
	{
		notice(usersvs.nick, origin, "You are not authorized to perform this command.");
		return;
	}

	if (strlen(password) > 32)
	{
		notice(usersvs.nick, origin, "Invalid parameters specified for \2PASSWORD\2.");
		return;
	}

	if (!strcasecmp(password, name))
	{
		notice(usersvs.nick, origin, "You cannot use your account name as a password.");
		notice(usersvs.nick, origin, "Syntax: SET PASSWORD <new password>");
		return;
	}

	snoop("SET:PASSWORD: \2%s\2 as \2%s\2 for \2%s\2", u->nick, mu->name, mu->name);

	strlcpy(mu->pass, password, NICKLEN);

	notice(usersvs.nick, origin, "The password for \2%s\2 has been changed to \2%s\2. " "Please write this down for future reference.", mu->name, mu->pass);

	return;
}

/* *INDENT-OFF* */

/* commands we understand */
static struct set_command_ us_set_commands[] = {
  { "EMAIL",      AC_NONE,  us_set_email      },
  { "EMAILMEMOS", AC_NONE,  us_set_emailmemos },
  { "HIDEMAIL",   AC_NONE,  us_set_hidemail   },
  { "NOMEMO",     AC_NONE,  us_set_nomemo     },
  { "NEVEROP",    AC_NONE,  us_set_neverop    },
  { "NOOP",       AC_NONE,  us_set_noop       },
  { "PASSWORD",   AC_NONE,  us_set_password   },
  { "PROPERTY",   AC_NONE,  us_set_property   },
  { NULL, 0, NULL }
};

/* *INDENT-ON* */

static struct set_command_ *us_set_cmd_find(char *origin, char *command)
{
	user_t *u = user_find(origin);
	struct set_command_ *c;

	for (c = us_set_commands; c->name; c++)
	{
		if (!strcasecmp(command, c->name))
		{
			/* no special access required, so go ahead... */
			if (c->access == AC_NONE)
				return c;

			/* sra? */
			if ((c->access == AC_SRA) && (is_sra(u->myuser)))
				return c;

			/* ircop? */
			if ((c->access == AC_IRCOP) && (is_ircop(u)))
				return c;

			/* otherwise... */
			else
			{
				notice(usersvs.nick, origin, "You are not authorized to perform this operation.");
				return NULL;
			}
		}
	}

	/* it's a command we don't understand */
	notice(usersvs.nick, origin, "Invalid command. Please use \2HELP\2 for help.");
	return NULL;
}
