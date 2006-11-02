/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService STATUS function.
 *
 * $Id: status.c 7037 2006-11-02 23:07:34Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"nickserv/status", FALSE, _modinit, _moddeinit,
	"$Id: status.c 7037 2006-11-02 23:07:34Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void ns_cmd_acc(sourceinfo_t *si, int parc, char *parv[]);
static void ns_cmd_status(sourceinfo_t *si, int parc, char *parv[]);

command_t ns_status = { "STATUS", "Displays session information.", AC_NONE, 0, ns_cmd_status };
command_t ns_acc = { "ACC", "Displays parsable session information", AC_NONE, 2, ns_cmd_acc };

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
	char *targuser = parv[0];
	char *targaccount = parv[1];
	user_t *u;
	myuser_t *mu;

	if (!targuser)
	{
		u = si->su;
		targuser = u != NULL ? u->nick : "?";
	}
	else
		u = user_find_named(targuser);

	if (!u)
	{
		command_fail(si, fault_nosuch_target, "%s%s%s ACC 0 (offline)", targuser, parc >= 2 ? " -> " : "", parc >= 2 ? targaccount : "");
		return;
	}

	if (!targaccount)
		targaccount = u->nick;
	if (!strcmp(targaccount, "*"))
		mu = u->myuser;
	else
		mu = myuser_find_ext(targaccount);

	if (!mu)
	{
		command_fail(si, fault_nosuch_target, "%s%s%s ACC 0 (not registered)", u->nick, parc >= 2 ? " -> " : "", parc >= 2 ? targaccount : "");
		return;
	}

	if (u->myuser == mu)
		command_success_nodata(si, "%s%s%s ACC 3", u->nick, parc >= 2 ? " -> " : "", parc >= 2 ? mu->name : "");
	else if (!irccasecmp(mu->name, u->nick) && myuser_access_verify(u, mu))
		command_success_nodata(si, "%s%s%s ACC 2", u->nick, parc >= 2 ? " -> " : "", parc >= 2 ? mu->name : "");
	else
		command_success_nodata(si, "%s%s%s ACC 1", u->nick, parc >= 2 ? " -> " : "", parc >= 2 ? mu->name : "");
}

static void ns_cmd_status(sourceinfo_t *si, int parc, char *parv[])
{
	logcommand(si, CMDLOG_GET, "STATUS");

	if (!si->smu)
		command_success_nodata(si, "You are not logged in.");
	else
	{
		command_success_nodata(si, "You are logged in as \2%s\2.", si->smu->name);

		if (is_soper(si->smu))
		{
			soper_t *soper = si->smu->soper;

			command_success_nodata(si, "You are a services operator of class %s.", soper->operclass ? soper->operclass->name : soper->classname);
		}
	}

	if (si->su != NULL && (si->smu == NULL || irccasecmp(si->smu->name, si->su->nick)))
	{
		myuser_t *mu;

		mu = myuser_find(si->su->nick);
		if (mu != NULL && myuser_access_verify(si->su, mu))
			command_success_nodata(si, "You are recognized as \2%s\2.", mu->name);
	}

	if (si->su != NULL && is_admin(si->su))
		command_success_nodata(si, "You are a server administrator.");

	if (si->su != NULL && is_ircop(si->su))
		command_success_nodata(si, "You are an IRC operator.");
}

