/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService SENDPASS function.
 *
 * $Id: sendpass.c 3383 2005-11-01 09:16:16Z pfish $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"userserv/sendpass", FALSE, _modinit, _moddeinit,
	"$Id: sendpass.c 3383 2005-11-01 09:16:16Z pfish $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void us_cmd_sendpass(char *origin);

command_t us_sendpass = { "SENDPASS", "Email registration passwords.",
				AC_IRCOP, us_cmd_sendpass };

list_t *us_cmdtree, *us_helptree;

void _modinit(module_t *m)
{
	us_cmdtree = module_locate_symbol("userserv/main", "us_cmdtree");
	us_helptree = module_locate_symbol("userserv/main", "us_helptree");
	command_add(&us_sendpass, us_cmdtree);
	help_addentry(us_helptree, "SENDPASS", "help/userserv/sendpass", NULL);
}

void _moddeinit()
{
	command_delete(&us_sendpass, us_cmdtree);
	help_delentry(us_helptree, "SENDPASS");
}

static void us_cmd_sendpass(char *origin)
{
	user_t *u = user_find(origin);
	myuser_t *mu;
	char *name = strtok(NULL, " ");

	if (u == NULL)
		return;

	if (!name)
	{
		notice(usersvs.nick, origin, "Insufficient parameters for \2SENDPASS\2.");
		notice(usersvs.nick, origin, "Syntax: SENDPASS <account>");
		return;
	}

	if (!(mu = myuser_find(name)))
	{
		notice(usersvs.nick, origin, "\2%s\2 is not registered.", name);
		return;
	}

	if (is_sra(mu) && !is_sra(u->myuser))
	{
		notice(usersvs.nick, origin, "\2%s\2 belongs to a services root administrator; you must be a services root to send the password.", name);
		return;
	}

	if (sendemail(u, EMAIL_SENDPASS, mu, mu->pass))
		notice(usersvs.nick, origin, "The password for \2%s\2 has been sent to \2%s\2.", mu->name, mu->email);
	else
		notice(usersvs.nick, origin, "Email send failed.");

	return;
}
