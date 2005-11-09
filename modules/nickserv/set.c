/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains routines to handle the CService SET command.
 *
 * $Id: set.c 3685 2005-11-09 01:07:04Z alambert $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"nickserv/set", FALSE, _modinit, _moddeinit,
	"$Id: set.c 3685 2005-11-09 01:07:04Z alambert $",
	"Atheme Development Group <http://www.atheme.org>"
);

static struct set_command_ *ns_set_cmd_find(char *origin, char *command);

static void ns_cmd_set(char *origin);

list_t *ns_cmdtree, *ns_helptree;

command_t ns_set = { "SET", "Sets various control flags.", AC_NONE, ns_cmd_set };

void _modinit(module_t *m)
{
	ns_cmdtree = module_locate_symbol("nickserv/main", "ns_cmdtree");
	ns_helptree = module_locate_symbol("nickserv/main", "ns_helptree");
	command_add(&ns_set, ns_cmdtree);
	help_addentry(ns_helptree, "SET EMAIL", "help/nickserv/set_email", NULL);
	help_addentry(ns_helptree, "SET EMAILMEMOS", "help/nickserv/set_emailmemos", NULL);
	help_addentry(ns_helptree, "SET HIDEMAIL", "help/nickserv/set_hidemail", NULL);
	help_addentry(ns_helptree, "SET NOMEMO", "help/nickserv/set_nomemo", NULL);
	help_addentry(ns_helptree, "SET NEVEROP", "help/nickserv/set_neverop", NULL);
	help_addentry(ns_helptree, "SET NOOP", "help/nickserv/set_noop", NULL);
	help_addentry(ns_helptree, "SET PASSWORD", "help/nickserv/set_password", NULL);
	help_addentry(ns_helptree, "SET PROPERTY", "help/nickserv/set_property", NULL);

}

void _moddeinit()
{
	command_delete(&ns_set, ns_cmdtree);
	help_delentry(ns_helptree, "SET EMAIL");
	help_delentry(ns_helptree, "SET EMAILMEMOS");
	help_delentry(ns_helptree, "SET HIDEMAIL");
	help_delentry(ns_helptree, "SET NOMEMO");
	help_delentry(ns_helptree, "SET NEVEROP");
	help_delentry(ns_helptree, "SET NOOP");
	help_delentry(ns_helptree, "SET PASSWORD");
	help_delentry(ns_helptree, "SET PROPERTY");

}

/* SET <setting> <parameters> */
static void ns_cmd_set(char *origin)
{
	char *setting = strtok(NULL, " ");
	char *params = strtok(NULL, "");
	struct set_command_ *c;

	if (!setting || !params)
	{
		notice(nicksvs.nick, origin, "Insufficient parameters specified for \2SET\2.");
		notice(nicksvs.nick, origin, "Syntax: SET <setting> <parameters>");
		return;
	}

	/* take the command through the hash table */
	if ((c = ns_set_cmd_find(origin, setting)))
	{
		if (c->func)
			c->func(origin, origin, params);
		else
			notice(nicksvs.nick, origin, "Invalid setting.  Please use \2HELP\2 for help.");
	}
}

