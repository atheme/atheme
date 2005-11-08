/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService STATUS function.
 *
 * $Id: status.c 3653 2005-11-08 00:49:36Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"userserv/status", FALSE, _modinit, _moddeinit,
	"$Id: status.c 3653 2005-11-08 00:49:36Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void us_cmd_acc(char *origin);
static void us_cmd_status(char *origin);

command_t us_status = {
	"STATUS",
	"Displays session information.",
	AC_NONE,
	us_cmd_status
};

command_t us_acc = {
	"ACC",
	"Displays parsable session information",
	AC_NONE,
	us_cmd_acc
};

list_t *us_cmdtree, *us_helptree;

void _modinit(module_t *m)
{
	us_cmdtree = module_locate_symbol("userserv/main", "us_cmdtree");
	us_helptree = module_locate_symbol("userserv/main", "us_helptree");
	command_add(&us_acc, us_cmdtree);
	help_addentry(us_helptree, "ACC", "help/userserv/acc", NULL);
	command_add(&us_status, us_cmdtree);
	help_addentry(us_helptree, "STATUS", "help/userserv/status", NULL);
}

void _moddeinit()
{
	command_delete(&us_acc, us_cmdtree);
	help_delentry(us_helptree, "ACC");
	command_delete(&us_status, us_cmdtree);
	help_delentry(us_helptree, "STATUS");
}

static void us_cmd_acc(char *origin)
{
	char *targ = strtok(NULL, " ");
	user_t *u;

	if (!targ)
		u = user_find(origin);
	else
		u = user_find_named(targ);

	logcommand(usersvs.me, user_find(origin), CMDLOG_GET, "ACC %s", u->nick);

	if (!u->myuser)
	{
		notice(usersvs.nick, origin, "%s ACC 0", u->nick);
		return;
	}
	else
		notice(usersvs.nick, origin, "%s ACC 3", u->nick);
}

static void us_cmd_status(char *origin)
{
	user_t *u = user_find(origin);

	logcommand(usersvs.me, u, CMDLOG_GET, "STATUS");

	if (!u->myuser)
	{
		notice(usersvs.nick, origin, "You are not logged in.");
		return;
	}

	notice(usersvs.nick, origin, "You are logged in as \2%s\2.", u->myuser->name);

	if (is_sra(u->myuser))
		notice(usersvs.nick, origin, "You are a services root administrator.");

	if (is_admin(u))
		notice(usersvs.nick, origin, "You are a server administrator.");

	if (is_ircop(u))
		notice(usersvs.nick, origin, "You are an IRC operator.");
}

