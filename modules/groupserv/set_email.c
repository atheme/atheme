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
	"groupserv/set_email", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void gs_cmd_set_email(sourceinfo_t *si, int parc, char *parv[]);

command_t gs_set_email = { "EMAIL", N_("Sets the group e-mail address."), AC_AUTHENTICATED, 2, gs_cmd_set_email, { .path = "groupserv/set_email" } };

static void gs_cmd_set_email(sourceinfo_t *si, int parc, char *parv[])
{
	mygroup_t *mg;
	char *mail = parv[1];

	if (!(mg = mygroup_find(parv[0])))
	{
		command_fail(si, fault_nosuch_target, _("Group \2%s\2 does not exist."), parv[0]);
		return;
	}
	
	if (!groupacs_sourceinfo_has_flag(mg, si, GA_SET))
	{
		command_fail(si, fault_noprivs, _("You are not authorized to execute this command."));
		return;
	}

	if (!mail || !strcasecmp(mail, "NONE") || !strcasecmp(mail, "OFF"))
	{
		if (metadata_find(mg, "email"))
		{
			metadata_delete(mg, "email");
			command_success_nodata(si, _("The e-mail address for group \2%s\2 was deleted."), entity(mg)->name);
			logcommand(si, CMDLOG_SET, "SET:EMAIL:NONE: \2%s\2", entity(mg)->name);
			return;
		}

		command_fail(si, fault_nochange, _("The e-mail address for group \2%s\2 was not set."), entity(mg)->name);
		return;
	}

	if (!validemail(mail))
	{
		command_fail(si, fault_badparams, _("\2%s\2 is not a valid e-mail address."), mail);
		return;
	}

	/* we'll overwrite any existing metadata */
	metadata_add(mg, "email", mail);

	logcommand(si, CMDLOG_SET, "SET:EMAIL: \2%s\2 \2%s\2", entity(mg)->name, mail);
	command_success_nodata(si, _("The e-mail address for group \2%s\2 has been set to \2%s\2."), parv[0], mail);
}

void _modinit(module_t *m)
{
	use_groupserv_main_symbols(m);
	use_groupserv_set_symbols(m);

	command_add(&gs_set_email, gs_set_cmdtree);
}

void _moddeinit(module_unload_intent_t intent)
{
	command_delete(&gs_set_email, gs_set_cmdtree);
}

