/*
 * Copyright (c) 2005 William Pitcock <nenolod -at- nenolod.net>
 * Copyright (c) 2007 Jilles Tjoelker
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Changes the account name to another registered nick
 *
 */

#include "atheme.h"
#include "uplink.h"

DECLARE_MODULE_V1
(
	"nickserv/set_accountname", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

mowgli_patricia_t **ns_set_cmdtree;

static void ns_cmd_set_accountname(sourceinfo_t *si, int parc, char *parv[]);

command_t ns_set_accountname = { "ACCOUNTNAME", N_("Changes your account name."), AC_NONE, 1, ns_cmd_set_accountname, { .path = "nickserv/set_accountname" } };

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(ns_set_cmdtree, "nickserv/set_core", "ns_set_cmdtree");

	command_add(&ns_set_accountname, *ns_set_cmdtree);
}

void _moddeinit(void)
{
	command_delete(&ns_set_accountname, *ns_set_cmdtree);
}

/* SET ACCOUNTNAME <nick> */
static void ns_cmd_set_accountname(sourceinfo_t *si, int parc, char *parv[])
{
	char *newname = parv[0];
	mynick_t *mn;

	if (!newname)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "ACCOUNTNAME");
		command_fail(si, fault_needmoreparams, _("Syntax: SET ACCOUNTNAME <nick>"));
		return;
	}

	if (!si->smu)
	{
		command_fail(si, fault_noprivs, _("You are not logged in."));
		return;
	}

	if (is_conf_soper(si->smu))
	{
		command_fail(si, fault_noprivs, _("You may not modify your account name because your operclass is defined in the configuration file."));
		return;
	}

	mn = mynick_find(newname);
	if (mn == NULL)
	{
		command_fail(si, fault_nosuch_target, _("Nick \2%s\2 is not registered."), newname);
		return;
	}
	if (mn->owner != si->smu)
	{
		command_fail(si, fault_noprivs, _("Nick \2%s\2 is not registered to your account."), newname);
		return;
	}

	if (!strcmp(entity(si->smu)->name, newname))
	{
		command_fail(si, fault_nochange, _("Your account name is already set to \2%s\2."), newname);
		return;
	}

	logcommand(si, CMDLOG_REGISTER, "SET:ACCOUNTNAME: \2%s\2", newname);
	command_success_nodata(si, _("Your account name is now set to \2%s\2."), newname);
	myuser_rename(si->smu, newname);
	return;
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
