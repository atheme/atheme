/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService STATUS function.
 *
 * $Id: status.c 5686 2006-07-03 16:25:03Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"nickserv/status", FALSE, _modinit, _moddeinit,
	"$Id: status.c 5686 2006-07-03 16:25:03Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void ns_cmd_acc(char *origin);
static void ns_cmd_status(char *origin);

command_t ns_status = {
	"STATUS",
	"Displays session information.",
	AC_NONE,
	ns_cmd_status
};

command_t ns_acc = {
	"ACC",
	"Displays parsable session information",
	AC_NONE,
	ns_cmd_acc
};

list_t *ns_cmdtree, *ns_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(ns_cmdtree, "nickserv/main", "ns_cmdtree");
	MODULE_USE_SYMBOL(ns_helptree, "nickserv/main", "ns_helptree");

	command_add(&ns_acc, ns_cmdtree);
	command_add(&ns_status, ns_cmdtree);
	help_addentry(ns_helptree, "STATUS", "help/nickserv/status", NULL);
}

void _moddeinit()
{
	command_delete(&ns_acc, ns_cmdtree);
	command_delete(&ns_status, ns_cmdtree);
	help_delentry(ns_helptree, "STATUS");
}

static void ns_cmd_acc(char *origin)
{
	char *targ = strtok(NULL, " ");
	user_t *u;

	if (!targ)
		u = user_find_named(origin);
	else
		u = user_find_named(targ);

	if (!u)
	{
		notice(nicksvs.nick, origin, "User not online.");
		return;
	}

	logcommand(nicksvs.me, user_find_named(origin), CMDLOG_GET, "ACC %s", u->nick);

	if (!u->myuser)
	{
		notice(nicksvs.nick, origin, "%s ACC 0", u->nick);
		return;
	}
	else
		notice(nicksvs.nick, origin, "%s ACC 3", u->nick);
}

static void ns_cmd_status(char *origin)
{
	user_t *u = user_find_named(origin);

	logcommand(nicksvs.me, u, CMDLOG_GET, "STATUS");

	if (!u->myuser)
	{
		notice(nicksvs.nick, origin, "You are not logged in.");
		return;
	}

	notice(nicksvs.nick, origin, "You are logged in as \2%s\2.", u->myuser->name);

	if (is_soper(u->myuser))
	{
		operclass_t *operclass;

		operclass = u->myuser->soper->operclass;
		if (operclass == NULL)
			notice(nicksvs.nick, origin, "You are a services root administrator.");
		else
			notice(nicksvs.nick, origin, "You are a services operator of class %s.", operclass->name);
	}

	if (is_admin(u))
		notice(nicksvs.nick, origin, "You are a server administrator.");

	if (is_ircop(u))
		notice(nicksvs.nick, origin, "You are an IRC operator.");
}

