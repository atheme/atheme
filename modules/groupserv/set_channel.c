
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
	"groupserv/set_channel", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void gs_cmd_set_channel(sourceinfo_t *si, int parc, char *parv[]);

command_t gs_set_channel = { "CHANNEL", N_("Sets the official group channel."), AC_AUTHENTICATED, 2, gs_cmd_set_channel, { .path = "groupserv/set_channel" } };

static void gs_cmd_set_channel(sourceinfo_t *si, int parc, char *parv[])
{
	mygroup_t *mg;
	char *chan = parv[1];

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

	if (!chan || !strcasecmp("OFF", chan) || !strcasecmp("NONE", chan))
	{
		/* not in a namespace to allow more natural use of SET PROPERTY.
		 * they may be able to introduce spaces, though. c'est la vie.
		 */
		if (metadata_find(mg, "channel"))
		{
			metadata_delete(mg, "channel");
			logcommand(si, CMDLOG_SET, "SET:CHANNEL:NONE: \2%s\2", entity(mg)->name);
			command_success_nodata(si, _("The official channel for \2%s\2 has been cleared."), parv[0]);
			return;
		}

		command_fail(si, fault_nochange, _("A official channel for \2%s\2 was not set."), parv[0]);
		return;
	}

	/* we'll overwrite any existing metadata */
	metadata_add(mg, "channel", chan);

	logcommand(si, CMDLOG_SET, "SET:CHANNEL: \2%s\2 \2%s\2", entity(mg)->name, chan);
	command_success_nodata(si, _("The official channel of \2%s\2 has been set to \2%s\2."), parv[0], chan);
}

void _modinit(module_t *m)
{
	use_groupserv_main_symbols(m);
	use_groupserv_set_symbols(m);

	command_add(&gs_set_channel, gs_set_cmdtree);
}

void _moddeinit(module_unload_intent_t intent)
{
	command_delete(&gs_set_channel, gs_set_cmdtree);
}