static void ns_set_email(char *origin, char *name, char *params)
{
	user_t *u = user_find(origin);
	char *email = strtok(params, " ");
	myuser_t *mu;

	if (!(mu = myuser_find(name)))
	{
		notice(nicksvs.nick, origin, "\2%s\2 is not registered.", name);
		return;
	}

	if (mu != u->myuser)
	{
		notice(nicksvs.nick, origin, "You are not authorized to perform this command.");
		return;
	}

	if (!email)
	{
		notice(nicksvs.nick, origin, "Insufficient parameters specified for \2EMAIL\2.");
		notice(nicksvs.nick, origin, "Syntax: SET EMAIL <new e-mail>");
		return;
	}

	if (strlen(email) >= EMAILLEN)
	{
		notice(nicksvs.nick, origin, "Invalid parameters specified for \2EMAIL\2.");
		return;
	}

	if (mu->flags & MU_WAITAUTH)
	{
		notice(nicksvs.nick, origin, "Please verify your original registration before changing your e-mail address.");
		return;
	}

	if (!validemail(email))
	{
		notice(nicksvs.nick, origin, "\2%s\2 is not a valid email address.", email);
		return;
	}

	if (!strcasecmp(mu->email, email))
	{
		notice(nicksvs.nick, origin, "The email address for \2%s\2 is already set to \2%s\2.", mu->name, mu->email);
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
			notice(nicksvs.nick, origin, "Sending email failed, sorry! Your email address is unchanged.");
			metadata_delete(mu, METADATA_USER, "private:verify:emailchg:key");
			metadata_delete(mu, METADATA_USER, "private:verify:emailchg:newemail");
			metadata_delete(mu, METADATA_USER, "private:verify:emailchg:timestamp");
			return;
		}

		logcommand(nicksvs.me, u, CMDLOG_SET, "SET EMAIL %s (awaiting verification)", email);
		notice(nicksvs.nick, origin, "An email containing email changing instructions " "has been sent to \2%s\2.", email);
		notice(nicksvs.nick, origin, "Your email address will not be changed until you follow " "these instructions.");

		return;
	}

	strlcpy(mu->email, email, EMAILLEN);

	logcommand(nicksvs.me, u, CMDLOG_SET, "SET EMAIL %s", email);
	notice(nicksvs.nick, origin, "The email address for \2%s\2 has been changed to \2%s\2.", mu->name, mu->email);
}

static void ns_set_hidemail(char *origin, char *name, char *params)
{
	user_t *u = user_find(origin);
	myuser_t *mu;

	if (!(mu = myuser_find(name)))
	{
		notice(nicksvs.nick, origin, "\2%s\2 is not registered.", name);
		return;
	}

	if (mu != u->myuser)
	{
		notice(nicksvs.nick, origin, "You are not authorized to perform this command.");
		return;
	}


	if (!strcasecmp("ON", params))
	{
		if (MU_HIDEMAIL & mu->flags)
		{
			notice(nicksvs.nick, origin, "The \2HIDEMAIL\2 flag is already set for \2%s\2.", mu->name);
			return;
		}

		snoop("SET:HIDEMAIL:ON: for \2%s\2", mu->name);
		logcommand(nicksvs.me, u, CMDLOG_SET, "SET HIDEMAIL ON");

		mu->flags |= MU_HIDEMAIL;

		notice(nicksvs.nick, origin, "The \2HIDEMAIL\2 flag has been set for \2%s\2.", mu->name);

		return;
	}

	else if (!strcasecmp("OFF", params))
	{
		if (!(MU_HIDEMAIL & mu->flags))
		{
			notice(nicksvs.nick, origin, "The \2HIDEMAIL\2 flag is not set for \2%s\2.", mu->name);
			return;
		}

		snoop("SET:HIDEMAIL:OFF: for \2%s\2", mu->name);
		logcommand(nicksvs.me, u, CMDLOG_SET, "SET HIDEMAIL OFF");

		mu->flags &= ~MU_HIDEMAIL;

		notice(nicksvs.nick, origin, "The \2HIDEMAIL\2 flag has been removed for \2%s\2.", mu->name);

		return;
	}

	else
	{
		notice(nicksvs.nick, origin, "Invalid parameters specified for \2HIDEMAIL\2.");
		return;
	}
}

