/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Lists object properties via their metadata table.
 */

#include "atheme.h"

static void ns_cmd_taxonomy(struct sourceinfo *si, int parc, char *parv[]);

static struct command ns_taxonomy = { "TAXONOMY", N_("Displays a user's metadata."), AC_NONE, 1, ns_cmd_taxonomy, { .path = "nickserv/taxonomy" } };

static void
ns_cmd_taxonomy(struct sourceinfo *si, int parc, char *parv[])
{
	const char *target = parv[0];
	struct myuser *mu;
	mowgli_patricia_iteration_state_t state;
	bool isoper;
	struct metadata *md;

	if (!target && si->smu)
		target = entity(si->smu)->name;
	if (!target)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "TAXONOMY");
		command_fail(si, fault_needmoreparams, _("Syntax: TAXONOMY <nick>"));
		return;
	}

	if (!(mu = myuser_find_ext(target)))
	{
		command_fail(si, fault_badparams, _("\2%s\2 is not registered."), target);
		return;
	}

	isoper = has_priv(si, PRIV_USER_AUSPEX);
	if (isoper)
		logcommand(si, CMDLOG_ADMIN, "TAXONOMY: \2%s\2 (oper)", entity(mu)->name);
	else
		logcommand(si, CMDLOG_GET, "TAXONOMY: \2%s\2", entity(mu)->name);

	command_success_nodata(si, _("Taxonomy for \2%s\2:"), entity(mu)->name);

	MOWGLI_PATRICIA_FOREACH(md, &state, atheme_object(mu)->metadata)
	{
		if (!strncmp(md->name, "private:", 8) && !isoper)
			continue;

		command_success_nodata(si, "%-32s: %s", md->name, md->value);
	}

	command_success_nodata(si, _("End of \2%s\2 taxonomy."), entity(mu)->name);
}

static void
mod_init(struct module ATHEME_VATTR_UNUSED *const restrict m)
{
	service_named_bind_command("nickserv", &ns_taxonomy);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("nickserv", &ns_taxonomy);
}

SIMPLE_DECLARE_MODULE_V1("nickserv/taxonomy", MODULE_UNLOAD_CAPABILITY_OK)
