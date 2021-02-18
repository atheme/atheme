/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005 William Pitcock <nenolod -at- nenolod.net>
 * Copyright (C) 2007 Jilles Tjoelker
 *
 * Changes the language services uses to talk to you.
 */

#include <atheme.h>

#ifdef ENABLE_NLS

static mowgli_patricia_t **ns_set_cmdtree = NULL;

// SET LANGUAGE <language>
static void
ns_cmd_set_language(struct sourceinfo *si, int parc, char *parv[])
{
	char *language = parv[0];
	struct language *lang;

	if (!language)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SET LANGUAGE");
		command_fail(si, fault_needmoreparams, _("Valid languages are: %s"), language_names());
		return;
	}

	lang = language_find(language);

	if (strcmp(language, "default") &&
			(lang == NULL || !language_is_valid(lang)))
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "SET LANGUAGE");
		command_fail(si, fault_badparams, _("Valid languages are: %s"), language_names());
		return;
	}

	logcommand(si, CMDLOG_SET, "SET:LANGUAGE: \2%s\2", language_get_name(lang));

	si->smu->language = lang;

	command_success_nodata(si, _("The language for \2%s\2 has been changed to \2%s\2."), entity(si->smu)->name, language_get_name(lang));

	return;
}

static struct command ns_set_language = {
	.name           = "LANGUAGE",
	.desc           = N_("Changes the language services uses to talk to you."),
	.access         = AC_NONE,
	.maxparc        = 1,
	.cmd            = &ns_cmd_set_language,
	.help           = { .path = "nickserv/set_language" },
};

static void
mod_init(struct module *const restrict m)
{
	if (! languages_get_available())
	{
		(void) slog(LG_ERROR, "%s: language support is not available in this installation", m->name);
		(void) slog(LG_ERROR, "%s: please check the log file for startup error messages", m->name);

		m->mflags |= MODFLAG_FAIL;
		return;
	}

	MODULE_TRY_REQUEST_SYMBOL(m, ns_set_cmdtree, "nickserv/set_core", "ns_set_cmdtree")

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

	m->mflags |= MODFLAG_FAIL;
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	// Nothing To Do
}

#endif /* !ENABLE_NLS */

SIMPLE_DECLARE_MODULE_V1("nickserv/set_language", MODULE_UNLOAD_CAPABILITY_OK)
