/*
 * Copyright (c) 2005 Patrick Fish, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for NickServ RESETPASS
 *
 * $Id: resetpass.c 3677 2005-11-08 23:32:49Z pfish $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"nickserv/resetpass", FALSE, _modinit, _moddeinit,
	"$Id: resetpass.c 3677 2005-11-08 23:32:49Z pfish $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void ns_cmd_resetpass(char *origin);

command_t ns_resetpass = { "RESETPASS", "Resets a nickname password.",
                        AC_IRCOP, ns_cmd_resetpass };
                                                                                   
list_t *ns_cmdtree, *ns_helptree;

void _modinit(module_t *m)
{
	ns_cmdtree = module_locate_symbol("nickserv/main", "ns_cmdtree");
	ns_helptree = module_locate_symbol("nickserv/main", "ns_helptree");
        command_add(&ns_resetpass, ns_cmdtree);
	help_addentry(ns_helptree, "RESETPASS", "help/nickserv/resetpass", NULL);
}

void _moddeinit()
{
	command_delete(&ns_resetpass, ns_cmdtree);
	help_delentry(ns_helptree, "RESETPASS");
}

static void ns_cmd_resetpass(char *origin)
{
	myuser_t *mu;
	user_t *u = user_find(origin);
	metadata_t *md;
	char *name = strtok(NULL, " ");

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

	if (is_sra(mu) && !is_sra(u->myuser))
	{
		logcommand(nicksvs.me, u, CMDLOG_ADMIN, "failed RESETPASS %s (is SRA)", name);
		notice(nicksvs.nick, origin, "\2%s\2 belongs to a services root administrator; you must be a services root administrator to reset the password.", name);
		return;
	}

	if ((md = metadata_find(mu, METADATA_USER, "private:mark:setter")) && is_sra(u->myuser))
	{
		char *newpass = gen_pw(12);
		logcommand(nicksvs.me, u, CMDLOG_ADMIN, "RESETPASS %s (overriding mark by %s)", name, md->value);
		notice(nicksvs.nick, origin, "Overriding MARK placed by %s on the nickname %s.", md->value, name);
		notice(nicksvs.nick, origin, "The password for the nickname %s has been changed to %s.", name, newpass);
		strlcpy(mu->pass, newpass, NICKLEN);
		wallops("%s reset the password for the \2MARKED\2 nickname %s.", origin, name);
		free(newpass);
		return;
	}

	if ((md = metadata_find(mu, METADATA_USER, "private:mark:setter")) && !is_sra(u->myuser))
	{
		logcommand(nicksvs.me, u, CMDLOG_ADMIN, "failed RESETPASS %s (marked by %s)", name, md->value);
		notice(nicksvs.nick, origin, "This operation cannot be performed on %s, because the nickname has been marked by %s.", name, md->value);
		return;
	}

	char *newpass = gen_pw(12);	
	notice(nicksvs.nick, origin, "The password for the nickname %s has been changed to %s.", name, newpass);
	strlcpy(mu->pass, newpass, NICKLEN);

	wallops("%s reset the password for the nickname %s", origin, name);
	snoop("RESETPASS: \2%s\2 reset the password for \2%s\2", origin, name);
	logcommand(nicksvs.me, u, CMDLOG_ADMIN, "RESETPASS %s", name);

	free(newpass);
}