static void ns_set_emailmemos(char *origin, char *name, char *params)
{
	user_t *u = user_find(origin);
	myuser_t *mu;

	if (!(mu = myuser_find(name)))
	{
		notice(nicksvs.nick,origin, "\2%s\2 is not registered.");
		return;
	}

	if (u->myuser != mu)
	{
		notice(nicksvs.nick, origin, "You are not authorized to perform this command.");
		return;
	}

	if (mu->flags & MU_WAITAUTH)
	{
		notice(nicksvs.nick, origin, "You have to verify your email address before you can enable emailing memos.");
		return;
	}

	if (!strcasecmp("ON", params))
	{
		if (MU_EMAILMEMOS & mu->flags)
		{
			notice(nicksvs.nick, origin, "The \2EMAILMEMOS\2 flag is already set for \2%s\2.", mu->name);
			return;
		}

		snoop("SET:EMAILMEMOS:ON: for \2%s\2 by \2%s\2", mu->name, origin);
		logcommand(nicksvs.me, u, CMDLOG_SET, "SET EMAILMEMOS ON");
		mu->flags |= MU_EMAILMEMOS;
		notice(nicksvs.nick, origin, "The \2EMAILMEMOS\2 flag has been set for \2%s\2.", mu->name);
		return;
	}

        else if (!strcasecmp("OFF", params))
        {
                if (!(MU_EMAILMEMOS & mu->flags))
                {
                        notice(nicksvs.nick, origin, "The \2EMAILMEMOS\2 flag is not set for \2%s\2.", mu->name);
                        return;
                }

                snoop("SET:EMAILMEMOS:OFF: for \2%s\2 by \2%s\2", mu->name, origin);
		logcommand(nicksvs.me, u, CMDLOG_SET, "SET EMAILMEMOS OFF");
                mu->flags &= ~MU_EMAILMEMOS;
                notice(nicksvs.nick, origin, "The \2EMAILMEMOS\2 flag has been removed for \2%s\2.", mu->name);
                return;
        }

        else
        {
                notice(nicksvs.nick, origin, "Invalid parameters specified for \2EMAILMEMOS\2.");
                return;
        }
}

	

static void ns_set_nomemo(char *origin, char *name, char *params)
{
	user_t *u = user_find(origin);
	myuser_t *mu;

	if (!(mu = myuser_find(name)))
	{
		notice(nicksvs.nick,origin, "\2%s\2 is not registered.");
		return;
	}

	if (u->myuser != mu)
	{
		notice(nicksvs.nick, origin, "You are not authorized to perform this command.");
		return;
	}

	if (!strcasecmp("ON", params))
	{
		if (MU_NOMEMO & mu->flags)
		{
			notice(nicksvs.nick, origin, "The \2NOMEMO\2 flag is already set for \2%s\2.", mu->name);
			return;
                }

		snoop("SET:NOMEMO:ON: for \2%s\2 by \2%s\2", mu->name, origin);
		logcommand(nicksvs.me, u, CMDLOG_SET, "SET NOMEMO ON");
                mu->flags |= MU_NOMEMO;
                notice(nicksvs.nick, origin, "The \2NOMEMO\2 flag has been set for \2%s\2.", mu->name);
                return;
	}

	else if (!strcasecmp("OFF", params))
	{
		if (!(MU_NOMEMO & mu->flags))
		{
			notice(nicksvs.nick, origin, "The \2NOMEMO\2 flag is not set for \2%s\2.", mu->name);
			return;
		}

		snoop("SET:NOMEMO:OFF: for \2%s\2 by \2%s\2", mu->name, origin);
		logcommand(nicksvs.me, u, CMDLOG_SET, "SET NOMEMO OFF");
		mu->flags &= ~MU_NOMEMO;
		notice(nicksvs.nick, origin, "The \2NOMEMO\2 flag has been removed for \2%s\2.", mu->name);
		return;
        }
	
	else
	{
		notice(nicksvs.nick, origin, "Invalid parameters specified for \2NOMEMO\2.");
		return;
	}
}


