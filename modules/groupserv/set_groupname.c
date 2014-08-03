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
#include "groupserv.h"

DECLARE_MODULE_V1
(
	"groupserv/set_groupname", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void gs_cmd_set_groupname(sourceinfo_t *si, int parc, char *parv[]);

command_t gs_set_groupname = { "GROUPNAME", N_("Changes the group's name."), AC_NONE, 1, gs_cmd_set_groupname, { .path = "groupserv/set_groupname" } };

void _modinit(module_t *m)
{
        use_groupserv_main_symbols(m);
        use_groupserv_set_symbols(m);

	command_add(&gs_set_groupname, gs_set_cmdtree);
}

void _moddeinit(module_unload_intent_t intent)
{
	command_delete(&gs_set_groupname, gs_set_cmdtree);
}

/* SET GROUPNAME <name> */
static void gs_cmd_set_groupname(sourceinfo_t *si, int parc, char *parv[])
{
	char *newname = parv[0];
	mygroup_t *mg;

	if (!newname)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "GROUPNAME");
		command_fail(si, fault_needmoreparams, _("Syntax: SET GROUPNAME <name>"));
		return;
	}

	if (*newname != '!')
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "GROUPNAME");
		command_fail(si, fault_badparams, _("Syntax: SET GROUPNAME <name>"));
		return;
	}

	mg = mygroup_find(newname);
	if (mg != NULL)
	{
		command_fail(si, fault_nosuch_target, _("The group \2%s\2 already exists."), newname);
		return;
	}

	if (!strcmp(entity(mg)->name, newname))
	{
		command_fail(si, fault_nochange, _("Your group name is already set to \2%s\2."), newname);
		return;
	}

	logcommand(si, CMDLOG_REGISTER, "SET:GROUPNAME: \2%s\2", newname);
	command_success_nodata(si, _("Your group name is now set to \2%s\2."), newname);
	mygroup_rename(mg, newname);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
