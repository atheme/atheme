/*
 * Copyright (c) 2005 Patrick Fish, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for NickServ RESETPASS
 *
 * $Id: resetpass.c 5686 2006-07-03 16:25:03Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"nickserv/resetpass", FALSE, _modinit, _moddeinit,
	"$Id: resetpass.c 5686 2006-07-03 16:25:03Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void ns_cmd_resetpass(char *origin);

command_t ns_resetpass = { "RESETPASS", "Resets a nickname password.",
                        PRIV_USER_ADMIN, ns_cmd_resetpass };
                                                                                   
list_t *ns_cmdtree, *ns_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(ns_cmdtree, "nickserv/main", "ns_cmdtree");
	MODULE_USE_SYMBOL(ns_helptree, "nickserv/main", "ns_helptree");

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
	user_t *u = user_find_named(origin);
	metadata_t *md;
	char *name = strtok(NULL, " ");
	char *newpass;

	if (!name)
	{
		notice(nicksvs.nick, origin, STR_INSUFFICIENT_PARAMS, "RESETPASS");
		notice(nicksvs.nick, origin, "Syntax: RESETPASS <nickname>");
		return;
	}

	if (!(mu = myuser_find(name)))
	{
		notice(nicksvs.nick, origin, "\2%s\2 is not registered.", name);
		return;
	}

	if (is_soper(mu) && !has_priv(u, PRIV_ADMIN))
	{
		logcommand(nicksvs.me, u, CMDLOG_ADMIN, "failed RESETPASS %s (is SOPER)", name);
		notice(nicksvs.nick, origin, "\2%s\2 belongs to a services operator; you need %s privilege to reset the password.", name, PRIV_ADMIN);
		return;
	}

	if ((md = metadata_find(mu, METADATA_USER, "private:mark:setter")) && has_priv(u, PRIV_MARK))
	{
		logcommand(nicksvs.me, u, CMDLOG_ADMIN, "RESETPASS %s (overriding mark by %s)", name, md->value);
		notice(nicksvs.nick, origin, "Overriding MARK placed by %s on the nickname %s.", md->value, name);
		newpass = gen_pw(12);
		notice(nicksvs.nick, origin, "The password for the nickname %s has been changed to %s.", name, newpass);
		set_password(mu, newpass);
		free(newpass);
		wallops("%s reset the password for the \2MARKED\2 nickname %s.", origin, name);
		return;
	}

	if ((md = metadata_find(mu, METADATA_USER, "private:mark:setter")))
	{
		logcommand(nicksvs.me, u, CMDLOG_ADMIN, "failed RESETPASS %s (marked by %s)", name, md->value);
		notice(nicksvs.nick, origin, "This operation cannot be performed on %s, because the nickname has been marked by %s.", name, md->value);
		return;
	}

	newpass = gen_pw(12);	
	notice(nicksvs.nick, origin, "The password for the nickname %s has been changed to %s.", name, newpass);
	set_password(mu, newpass);
	free(newpass);

	wallops("%s reset the password for the nickname %s", origin, name);
	snoop("RESETPASS: \2%s\2 reset the password for \2%s\2", origin, name);
	logcommand(nicksvs.me, u, CMDLOG_ADMIN, "RESETPASS %s", name);

}
