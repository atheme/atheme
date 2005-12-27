/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the UserServ DROP function.
 *
 * $Id: drop.c 4219 2005-12-27 17:41:18Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"userserv/drop", FALSE, _modinit, _moddeinit,
	"$Id: drop.c 4219 2005-12-27 17:41:18Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void us_cmd_drop(char *origin);

command_t us_drop = { "DROP", "Drops a account registration.", AC_NONE, us_cmd_drop };

list_t *us_cmdtree, *us_helptree;

void _modinit(module_t *m)
{
	us_cmdtree = module_locate_symbol("userserv/main", "us_cmdtree");
	us_helptree = module_locate_symbol("userserv/main", "us_helptree");
	command_add(&us_drop, us_cmdtree);
	help_addentry(us_helptree, "DROP", "help/userserv/drop", NULL);
}

void _moddeinit()
{
	command_delete(&us_drop, us_cmdtree);
	help_delentry(us_helptree, "DROP");
}

static void us_cmd_drop(char *origin)
{
	user_t *u = user_find(origin);
	myuser_t *mu;
	char *acc = strtok(NULL, " ");
	char *pass = strtok(NULL, " ");

	if (!acc && !pass && !is_sra(u->myuser))
	{
		notice(usersvs.nick, origin, "Insufficient parameters specified for \2DROP\2.");
		notice(usersvs.nick, origin, "Syntax: DROP <account> <password>");
		return;
	}

	if (!(mu = myuser_find(acc)))
	{
		notice(usersvs.nick, origin, "\2%s\2 is not registered.", acc);
		return;
	}

	if ((pass || !has_priv(u, PRIV_USER_ADMIN)) && !verify_password(mu, pass))
	{
		notice(usersvs.nick, origin, "Authentication failed. Invalid password for \2%s\2.", mu->name);
		return;
	}

	if (is_sra(mu))
	{
		notice(usersvs.nick, origin, "The account \2%s\2 belongs to a services root administrator; it cannot be dropped.", acc);
		return;
	}

	if (!pass)
		wallops("%s dropped the account \2%s\2", origin, mu->name);

	snoop("DROP: \2%s\2 by \2%s\2", mu->name, u->nick);
	logcommand(usersvs.me, u, pass ? CMDLOG_REGISTER : CMDLOG_ADMIN, "DROP %s%s", mu->name, pass ? "" : " (admin)");
	hook_call_event("user_drop", mu);
	notice(usersvs.nick, origin, "The account \2%s\2 has been dropped.", mu->name);
	myuser_delete(mu->name);
}
