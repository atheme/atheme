/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService SENDPASS function.
 *
 * $Id: sendpass.c 2557 2005-10-04 06:44:30Z pfish $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"nickserv/sendpass", FALSE, _modinit, _moddeinit,
	"$Id: sendpass.c 2557 2005-10-04 06:44:30Z pfish $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void ns_cmd_sendpass(char *origin);

command_t ns_sendpass = { "SENDPASS", "Email registration passwords.",
				AC_IRCOP, ns_cmd_sendpass };

list_t *ns_cmdtree, *ns_helptree;

void _modinit(module_t *m)
{
	ns_cmdtree = module_locate_symbol("nickserv/main", "ns_cmdtree");
	command_add(&ns_sendpass, ns_cmdtree);
}

void _moddeinit()
{
	command_delete(&ns_sendpass, ns_cmdtree);
}

static void ns_cmd_sendpass(char *origin)
{
	myuser_t *mu;
	char *name = strtok(NULL, " ");

	if (!name)
	{
		notice(nicksvs.nick, origin, "Insufficient parameters for \2SENDPASS\2.");
		notice(nicksvs.nick, origin, "Syntax: SENDPASS <nickname>");
		return;
	}

	if (!(mu = myuser_find(name)))
	{
		notice(nicksvs.nick, origin, "\2%s\2 is not registered.", name);
		return;
	}

	notice(nicksvs.nick, origin, "The password for \2%s\2 has been sent to \2%s\2.", mu->name, mu->email);

	sendemail(mu->name, mu->pass, 2);

	return;
}
