/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains routines to handle the GroupServ HELP command.
 */

#include "atheme.h"
#include "groupserv.h"

static void
cmd_gs_drop_func(sourceinfo_t *const restrict si, const int __attribute__((unused)) parc, char *parv[])
{
	const char *const name = parv[0];
	const char *const key = parv[1];

	if (! name)
	{
		(void) command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "DROP");
		(void) command_fail(si, fault_needmoreparams, _("Syntax: DROP <!group>"));
		return;
	}

	if (*name != '!')
	{
		(void) command_fail(si, fault_badparams, STR_INVALID_PARAMS, "DROP");
		(void) command_fail(si, fault_badparams, _("Syntax: DROP <!group>"));
		return;
	}

	mygroup_t *mg;

	if (! (mg = mygroup_find(name)))
	{
		(void) command_fail(si, fault_nosuch_target, _("Group \2%s\2 does not exist."), name);
		return;
	}

	if (! groupacs_sourceinfo_has_flag(mg, si, GA_FOUNDER))
	{
		(void) command_fail(si, fault_noprivs, _("You are not authorized to execute this command."));
		return;
	}

	if (si->su)
	{
		const char *const challenge = create_weak_challenge(si, entity(mg)->name);

		if (! challenge)
		{
			(void) command_fail(si, fault_internalerror, _("Failed to create challenge"));
			return;
		}

		if (! key)
		{
			char fullcmd[BUFSIZE];

			(void) snprintf(fullcmd, sizeof fullcmd, "/%s%s DROP %s %s", (ircd->uses_rcommand == false) ?
			                "msg " : "", si->service->disp, entity(mg)->name, challenge);

			(void) command_success_nodata(si, _("To avoid accidental use of this command, this operation "
			                                    "has to be confirmed. Please confirm by replying with "
			                                    "\2%s\2"), fullcmd);
			return;
		}

		if (strcmp(challenge, key) != 0)
		{
			(void) command_fail(si, fault_badparams, _("Invalid key for \2%s\2."), "DROP");
			return;
		}
	}

	(void) logcommand(si, CMDLOG_REGISTER, "DROP: \2%s\2", entity(mg)->name);

	(void) remove_group_chanacs(mg);
	(void) hook_call_group_drop(mg);

	(void) command_success_nodata(si, _("The group \2%s\2 has been dropped."), entity(mg)->name);
	(void) object_unref(mg);
}

static command_t cmd_gs_drop = {

	.name           = "DROP",
	.desc           = N_("Drops a group registration."),
	.access         = AC_AUTHENTICATED,
	.maxparc        = 2,
	.cmd            = &cmd_gs_drop_func,

	.help           = {

		.path   = "groupserv/drop",
		.func   = NULL,
	},
};

static void
mod_init(module_t *const restrict m)
{
	(void) use_groupserv_main_symbols(m);

	(void) service_named_bind_command("groupserv", &cmd_gs_drop);
}

static void
mod_deinit(const module_unload_intent_t __attribute__((unused)) intent)
{
	(void) service_named_unbind_command("groupserv", &cmd_gs_drop);
}

SIMPLE_DECLARE_MODULE_V1("groupserv/drop", MODULE_UNLOAD_CAPABILITY_OK)
