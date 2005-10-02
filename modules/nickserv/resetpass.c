/*
 * Copyright (c) 2005 Patrick Fish, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for NickServ RESETPASS
 *
 * $Id: resetpass.c 2513 2005-10-02 23:34:28Z pfish $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"nickserv/resetpass", FALSE, _modinit, _moddeinit,
	"$Id: resetpass.c 2513 2005-10-02 23:34:28Z pfish $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void ns_cmd_resetpass(char *origin);

command_t ns_resetpass = { "RESETPASS", "Resets a nickname password.",
                        AC_IRCOP, ns_cmd_resetpass };
                                                                                   
list_t *ns_cmdtree;

void _modinit(module_t *m)
{
	ns_cmdtree = module_locate_symbol("nickserv/main", "ns_cmdtree");
        command_add(&ns_resetpass, ns_cmdtree);
}

void _moddeinit()
{
	command_delete(&ns_resetpass, ns_cmdtree);
}

static void ns_cmd_resetpass(char *origin)
{
	myuser_t *mu;
	metadata_t *md;
	char *name = strtok(NULL, " ");
	char *newpass = gen_pw(12);

	if (!name)
	{
		notice(nicksvs.nick, origin, "Invalid parameters specified for \2RESETPASS\2.");
		notice(nicksvs.nick, origin, "Syntax: RESETPASS <nickname>");
		return;
	}

	if (!(mu = myuser_find(name)))
	{
		notice(nicksvs.nick, origin, "\2%s\2 is not registered.", name);
		return;
	}

	if (md = metadata_find(mu, METADATA_USER, "private:mark:setter"))
	{
		notice(nicksvs.nick, origin, "This operation cannot be performed on %s, because the nickname has been marked by %s.", name, md->value);
		return;
	}

	notice(nicksvs.nick, origin, "The password for the nickname %s has been changed to %s.", name, newpass);
	strlcpy(mu->pass, newpass, NICKLEN);

	wallops("%s used the RESETPASS cmd on the nickname %s", origin, name);
	snoop("RESETPASS: \2%s\2 reset the password for \2%s\2", origin, name);

	free(newpass);
}