static void ns_set_neverop(char *origin, char *name, char *params)
{
	user_t *u = user_find(origin);
	myuser_t *mu;

	if (!(mu = myuser_find(name)))
	{
		notice(nicksvs.nick, origin, "\2%s\2 is not registered.", name);
		return;
	}

	if (u->myuser != mu)
	{
		notice(nicksvs.nick, origin, "You are not authorized to perform this command.");
		return;
	}

	if (!strcasecmp("ON", params))
	{
		if (MU_NEVEROP & mu->flags)
		{
			notice(nicksvs.nick, origin, "The \2NEVEROP\2 flag is already set for \2%s\2.", mu->name);
			return;
		}

		snoop("SET:NEVEROP:ON: for \2%s\2 by \2%s\2", mu->name, origin);
		logcommand(nicksvs.me, u, CMDLOG_SET, "SET NEVEROP ON");

		mu->flags |= MU_NEVEROP;

		notice(nicksvs.nick, origin, "The \2NEVEROP\2 flag has been set for \2%s\2.", mu->name);

		return;
	}

	else if (!strcasecmp("OFF", params))
	{
		if (!(MU_NEVEROP & mu->flags))
		{
			notice(nicksvs.nick, origin, "The \2NEVEROP\2 flag is not set for \2%s\2.", mu->name);
			return;
		}

		snoop("SET:NEVEROP:OFF: for \2%s\2 by \2%s\2", mu->name, origin);
		logcommand(nicksvs.me, u, CMDLOG_SET, "SET NEVEROP OFF");

		mu->flags &= ~MU_NEVEROP;

		notice(nicksvs.nick, origin, "The \2NEVEROP\2 flag has been removed for \2%s\2.", mu->name);

		return;
	}

	else
	{
		notice(nicksvs.nick, origin, "Invalid parameters specified for \2NEVEROP\2.");
		return;
	}
}

static void ns_set_noop(char *origin, char *name, char *params)
{
	user_t *u = user_find(origin);
	myuser_t *mu;

	if (!(mu = myuser_find(name)))
	{
		notice(nicksvs.nick, origin, "\2%s\2 is not registered.", name);
		return;
	}

	if (mu != u->myuser)
	{
		notice(nicksvs.nick, origin, "You are not authorized to perform this command.");
		return;
	}


	if (!strcasecmp("ON", params))
	{
		if (MU_NOOP & mu->flags)
		{
			notice(nicksvs.nick, origin, "The \2NOOP\2 flag is already set for \2%s\2.", mu->name);
			return;
		}

		snoop("SET:NOOP:ON: for \2%s\2", mu->name);
		logcommand(nicksvs.me, u, CMDLOG_SET, "SET NOOP ON");

		mu->flags |= MU_NOOP;

		notice(nicksvs.nick, origin, "The \2NOOP\2 flag has been set for \2%s\2.", mu->name);

		return;
	}

	else if (!strcasecmp("OFF", params))
	{
		if (!(MU_NOOP & mu->flags))
		{
			notice(nicksvs.nick, origin, "The \2NOOP\2 flag is not set for \2%s\2.", mu->name);
			return;
		}

		snoop("SET:NOOP:OFF: for \2%s\2", mu->name);
		logcommand(nicksvs.me, u, CMDLOG_SET, "SET NOOP OFF");

		mu->flags &= ~MU_NOOP;

		notice(nicksvs.nick, origin, "The \2NOOP\2 flag has been removed for \2%s\2.", mu->name);

		return;
	}

	else
	{
		notice(nicksvs.nick, origin, "Invalid parameters specified for \2NOOP\2.");
		return;
	}
}

