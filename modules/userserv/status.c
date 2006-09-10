/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService STATUS function.
 *
 * $Id: status.c 6337 2006-09-10 15:54:41Z pippijn $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"userserv/status", FALSE, _modinit, _moddeinit,
	"$Id: status.c 6337 2006-09-10 15:54:41Z pippijn $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void us_cmd_acc(sourceinfo_t *si, int parc, char *parv[]);
static void us_cmd_status(sourceinfo_t *si, int parc, char *parv[]);

command_t us_status = { "STATUS", "Displays session information.", AC_NONE, 0, us_cmd_status };
command_t us_acc = { "ACC", "Displays parsable session information", AC_NONE, 1, us_cmd_acc };

list_t *us_cmdtree, *us_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(us_cmdtree, "userserv/main", "us_cmdtree");
	MODULE_USE_SYMBOL(us_helptree, "userserv/main", "us_helptree");

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

static void us_cmd_acc(sourceinfo_t *si, int parc, char *parv[])
{
	char *targ = parv[0];
	user_t *u;

	if (!targ)
		u = si->su;
	else
		u = user_find_named(targ);

	if (!u)
	{
		notice(usersvs.nick, si->su->nick, "User not online.");
		return;
	}


	logcommand(usersvs.me, si->su, CMDLOG_GET, "ACC %s", u->nick);

	if (!u->myuser)
	{
		notice(usersvs.nick, si->su->nick, "%s ACC 0", u->nick);
		return;
	}
	else
		notice(usersvs.nick, si->su->nick, "%s ACC 3", u->nick);
}

static void us_cmd_status(sourceinfo_t *si, int parc, char *parv[])
{
	logcommand(usersvs.me, si->su, CMDLOG_GET, "STATUS");

	if (!si->su->myuser)
	{
		notice(usersvs.nick, si->su->nick, "You are not logged in.");
		return;
	}

	notice(usersvs.nick, si->su->nick, "You are logged in as \2%s\2.", si->su->myuser->name);

	if (is_soper(si->su->myuser))
	{
		operclass_t *operclass;

		operclass = si->su->myuser->soper->operclass;
		if (operclass == NULL)
			notice(usersvs.nick, si->su->nick, "You are a services root administrator.");
		else
			notice(usersvs.nick, si->su->nick, "You are a services operator of class %s.", operclass->name);
	}

	if (is_admin(si->su))
		notice(usersvs.nick, si->su->nick, "You are a server administrator.");

	if (is_ircop(si->su))
		notice(usersvs.nick, si->su->nick, "You are an IRC operator.");
}

