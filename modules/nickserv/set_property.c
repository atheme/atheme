/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005 William Pitcock <nenolod -at- nenolod.net>
 * Copyright (C) 2007 Jilles Tjoelker
 *
 * Manipulates metadata entries associated with an account.
 */

#include <atheme.h>

static mowgli_patricia_t **ns_set_cmdtree = NULL;

// SET PROPERTY <property> [value]
static void
ns_cmd_set_property(struct sourceinfo *si, int parc, char *parv[])
{
	char *property = strtok(parv[0], " ");
	char *value = strtok(NULL, "");
	unsigned int count;
	mowgli_patricia_iteration_state_t state;
	struct metadata *md;
	struct hook_metadata_change mdchange;

	if (!property)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SET PROPERTY");
		command_fail(si, fault_needmoreparams, _("Syntax: SET PROPERTY <property> [value]"));
		return;
	}

	if (strchr(property, ':') && !has_priv(si, PRIV_METADATA))
	{
		command_fail(si, fault_badparams, _("Invalid property name."));
		return;
	}

	if (strchr(property, ':'))
		logcommand(si, CMDLOG_SET, "SET:PROPERTY: \2%s\2: \2%s\2/\2%s\2", entity(si->smu)->name, property, value);

	if (!value)
	{
		md = metadata_find(si->smu, property);

		if (!md)
		{
			command_fail(si, fault_nosuch_target, _("Metadata entry \2%s\2 was not set."), property);
			return;
		}

		mdchange.target = si->smu;
		mdchange.name = md->name;
		mdchange.value = md->value;
		hook_call_metadata_change(&mdchange);

		metadata_delete(si->smu, property);
		logcommand(si, CMDLOG_SET, "SET:PROPERTY: \2%s\2 (deleted)", property);
		command_success_nodata(si, _("Metadata entry \2%s\2 has been deleted."), property);
		return;
	}

	count = 0;
	MOWGLI_PATRICIA_FOREACH(md, &state, atheme_object(si->smu)->metadata)
	{
		if (strncmp(md->name, "private:", 8))
			count++;
	}
	if (count >= me.mdlimit)
	{
		command_fail(si, fault_toomany, _("Cannot add \2%s\2 to \2%s\2 metadata table, it is full."),
					property, entity(si->smu)->name);
		return;
	}

	if (strlen(property) > 32 || strlen(value) > 300 || has_ctrl_chars(property))
	{
		command_fail(si, fault_badparams, _("Parameters are too long. Aborting."));
		return;
	}

	md = metadata_add(si->smu, property, value);
	if (md != NULL)
	{
		mdchange.target = si->smu;
		mdchange.name = md->name;
		mdchange.value = md->value;
		hook_call_metadata_change(&mdchange);
	}
	logcommand(si, CMDLOG_SET, "SET:PROPERTY: \2%s\2 to \2%s\2", property, value);
	command_success_nodata(si, _("Metadata entry \2%s\2 added."), property);
}

static struct command ns_set_property = {
	.name           = "PROPERTY",
	.desc           = N_("Manipulates metadata entries associated with an account."),
	.access         = AC_NONE,
	.maxparc        = 2,
	.cmd            = &ns_cmd_set_property,
	.help           = { .path = "nickserv/set_property" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_SYMBOL(m, ns_set_cmdtree, "nickserv/set_core", "ns_set_cmdtree")

	command_add(&ns_set_property, *ns_set_cmdtree);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	command_delete(&ns_set_property, *ns_set_cmdtree);
}

SIMPLE_DECLARE_MODULE_V1("nickserv/set_property", MODULE_UNLOAD_CAPABILITY_OK)
