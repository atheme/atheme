/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the UserServ DROP function.
 *
 * $Id: drop.c 6337 2006-09-10 15:54:41Z pippijn $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"userserv/drop", FALSE, _modinit, _moddeinit,
	"$Id: drop.c 6337 2006-09-10 15:54:41Z pippijn $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void us_cmd_drop(sourceinfo_t *si, int parc, char *parv[]);

command_t us_drop = { "DROP", "Drops a account registration.", AC_NONE, 2, us_cmd_drop };

list_t *us_cmdtree, *us_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(us_cmdtree, "userserv/main", "us_cmdtree");
	MODULE_USE_SYMBOL(us_helptree, "userserv/main", "us_helptree");

	command_add(&us_drop, us_cmdtree);
	help_addentry(us_helptree, "DROP", "help/userserv/drop", NULL);
}

void _moddeinit()
{
	command_delete(&us_drop, us_cmdtree);
	help_delentry(us_helptree, "DROP");
}

static void us_cmd_drop(sourceinfo_t *si, int parc, char *parv[])
{
	myuser_t *mu;
	char *acc = parv[0];
	char *pass = parv[1];

	if (!acc)
	{
		notice(usersvs.nick, si->su->nick, STR_INSUFFICIENT_PARAMS, "DROP");
		notice(usersvs.nick, si->su->nick, "Syntax: DROP <account> <password>");
		return;
	}

	if (!(mu = myuser_find(acc)))
	{
		notice(usersvs.nick, si->su->nick, "\2%s\2 is not registered.", acc);
		return;
	}

	if ((pass || !has_priv(si->su, PRIV_USER_ADMIN)) && !verify_password(mu, pass))
	{
		notice(usersvs.nick, si->su->nick, "Authentication failed. Invalid password for \2%s\2.", mu->name);
		return;
	}

	if (is_soper(mu))
	{
		notice(usersvs.nick, si->su->nick, "The account \2%s\2 belongs to a services operator; it cannot be dropped.", acc);
		return;
	}

	if (!pass)
		wallops("%s dropped the account \2%s\2", si->su->nick, mu->name);

	snoop("DROP: \2%s\2 by \2%s\2", mu->name, si->su->nick);
	logcommand(usersvs.me, si->su, pass ? CMDLOG_REGISTER : CMDLOG_ADMIN, "DROP %s%s", mu->name, pass ? "" : " (admin)");
	hook_call_event("user_drop", mu);
	notice(usersvs.nick, si->su->nick, "The account \2%s\2 has been dropped.", mu->name);
	myuser_delete(mu);
}
