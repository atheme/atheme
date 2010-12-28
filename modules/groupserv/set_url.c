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
	"groupserv/set_url", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void gs_cmd_set_url(sourceinfo_t *si, int parc, char *parv[]);

command_t gs_set_url = { "URL", N_("Sets the group URL."), AC_AUTHENTICATED, 2, gs_cmd_set_url, { .path = "groupserv/set_url" } };

static void gs_cmd_set_url(sourceinfo_t *si, int parc, char *parv[])
{
	mygroup_t *mg;
	char *url = parv[1];

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

	if (!url || !strcasecmp("OFF", url) || !strcasecmp("NONE", url))
	{
		/* not in a namespace to allow more natural use of SET PROPERTY.
		 * they may be able to introduce spaces, though. c'est la vie.
		 */
		if (metadata_find(mg, "url"))
		{
			metadata_delete(mg, "url");
			logcommand(si, CMDLOG_SET, "SET:URL:NONE: \2%s\2", entity(mg)->name);
			command_success_nodata(si, _("The URL for \2%s\2 has been cleared."), parv[0]);
			return;
		}

		command_fail(si, fault_nochange, _("The URL for \2%s\2 was not set."), parv[0]);
		return;
	}

	/* we'll overwrite any existing metadata */
	metadata_add(mg, "url", url);

	logcommand(si, CMDLOG_SET, "SET:URL: \2%s\2 \2%s\2", entity(mg)->name, url);
	command_success_nodata(si, _("The URL of \2%s\2 has been set to \2%s\2."), parv[0], url);
}

void _modinit(module_t *m)
{
	use_groupserv_main_symbols(m);
	use_groupserv_set_symbols(m);

	command_add(&gs_set_url, gs_set_cmdtree);
}

void _moddeinit(module_unload_intent_t intent)
{
	command_delete(&gs_set_url, gs_set_cmdtree);
}