static void ns_set_property(char *origin, char *name, char *params)
{
	user_t *u = user_find(origin);
	myuser_t *mu;
	char *property = strtok(params, " ");
	char *value = strtok(NULL, "");

	if (!(mu = myuser_find(name)))
	{
		notice(nicksvs.nick, origin, "\2%s\2 is not registered.", name);
		return;
	}

	if (u->myuser != mu)
	{
		notice(nicksvs.nick, origin, "You are not authorized to perform this command.");
		return;
	}

	if (!property)
	{
		notice(nicksvs.nick, origin, "Syntax: SET PROPERTY <property> [value]");
		return;
	}

	if (strchr(property, ':') && !is_ircop(u) && !is_sra(mu))
	{
		notice(nicksvs.nick, origin, "Invalid property name.");
		return;
	}

	snoop("SET:PROPERTY: \2%s\2: \2%s\2/\2%s\2", mu->name, property, value);

	if (mu->metadata.count >= me.mdlimit)
	{
		notice(nicksvs.nick, origin, "Cannot add \2%s\2 to \2%s\2 metadata table, it is full.",
					property, name);
		return;
	}

	if (!value)
	{
		metadata_t *md = metadata_find(mu, METADATA_USER, property);

		if (!md)
		{
			notice(nicksvs.nick, origin, "Metadata entry \2%s\2 was not set.", property);
			return;
		}

		metadata_delete(mu, METADATA_USER, property);
		logcommand(nicksvs.me, u, CMDLOG_SET, "SET PROPERTY %s (deleted)", property);
		notice(nicksvs.nick, origin, "Metadata entry \2%s\2 has been deleted.", property);
		return;
	}

	if (strlen(property) > 32 || strlen(value) > 300)
	{
		notice(nicksvs.nick, origin, "Parameters are too long. Aborting.");
		return;
	}

	metadata_add(mu, METADATA_USER, property, value);
	logcommand(nicksvs.me, u, CMDLOG_SET, "SET PROPERTY %s to %s", property, value);
	notice(nicksvs.nick, origin, "Metadata entry \2%s\2 added.", property);
}

static void ns_set_password(char *origin, char *name, char *params)
{
	char *password = strtok(params, " ");
	user_t *u = user_find(origin);
	myuser_t *mu;

	if (!(mu = myuser_find(name)))
	{
		notice(nicksvs.nick, origin, "\2%s\2 is not registered.", name);
		return;
	}

	if (u->myuser != mu)
	{
		notice(nicksvs.nick, origin, "You are not authorized to perform this command.");
		return;
	}

	if (strlen(password) > 32)
	{
		notice(nicksvs.nick, origin, "Invalid parameters specified for \2PASSWORD\2.");
		return;
	}

	if (!strcasecmp(password, name))
	{
		notice(nicksvs.nick, origin, "You cannot use your nickname as a password.");
		notice(nicksvs.nick, origin, "Syntax: SET PASSWORD <new password>");
		return;
	}

	snoop("SET:PASSWORD: \2%s\2 as \2%s\2 for \2%s\2", u->nick, mu->name, mu->name);
	logcommand(nicksvs.me, u, CMDLOG_SET, "SET PASSWORD");

	set_password(mu, password);

	notice(nicksvs.nick, origin, "The password for \2%s\2 has been changed to \2%s\2. " "Please write this down for future reference.", mu->name, password);

	return;
}

/* *INDENT-OFF* */

/* commands we understand */
static struct set_command_ ns_set_commands[] = {
  { "EMAIL",      AC_NONE,  ns_set_email      },
  { "HIDEMAIL",   AC_NONE,  ns_set_hidemail   },
  { "NEVEROP",    AC_NONE,  ns_set_neverop    },
  { "NOOP",       AC_NONE,  ns_set_noop       },
  { "PASSWORD",   AC_NONE,  ns_set_password   },
  { "PROPERTY",   AC_NONE,  ns_set_property   },
  { "NOMEMO",	  AC_NONE,  ns_set_nomemo     },
  { "EMAILMEMOS", AC_NONE,  ns_set_emailmemos },
  { NULL, 0, NULL }
};

/* *INDENT-ON* */

static struct set_command_ *ns_set_cmd_find(char *origin, char *command)
{
	user_t *u = user_find(origin);
	struct set_command_ *c;

	for (c = ns_set_commands; c->name; c++)
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
				notice(nicksvs.nick, origin, "You are not authorized to perform this operation.");
				return NULL;
			}
		}
	}

	/* it's a command we don't understand */
	notice(nicksvs.nick, origin, "Invalid command. Please use \2HELP\2 for help.");
	return NULL;
}
