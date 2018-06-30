/*
 * Copyright (c) 2010 William Pitcock <nenolod@atheme.org>
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for checking password security with cracklib.
 */

#include "atheme.h"

#ifdef HAVE_CRACKLIB

#include "conf.h"

#include <sys/stat.h>
#include <crack.h>

static bool cracklib_warn = false;

static void
cracklib_config_ready(void *unused)
{
	struct module *m;
	m = module_find_published("nickserv/cracklib");

	if (nicksvs.cracklib_dict == NULL)
	{
		slog(LG_INFO, "%s requires that the cracklib_dict configuration option be set.", m->name);
		module_unload(m, MODULE_UNLOAD_INTENT_PERM);
		return;
	}

	hook_del_config_ready(cracklib_config_ready);
}

static int
check_dict(char *filename)
{
	struct stat sb;

	if (stat(filename, &sb) == -1)
	{
		slog(LG_ERROR, "No cracklib dictionary found, falling back to not using a dictionary.");
		return 0;
	}
	else
		return 1;
}

static void
cracklib_hook(hook_user_register_check_t *hdata)
{
	const char *cracklib_reason;
	char dict[BUFSIZE];

	mowgli_strlcpy(dict, nicksvs.cracklib_dict, BUFSIZE);
	mowgli_strlcat(dict, ".pwd", BUFSIZE);

	return_if_fail(hdata != NULL);
	return_if_fail(hdata->si != NULL);
	return_if_fail(hdata->password != NULL);

	if ((cracklib_reason = FascistCheck(hdata->password, check_dict(dict) ? nicksvs.cracklib_dict : NULL)) != NULL)
	{
		if (cracklib_warn)
			command_fail(hdata->si, fault_badparams, _("The password provided is insecure because %s. You may want to set a different password with /msg %s set password <password> ."), cracklib_reason, nicksvs.nick);
		else
		{
			command_fail(hdata->si, fault_badparams, _("The password provided is insecure: %s"), cracklib_reason);
			hdata->approved++;
		}
	}
}

static void
osinfo_hook(struct sourceinfo *si)
{
	return_if_fail(si != NULL);

	command_success_nodata(si, "Registrations will fail with bad passwords: %s", cracklib_warn ? "No" : "Yes");
}

static void
mod_init(struct module ATHEME_VATTR_UNUSED *const restrict m)
{
	hook_add_event("user_can_register");
	hook_add_user_can_register(cracklib_hook);

	hook_add_event("config_ready");
	hook_add_config_ready(cracklib_config_ready);

	hook_add_event("operserv_info");
	hook_add_operserv_info(osinfo_hook);

	add_bool_conf_item("CRACKLIB_WARN", &nicksvs.me->conf_table, 0, &cracklib_warn, false);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	hook_del_user_can_register(cracklib_hook);
	hook_del_config_ready(cracklib_config_ready);
	hook_del_operserv_info(osinfo_hook);

	del_conf_item("CRACKLIB_WARN", &nicksvs.me->conf_table);
}

#else /* HAVE_CRACKLIB */

static void
mod_init(struct module *const restrict m)
{
	(void) slog(LG_ERROR, "Module %s requires cracklib support, refusing to load.", m->name);

	m->mflags |= MODFLAG_FAIL;
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	// Nothing To Do
}

#endif /* !HAVE_CRACKLIB */

SIMPLE_DECLARE_MODULE_V1("nickserv/cracklib", MODULE_UNLOAD_CAPABILITY_OK)
