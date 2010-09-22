/*
 * Copyright (c) 2005 William Pitcock <nenolod -at- nenolod.net>
 * Copyright (c) 2007 Jilles Tjoelker
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Changes the language services uses to talk to you.
 *
 */

#include "atheme.h"
#include "uplink.h"

#ifdef ENABLE_NLS

DECLARE_MODULE_V1
(
	"nickserv/set_language", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

mowgli_patricia_t **ns_set_cmdtree;

static void ns_cmd_set_language(sourceinfo_t *si, int parc, char *parv[]);

command_t ns_set_language = { "LANGUAGE", N_("Changes the language services uses to talk to you."), AC_NONE, 1, ns_cmd_set_language, { .path = "nickserv/set_language" } };

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(ns_set_cmdtree, "nickserv/set_core", "ns_set_cmdtree");

	command_add(&ns_set_language, *ns_set_cmdtree);
}

void _moddeinit(void)
{
	command_delete(&ns_set_language, *ns_set_cmdtree);
}

/* SET LANGUAGE <language> */
static void ns_cmd_set_language(sourceinfo_t *si, int parc, char *parv[])
{
	char *language = strtok(parv[0], " ");
	language_t *lang;

	if (si->smu == NULL)
		return;

	if (language == NULL)
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

#endif /* ENABLE_NLS */

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
