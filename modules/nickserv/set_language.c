/*
 * Copyright (c) 2005 William Pitcock <nenolod -at- nenolod.net>
 * Copyright (c) 2007 Jilles Tjoelker
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Changes the language services uses to talk to you.
 */

#include "atheme.h"

#ifdef ENABLE_NLS

#include "uplink.h"

static mowgli_patricia_t **ns_set_cmdtree = NULL;

// SET LANGUAGE <language>
static void
ns_cmd_set_language(struct sourceinfo *si, int parc, char *parv[])
{
	char *language = parv[0];
	struct language *lang;

	if (!language)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "LANGUAGE");
		command_fail(si, fault_needmoreparams, _("Valid languages are: %s"), language_names());
		return;
	}

	lang = language_find(language);

	if (strcmp(language, "default") &&
			(lang == NULL || !language_is_valid(lang)))
	{
		command_fail(si, fault_badparams, _("Invalid language \2%s\2."), language);
		command_fail(si, fault_badparams, _("Valid languages are: %s"), language_names());
		return;
	}

	logcommand(si, CMDLOG_SET, "SET:LANGUAGE: \2%s\2", language_get_name(lang));

	si->smu->language = lang;

	command_success_nodata(si, _("The language for \2%s\2 has been changed to \2%s\2."), entity(si->smu)->name, language_get_name(lang));

	return;
}

static struct command ns_set_language = { "LANGUAGE", N_("Changes the language services uses to talk to you."), AC_NONE, 1, ns_cmd_set_language, { .path = "nickserv/set_language" } };

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_SYMBOL(m, ns_set_cmdtree, "nickserv/set_core", "ns_set_cmdtree");

	command_add(&ns_set_language, *ns_set_cmdtree);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	command_delete(&ns_set_language, *ns_set_cmdtree);
}

#else /* ENABLE_NLS */

static void
mod_init(struct module *const restrict m)
{
	(void) slog(LG_ERROR, "Module %s requires NLS support, refusing to load.", m->name);

	m->mflags |= MODTYPE_FAIL;
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	// Nothing To Do
}

#endif /* !ENABLE_NLS */

SIMPLE_DECLARE_MODULE_V1("nickserv/set_language", MODULE_UNLOAD_CAPABILITY_OK)
