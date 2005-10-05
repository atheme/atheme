/*
 * Copyright (c) 2005 Patrick Fish, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for UserServ RESETPASS
 *
 * $Id: resetpass.c 2575 2005-10-05 02:46:11Z alambert $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"userserv/resetpass", FALSE, _modinit, _moddeinit,
	"$Id: resetpass.c 2575 2005-10-05 02:46:11Z alambert $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void us_cmd_resetpass(char *origin);

command_t us_resetpass = { "RESETPASS", "Resets a nickname password.",
                        AC_IRCOP, us_cmd_resetpass };
                                                                                   
list_t *us_cmdtree, *us_helptree;

void _modinit(module_t *m)
{
	us_cmdtree = module_locate_symbol("userserv/main", "us_cmdtree");
	us_helptree = module_locate_symbol("userserv/main", "us_helptree");
        command_add(&us_resetpass, us_cmdtree);
	help_addentry(us_helptree, "RESETPASS", "help/userserv/resetpass", NULL);
}

void _moddeinit()
{
	command_delete(&us_resetpass, us_cmdtree);
	help_delentry(us_helptree, "RESETPASS");
}

static void us_cmd_resetpass(char *origin)
{
	myuser_t *mu;
	user_t *u = user_find(origin);
	metadata_t *md;
	char *name = strtok(NULL, " ");
	char *newpass = gen_pw(12);

	if (!name)
	{
		notice(usersvs.nick, origin, "Invalid parameters specified for \2RESETPASS\2.");
		notice(usersvs.nick, origin, "Syntax: RESETPASS <nickname>");
		return;
	}

	if (!(mu = myuser_find(name)))
	{
		notice(usersvs.nick, origin, "\2%s\2 is not registered.", name);
		return;
	}

	if ((md = metadata_find(mu, METADATA_USER, "private:mark:setter")) && is_sra(u->myuser))
	{
		notice(usersvs.nick, origin, "Overriding MARK placed by %s on the nickname %s.", md->value, name);
		notice(usersvs.nick, origin, "The password for the nickname %s has been changed to %s.", name, newpass);
		strlcpy(mu->pass, newpass, NICKLEN);
		wallops("%s used the RESETPASS cmd on the \2MARKED\2 nickname %s.", origin, name);
		return;
	}

	if ((md = metadata_find(mu, METADATA_USER, "private:mark:setter")) && !is_sra(u->myuser))
	{
		notice(usersvs.nick, origin, "This operation cannot be performed on %s, because the nickname has been marked by %s.", name, md->value);
		return;
	}
	


	notice(usersvs.nick, origin, "The password for the nickname %s has been changed to %s.", name, newpass);
	strlcpy(mu->pass, newpass, NICKLEN);

	wallops("%s used the RESETPASS cmd on the nickname %s", origin, name);
	snoop("RESETPASS: \2%s\2 reset the password for \2%s\2", origin, name);

	free(newpass);
}
