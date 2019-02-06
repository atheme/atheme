/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2010 William Pitcock <nenolod@atheme.org>
 * Copyright (C) 2019 Atheme Development Group (https://atheme.github.io/)
 *
 * This file contains code for checking password security.
 */

#include "atheme.h"

#ifdef HAVE_ANY_PASSWORD_QUALITY_LIBRARY

#ifdef HAVE_CRACKLIB
#  include <crack.h>
#endif /* HAVE_CRACKLIB */

#ifdef HAVE_PASSWDQC
#  include <passwdqc.h>
#endif /* HAVE_PASSWDQC */

#ifdef HAVE_CRACKLIB
static char *cracklib_dict = NULL;
#endif /* HAVE_CRACKLIB */

#ifdef HAVE_PASSWDQC
static passwdqc_params_qc_t qc_config = {
	.min                    = { 32, 24, 11, 8, 7 },
	.max                    = PASSLEN,
	.passphrase_words       = 3,
	.match_length           = 4,
};
#endif /* HAVE_PASSWDQC */

static bool pwquality_warn_only = false;

static bool
pwquality_config_check(void)
{
#ifdef HAVE_CRACKLIB
	if (! cracklib_dict)
	{
		(void) slog(LG_ERROR, "The cracklib dictionary is not configured");
		return false;
	}

	char dictpath[BUFSIZE];
	(void) snprintf(dictpath, sizeof dictpath, "%s.pwd", cracklib_dict);

	struct stat sb;
	if (stat(dictpath, &sb) == -1)
	{
		(void) slog(LG_ERROR, "The cracklib dictionary at '%s' is not usable", cracklib_dict);
		return false;
	}
#endif /* HAVE_CRACKLIB */

	return true;
}

static void
pwquality_register_hook(hook_user_register_check_t *const restrict hdata)
{
	return_if_fail(hdata != NULL);
	return_if_fail(hdata->si != NULL);
	return_if_fail(hdata->password != NULL);

	bool good_password = true;
	const char *cl_reason = NULL;
	const char *qc_reason = NULL;

	if (! pwquality_config_check())
		return;

#ifdef HAVE_CRACKLIB
	if ((cl_reason = FascistCheck(hdata->password, cracklib_dict)))
		good_password = false;
#endif /* HAVE_CRACKLIB */

#ifdef HAVE_PASSWDQC
	if ((qc_reason = passwdqc_check(&qc_config, hdata->password, NULL, NULL)))
		good_password = false;
#endif /* HAVE_PASSWDQC */

	if (good_password)
		return;

	if (cl_reason)
		(void) command_fail(hdata->si, fault_badparams, _("Password is insecure [cracklib]: %s"), cl_reason);

	if (qc_reason)
		(void) command_fail(hdata->si, fault_badparams, _("Password is insecure [passwdqc]: %s"), qc_reason);

	if (pwquality_warn_only)
	{
		(void) command_fail(hdata->si, fault_badparams, _("You may want to set a different password with: "
		                                                  "\002/msg %s SET PASSWORD <newpassword>\002"),
		                                                  nicksvs.me->disp);
		return;
	}

	(void) command_fail(hdata->si, fault_badparams, _("Registration denied due to insecure password"));
	hdata->approved++;
}

static void
pwquality_osinfo_hook(struct sourceinfo *const restrict si)
{
	return_if_fail(si != NULL);

	if (! pwquality_config_check())
		return;

#ifdef HAVE_CRACKLIB
	(void) command_success_nodata(si, "Cracklib dictionary path: %s", cracklib_dict);
#endif /* HAVE_PASSWDQC */

	(void) command_success_nodata(si, "Registrations will fail with bad passwords: %s",
	                                  pwquality_warn_only ? "No" : "Yes");
}

static void
mod_init(struct module ATHEME_VATTR_UNUSED *const restrict m)
{
	(void) hook_add_event("user_can_register");
	(void) hook_add_user_can_register(&pwquality_register_hook);

	(void) hook_add_event("operserv_info");
	(void) hook_add_operserv_info(&pwquality_osinfo_hook);

	(void) add_bool_conf_item("PWQUALITY_WARN_ONLY", &nicksvs.me->conf_table, 0, &pwquality_warn_only, false);

#ifdef HAVE_CRACKLIB
	(void) add_dupstr_conf_item("CRACKLIB_DICT", &nicksvs.me->conf_table, 0, &cracklib_dict, NULL);
#endif /* HAVE_CRACKLIB */
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	(void) hook_del_user_can_register(&pwquality_register_hook);
	(void) hook_del_operserv_info(&pwquality_osinfo_hook);

	(void) del_conf_item("PWQUALITY_WARN_ONLY", &nicksvs.me->conf_table);

#ifdef HAVE_CRACKLIB
	(void) del_conf_item("CRACKLIB_DICT", &nicksvs.me->conf_table);
#endif /* HAVE_CRACKLIB */
}

#else /* HAVE_ANY_PASSWORD_QUALITY_LIBRARY */

static void
mod_init(struct module *const restrict m)
{
	(void) slog(LG_ERROR, "Module %s requires a password quality library, refusing to load.", m->name);

	m->mflags |= MODFLAG_FAIL;
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	// Nothing To Do
}

#endif /* !HAVE_ANY_PASSWORD_QUALITY_LIBRARY */

SIMPLE_DECLARE_MODULE_V1("nickserv/pwquality", MODULE_UNLOAD_CAPABILITY_OK)
