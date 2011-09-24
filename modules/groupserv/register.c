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
	"groupserv/register", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void gs_cmd_register(sourceinfo_t *si, int parc, char *parv[]);

command_t gs_register = { "REGISTER", N_("Registers a group."), AC_AUTHENTICATED, 2, gs_cmd_register, { .path = "groupserv/register" } };

static void gs_cmd_register(sourceinfo_t *si, int parc, char *parv[])
{
	mygroup_t *mg;

	if (!parv[0])
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "REGISTER");
		command_fail(si, fault_needmoreparams, _("To register a group: REGISTER <!groupname>"));
		return;
	}

	if (*parv[0] != '!')
	{
		command_fail(si, fault_needmoreparams, STR_INVALID_PARAMS, "REGISTER");
		command_fail(si, fault_needmoreparams, _("To register a group: REGISTER <!groupname>"));
		return;
	}

	if (mygroup_find(parv[0]))
	{
		command_fail(si, fault_alreadyexists, _("The group \2%s\2 already exists."), parv[0]);
		return;
	}

	if (strlen(parv[0]) >= NICKLEN)
	{
		command_fail(si, fault_badparams, _("The group name \2%s\2 is invalid."), parv[0]);
		return;
	}

	if (myuser_count_group_flag(si->smu, GA_FOUNDER) > gs_config->maxgroups &&
	    !has_priv(si, PRIV_REG_NOLIMIT))
	{
		command_fail(si, fault_toomany, _("You have too many groups registered."));
		return;
	}

        if (metadata_find(si->smu, "private:restrict:setter"))
        {
                command_fail(si, fault_noprivs, _("You have been restricted from registering groups by network staff."));
                return;
        }

	mg = mygroup_add(parv[0]);
	groupacs_add(mg, si->smu, GA_ALL | GA_FOUNDER);

	logcommand(si, CMDLOG_REGISTER, "REGISTER: \2%s\2", entity(mg)->name); 
	command_success_nodata(si, _("The group \2%s\2 has been registered to \2%s\2."), entity(mg)->name, entity(si->smu)->name);
}


void _modinit(module_t *m)
{
	use_groupserv_main_symbols(m);

	service_named_bind_command("groupserv", &gs_register);
}

void _moddeinit(module_unload_intent_t intent)
{
	service_named_unbind_command("groupserv", &gs_register);
}

