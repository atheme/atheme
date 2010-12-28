
/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains routines to handle the GroupServ HELP command.
 *
 */

#include "atheme.h"
#include "groupserv.h"

DECLARE_MODULE_V1
(
	"groupserv/regnolimit", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void gs_cmd_regnolimit(sourceinfo_t *si, int parc, char *parv[]);

command_t gs_regnolimit = { "REGNOLIMIT", N_("Allow a group to bypass registration limits."), PRIV_GROUP_ADMIN, 2, gs_cmd_regnolimit, { .path = "groupserv/regnolimit" } };

static void gs_cmd_regnolimit(sourceinfo_t *si, int parc, char *parv[])
{
	mygroup_t *mg;

	if (!parv[0] || !parv[1])
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "REGNOLIMIT");
		command_fail(si, fault_needmoreparams, _("Syntax: REGNOLIMIT <!group> <ON|OFF>"));
		return;
	}

	if ((mg = mygroup_find(parv[0])) == NULL)
	{
		command_fail(si, fault_nosuch_target, _("The group \2%s\2 does not exist."), parv[0]);
		return;
	}
	
	if (!strcasecmp(parv[1], "ON"))
	{
		if (mg->flags & MG_REGNOLIMIT)
		{
			command_fail(si, fault_nochange, _("\2%s\2 can already bypass registration limits."), entity(mg)->name);
			return;
		}

		mg->flags |= MG_REGNOLIMIT;

		wallops("%s set the REGNOLIMIT option on the group \2%s\2.", get_oper_name(si), entity(mg)->name);
		logcommand(si, CMDLOG_ADMIN, "REGNOLIMIT:ON: \2%s\2", entity(mg)->name);
		command_success_nodata(si, _("\2%s\2 can now bypass registration limits."), entity(mg)->name);
	}
	else if (!strcasecmp(parv[1], "OFF"))
	{
		if (!(mg->flags & MG_REGNOLIMIT))
		{
			command_fail(si, fault_nochange, _("\2%s\2 cannot bypass registration limits."), entity(mg)->name);
			return;
		}

		mg->flags &= ~MG_REGNOLIMIT;

		wallops("%s removed the REGNOLIMIT option from the group \2%s\2.", get_oper_name(si), entity(mg)->name);
		logcommand(si, CMDLOG_ADMIN, "REGNOLIMIT:OFF: \2%s\2", entity(mg)->name);
		command_success_nodata(si, _("\2%s\2 cannot bypass registration limits anymore."), entity(mg)->name);
	}
	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "REGNOLIMIT");
		command_fail(si, fault_badparams, _("Syntax: REGNOLIMIT <!group> <ON|OFF>"));
	}
}

void _modinit(module_t *m)
{
	use_groupserv_main_symbols(m);

	service_named_bind_command("groupserv", &gs_regnolimit);
}

void _moddeinit(module_unload_intent_t intent)
{
	service_named_unbind_command("groupserv", &gs_regnolimit);
}

