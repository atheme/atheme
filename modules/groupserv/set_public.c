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
	"groupserv/set_public", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void gs_cmd_set_public(sourceinfo_t *si, int parc, char *parv[]);

command_t gs_set_public = { "PUBLIC", N_("Sets the group as public."), AC_AUTHENTICATED, 2, gs_cmd_set_public, { .path = "groupserv/set_public" } };

static void gs_cmd_set_public(sourceinfo_t *si, int parc, char *parv[])
{
	mygroup_t *mg;

	if (!parv[0] || !parv[1])
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "PUBLIC");
		command_fail(si, fault_needmoreparams, _("Syntax: PUBLIC <!group> <ON|OFF>"));
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
		if (mg->flags & MG_PUBLIC)
		{
			command_fail(si, fault_nochange, _("\2%s\2 is already public."), entity(mg)->name);
			return;
		}

		mg->flags |= MG_PUBLIC;

		logcommand(si, CMDLOG_SET, "PUBLIC:ON: \2%s\2", entity(mg)->name);
		command_success_nodata(si, _("\2%s\2 is now public."), entity(mg)->name);
	}
	else if (!strcasecmp(parv[1], "OFF"))
	{
		if (!(mg->flags & MG_PUBLIC))
		{
			command_fail(si, fault_nochange, _("\2%s\2 is not public already."), entity(mg)->name);
			return;
		}

		mg->flags &= ~MG_PUBLIC;

		logcommand(si, CMDLOG_SET, "PUBLIC:OFF: \2%s\2", entity(mg)->name);
		command_success_nodata(si, _("\2%s\2 is no longer public."), entity(mg)->name);
	}
	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "PUBLIC");
		command_fail(si, fault_badparams, _("Syntax: PUBLIC <!group> <ON|OFF>"));
	}
}

void _modinit(module_t *m)
{
	use_groupserv_main_symbols(m);
	use_groupserv_set_symbols(m);

	command_add(&gs_set_public, gs_set_cmdtree);
}

void _moddeinit(module_unload_intent_t intent)
{
	command_delete(&gs_set_public, gs_set_cmdtree);
}

