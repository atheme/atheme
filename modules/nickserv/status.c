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
	"nickserv/status", FALSE, _modinit, _moddeinit,
	"$Id: status.c 6337 2006-09-10 15:54:41Z pippijn $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void ns_cmd_acc(sourceinfo_t *si, int parc, char *parv[]);
static void ns_cmd_status(sourceinfo_t *si, int parc, char *parv[]);

command_t ns_status = { "STATUS", "Displays session information.", AC_NONE, 0, ns_cmd_status };
command_t ns_acc = { "ACC", "Displays parsable session information", AC_NONE, 1, ns_cmd_acc };

list_t *ns_cmdtree, *ns_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(ns_cmdtree, "nickserv/main", "ns_cmdtree");
	MODULE_USE_SYMBOL(ns_helptree, "nickserv/main", "ns_helptree");

	command_add(&ns_acc, ns_cmdtree);
	help_addentry(ns_helptree, "ACC", "help/nickserv/acc", NULL);
	command_add(&ns_status, ns_cmdtree);
	help_addentry(ns_helptree, "STATUS", "help/nickserv/status", NULL);
}

void _moddeinit()
{
	command_delete(&ns_acc, ns_cmdtree);
	help_delentry(ns_helptree, "ACC");
	command_delete(&ns_status, ns_cmdtree);
	help_delentry(ns_helptree, "STATUS");
}

static void ns_cmd_acc(sourceinfo_t *si, int parc, char *parv[])
{
	char *targ = parv[0];
	user_t *u;

	if (!targ)
		u = si->su;
	else
		u = user_find_named(targ);

	if (!u)
	{
		notice(nicksvs.nick, si->su->nick, "User not online.");
		return;
	}


	logcommand(nicksvs.me, si->su, CMDLOG_GET, "ACC %s", u->nick);

	if (!u->myuser)
	{
		notice(nicksvs.nick, si->su->nick, "%s ACC 0", u->nick);
		return;
	}
	else
		notice(nicksvs.nick, si->su->nick, "%s ACC 3", u->nick);
}

static void ns_cmd_status(sourceinfo_t *si, int parc, char *parv[])
{
	logcommand(nicksvs.me, si->su, CMDLOG_GET, "STATUS");

	if (!si->su->myuser)
	{
		notice(nicksvs.nick, si->su->nick, "You are not logged in.");
		return;
	}

	notice(nicksvs.nick, si->su->nick, "You are logged in as \2%s\2.", si->su->myuser->name);

	if (is_soper(si->su->myuser))
	{
		operclass_t *operclass;

		operclass = si->su->myuser->soper->operclass;
		if (operclass == NULL)
			notice(nicksvs.nick, si->su->nick, "You are a services root administrator.");
		else
			notice(nicksvs.nick, si->su->nick, "You are a services operator of class %s.", operclass->name);
	}

	if (is_admin(si->su))
		notice(nicksvs.nick, si->su->nick, "You are a server administrator.");

	if (is_ircop(si->su))
		notice(nicksvs.nick, si->su->nick, "You are an IRC operator.");
}

