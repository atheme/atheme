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
	"groupserv/set_open", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void gs_cmd_set_open(sourceinfo_t *si, int parc, char *parv[]);

command_t gs_set_open = { "OPEN", N_("Sets the group as open for anyone to join."), AC_AUTHENTICATED, 2, gs_cmd_set_open, { .path = "groupserv/set_open" } };

static void gs_cmd_set_open(sourceinfo_t *si, int parc, char *parv[])
{
	mygroup_t *mg;

	if (!parv[0] || !parv[1])
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "OPEN");
		command_fail(si, fault_needmoreparams, _("Syntax: OPEN <!group> <ON|OFF>"));
		return;
	}

	if ((mg = mygroup_find(parv[0])) == NULL)
	{
		command_fail(si, fault_nosuch_target, _("The group \2%s\2 does not exist."), parv[0]);
		return;
	}
	
	if (!groupacs_sourceinfo_has_flag(mg, si, GA_FOUNDER))
	{
		command_fail(si, fault_noprivs, _("You are not authorized to execute this command."));
		return;
	}

	if (!strcasecmp(parv[1], "ON"))
	{
		if (!gs_config->enable_open_groups)
		{
			command_fail(si, fault_nochange, _("Setting groups as open has been administratively disabled."));
			return;
		}

		if (mg->flags & MG_OPEN)
		{
			command_fail(si, fault_nochange, _("\2%s\2 is already open to anyone joining."), entity(mg)->name);
			return;
		}

		mg->flags |= MG_OPEN;

		logcommand(si, CMDLOG_SET, "OPEN:ON: \2%s\2", entity(mg)->name);
		command_success_nodata(si, _("\2%s\2 is now open to anyone joining."), entity(mg)->name);
	}
	else if (!strcasecmp(parv[1], "OFF"))
	{
		if (!(mg->flags & MG_OPEN))
		{
			command_fail(si, fault_nochange, _("\2%s\2 is already not open to anyone joining."), entity(mg)->name);
			return;
		}

		mg->flags &= ~MG_OPEN;

		logcommand(si, CMDLOG_SET, "OPEN:OFF: \2%s\2", entity(mg)->name);
		command_success_nodata(si, _("\2%s\2 is no longer open to anyone joining."), entity(mg)->name);
	}
	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "OPEN");
		command_fail(si, fault_badparams, _("Syntax: OPEN <!group> <ON|OFF>"));
	}
}

void _modinit(module_t *m)
{
	use_groupserv_main_symbols(m);
	use_groupserv_set_symbols(m);

	command_add(&gs_set_open, gs_set_cmdtree);
}

void _moddeinit(module_unload_intent_t intent)
{
	command_delete(&gs_set_open, gs_set_cmdtree);
}

