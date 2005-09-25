/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService SENDPASS function.
 *
 * $Id: sendpass.c 2359 2005-09-25 02:49:10Z nenolod $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"userserv/sendpass", FALSE, _modinit, _moddeinit,
	"$Id: sendpass.c 2359 2005-09-25 02:49:10Z nenolod $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void us_cmd_sendpass(char *origin);

command_t us_sendpass = { "SENDPASS", "Email registration passwords.",
				AC_IRCOP, us_cmd_sendpass };

list_t *us_cmdtree;

void _modinit(module_t *m)
{
	us_cmdtree = module_locate_symbol("userserv/main", "us_cmdtree");
	command_add(&us_sendpass, us_cmdtree);
}

void _moddeinit()
{
	command_delete(&us_sendpass, us_cmdtree);
}

static void us_cmd_sendpass(char *origin)
{
	myuser_t *mu;
	char *name = strtok(NULL, " ");

	if (!name)
	{
		notice(usersvs.nick, origin, "Insufficient parameters for \2SENDPASS\2.");
		notice(usersvs.nick, origin, "Syntax: SENDPASS <nickname>");
		return;
	}

	if (!(mu = myuser_find(name)))
	{
		notice(usersvs.nick, origin, "\2%s\2 is not registered.", name);
		return;
	}

	notice(usersvs.nick, origin, "The password for \2%s\2 has been sent to \2%s\2.", mu->name, mu->email);

	sendemail(mu->name, mu->pass, 2);

	return;
}
