/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Copyright (c) 2006-2010 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains routines to handle the CService SET PROPERTY command.
 */

#include "atheme.h"

static void cs_cmd_set_property(struct sourceinfo *si, int parc, char *parv[]);

static struct command cs_set_property = { "PROPERTY", N_("Manipulates channel metadata."), AC_NONE, 2, cs_cmd_set_property, { .path = "cservice/set_property" } };

static mowgli_patricia_t **cs_set_cmdtree = NULL;

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_SYMBOL(m, cs_set_cmdtree, "chanserv/set_core", "cs_set_cmdtree");

	command_add(&cs_set_property, *cs_set_cmdtree);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	command_delete(&cs_set_property, *cs_set_cmdtree);
}

static void
cs_cmd_set_property(struct sourceinfo *si, int parc, char *parv[])
{
	struct mychan *mc;
	char *property = strtok(parv[1], " ");
	char *value = strtok(NULL, "");
	unsigned int count;
	mowgli_patricia_iteration_state_t state;
	struct metadata *md;

	if (!property)
	{
		command_fail(si, fault_needmoreparams, _("Syntax: SET <#channel> PROPERTY <property> [value]"));
		return;
	}

	/* do we really need to allow this? -- jilles */
	if (strchr(property, ':') && !has_priv(si, PRIV_METADATA))
	{
		command_fail(si, fault_badparams, _("Invalid property name."));
		return;
	}

	if (!(mc = mychan_find(parv[0])))
	{
		command_fail(si, fault_nosuch_target, _("Channel \2%s\2 is not registered."), parv[0]);
		return;
	}

	if (!is_founder(mc, entity(si->smu)))
	{
		command_fail(si, fault_noprivs, _("You are not authorized to perform this command."));
		return;
	}

	if (strchr(property, ':'))
		logcommand(si, CMDLOG_SET, "SET:PROPERTY: \2%s\2: \2%s\2/\2%s\2", mc->name, property, value);

	if (!value)
	{
		md = metadata_find(mc, property);

		if (!md)
		{
			command_fail(si, fault_nochange, _("Metadata entry \2%s\2 was not set."), property);
			return;
		}

		metadata_delete(mc, property);
		logcommand(si, CMDLOG_SET, "SET:PROPERTY: \2%s\2 \2%s\2 (deleted)", mc->name, property);
		verbose(mc, _("\2%s\2 deleted the metadata entry \2%s\2"), get_source_name(si), property);
		command_success_nodata(si, _("Metadata entry \2%s\2 has been deleted."), property);
		return;
	}

	count = 0;
	if (atheme_object(mc)->metadata)
	{
		MOWGLI_PATRICIA_FOREACH(md, &state, atheme_object(mc)->metadata)
		{
			if (strncmp(md->name, "private:", 8))
				count++;
		}
	}
	if (count >= me.mdlimit)
	{
		command_fail(si, fault_toomany, _("Cannot add \2%s\2 to \2%s\2 metadata table, it is full."),
						property, parv[0]);
		return;
	}

	if (strlen(property) > 32 || strlen(value) > 300 || has_ctrl_chars(property))
	{
		command_fail(si, fault_badparams, _("Parameters are too long. Aborting."));
		return;
	}

	metadata_add(mc, property, value);
	logcommand(si, CMDLOG_SET, "SET:PROPERTY: \2%s\2 on \2%s\2 to \2%s\2", property, mc->name, value);
	verbose(mc, _("\2%s\2 added the metadata entry \2%s\2 with value \2%s\2"), get_source_name(si), property, value);
	command_success_nodata(si, _("Metadata entry \2%s\2 added."), property);
}

SIMPLE_DECLARE_MODULE_V1("chanserv/set_property", MODULE_UNLOAD_CAPABILITY_OK)
