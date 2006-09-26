/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the nickserv DROP function.
 *
 * $Id: drop.c 6483 2006-09-26 14:54:03Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"nickserv/drop", FALSE, _modinit, _moddeinit,
	"$Id: drop.c 6483 2006-09-26 14:54:03Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void ns_cmd_drop(sourceinfo_t *si, int parc, char *parv[]);

command_t ns_drop = { "DROP", "Drops a nickname registration.", AC_NONE, 2, ns_cmd_drop };

list_t *ns_cmdtree, *ns_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(ns_cmdtree, "nickserv/main", "ns_cmdtree");
	MODULE_USE_SYMBOL(ns_helptree, "nickserv/main", "ns_helptree");

	command_add(&ns_drop, ns_cmdtree);
	help_addentry(ns_helptree, "DROP", "help/nickserv/drop", NULL);
}

void _moddeinit()
{
	command_delete(&ns_drop, ns_cmdtree);
	help_delentry(ns_helptree, "DROP");
}

static void ns_cmd_drop(sourceinfo_t *si, int parc, char *parv[])
{
	myuser_t *mu;
	char *acc = parv[0];
	char *pass = parv[1];

	if (!acc)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "DROP");
		command_fail(si, fault_needmoreparams, "Syntax: DROP <nickname> <password>");
		return;
	}

	if (!(mu = myuser_find(acc)))
	{
		command_fail(si, fault_nosuch_target, "\2%s\2 is not registered.", acc);
		return;
	}

	if ((pass || !has_priv(si->su, PRIV_USER_ADMIN)) && !verify_password(mu, pass))
	{
		command_fail(si, fault_authfail, "Authentication failed. Invalid password for \2%s\2.", mu->name);
		return;
	}

	if (is_soper(mu))
	{
		command_fail(si, fault_badparams, "The nickname \2%s\2 belongs to a services operator; it cannot be dropped.", acc);
		return;
	}

	if (!pass)
		wallops("%s dropped the nickname \2%s\2", si->su->nick, mu->name);

	snoop("DROP: \2%s\2 by \2%s\2", mu->name, si->su->nick);
	logcommand(nicksvs.me, si->su, pass ? CMDLOG_REGISTER : CMDLOG_ADMIN, "DROP %s%s", mu->name, pass ? "" : " (admin)");
	hook_call_event("user_drop", mu);
	command_success_nodata(si, "The nickname \2%s\2 has been dropped.", mu->name);
	myuser_delete(mu);
}
