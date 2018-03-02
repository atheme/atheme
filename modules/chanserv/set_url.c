/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Copyright (c) 2006-2010 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains routines to handle the CService SET URL command.
 */

#include "atheme.h"

static mowgli_patricia_t **cs_set_cmdtree = NULL;

static void
cs_cmd_set_url(struct sourceinfo *si, int parc, char *parv[])
{
	struct mychan *mc;
	char *url = parv[1];

	if (!(mc = mychan_find(parv[0])))
	{
		command_fail(si, fault_nosuch_target, _("Channel \2%s\2 is not registered."), parv[0]);
		return;
	}

	if (!chanacs_source_has_flag(mc, si, CA_SET))
	{
		command_fail(si, fault_noprivs, _("You are not authorized to execute this command."));
		return;
	}

	if (!url || !strcasecmp("OFF", url) || !strcasecmp("NONE", url))
	{
		/* not in a namespace to allow more natural use of SET PROPERTY.
		 * they may be able to introduce spaces, though. c'est la vie.
		 */
		if (metadata_find(mc, "url"))
		{
			metadata_delete(mc, "url");
			logcommand(si, CMDLOG_SET, "SET:URL:NONE: \2%s\2", mc->name);
			verbose(mc, _("\2%s\2 cleared the channel URL"), get_source_name(si));
			command_success_nodata(si, _("The URL for \2%s\2 has been cleared."), parv[0]);
			return;
		}

		command_fail(si, fault_nochange, _("The URL for \2%s\2 was not set."), parv[0]);
		return;
	}

	// we'll overwrite any existing metadata
	metadata_add(mc, "url", url);

	logcommand(si, CMDLOG_SET, "SET:URL: \2%s\2 \2%s\2", mc->name, url);
	verbose(mc, _("\2%s\2 set the channel URL to \2%s\2"), get_source_name(si), url);
	command_success_nodata(si, _("The URL of \2%s\2 has been set to \2%s\2."), parv[0], url);
}

static struct command cs_set_url = { "URL", N_("Sets the channel URL."), AC_NONE, 2, cs_cmd_set_url, { .path = "cservice/set_url" } };

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_SYMBOL(m, cs_set_cmdtree, "chanserv/set_core", "cs_set_cmdtree");

	command_add(&cs_set_url, *cs_set_cmdtree);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	command_delete(&cs_set_url, *cs_set_cmdtree);
}

SIMPLE_DECLARE_MODULE_V1("chanserv/set_url", MODULE_UNLOAD_CAPABILITY_OK)
