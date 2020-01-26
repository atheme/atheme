/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2010 William Pitcock <nenolod@dereferenced.org>
 * Copyright (C) 2019 Atheme Development Group (https://atheme.github.io/)
 *
 * This file contains code for checking password security.
 */

#include <atheme.h>

#ifdef HAVE_ANY_PASSWORD_QUALITY_LIBRARY

#ifdef HAVE_CRACKLIB
#  include <crack.h>
#endif /* HAVE_CRACKLIB */

#ifdef HAVE_LIBPASSWDQC
#  include <passwdqc.h>
#endif /* HAVE_LIBPASSWDQC */

#ifdef HAVE_CRACKLIB
static char *cracklib_dict = NULL;
#endif /* HAVE_CRACKLIB */

#ifdef HAVE_LIBPASSWDQC

#define PASSWDQC_N0_DEF     20
#define PASSWDQC_N1_DEF     16
#define PASSWDQC_N2_DEF     16
#define PASSWDQC_N3_DEF     12
#define PASSWDQC_N4_DEF     8

#define PASSWDQC_PW_MIN     2
#define PASSWDQC_PW_DEF     4
#define PASSWDQC_PW_MAX     8

#define PASSWDQC_PM_MIN     8
#define PASSWDQC_PM_DEF     PASSLEN
#define PASSWDQC_PM_MAX     PASSLEN

static unsigned int passwdqc_min_n0 = PASSWDQC_N0_DEF;
static unsigned int passwdqc_min_n1 = PASSWDQC_N1_DEF;
static unsigned int passwdqc_min_n2 = PASSWDQC_N2_DEF;
static unsigned int passwdqc_min_n3 = PASSWDQC_N3_DEF;
static unsigned int passwdqc_min_n4 = PASSWDQC_N4_DEF;
static unsigned int passwdqc_words = PASSWDQC_PW_DEF;
static unsigned int passwdqc_max = PASSWDQC_PM_DEF;

static passwdqc_params_qc_t qc_config = {

	/* Set in pwquality_config_check() below */
	.min                = { 0, 0, 0, 0, 0 },
	.max                = 0,
	.passphrase_words   = 0,

	/* We don't have access to the user's old password */
	.match_length       = 0,
	.similar_deny       = 0,
};

#endif /* HAVE_LIBPASSWDQC */

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

#ifdef HAVE_LIBPASSWDQC
	if (passwdqc_min_n0 > passwdqc_max)
	{
		(void) slog(LG_ERROR, "Bad config: nickserv::passwdqc_min_n0 is more than nickserv::passwdqc_max");
		return false;
	}
	if (passwdqc_min_n1 > passwdqc_min_n0)
	{
		(void) slog(LG_ERROR, "Bad config: nickserv::passwdqc_min_n1 is more than nickserv::passwdqc_min_n0");
		return false;
	}
	if (passwdqc_min_n2 > passwdqc_min_n1)
	{
		(void) slog(LG_ERROR, "Bad config: nickserv::passwdqc_min_n2 is more than nickserv::passwdqc_min_n1");
		return false;
	}
	if (passwdqc_min_n3 > passwdqc_min_n2)
	{
		(void) slog(LG_ERROR, "Bad config: nickserv::passwdqc_min_n3 is more than nickserv::passwdqc_min_n2");
		return false;
	}
	if (passwdqc_min_n4 > passwdqc_min_n3)
	{
		(void) slog(LG_ERROR, "Bad config: nickserv::passwdqc_min_n4 is more than nickserv::passwdqc_min_n3");
		return false;
	}

	qc_config.passphrase_words = (int) passwdqc_words;
	qc_config.min[0] = (int) passwdqc_min_n0;
	qc_config.min[1] = (int) passwdqc_min_n1;
	qc_config.min[2] = (int) passwdqc_min_n2;
	qc_config.min[3] = (int) passwdqc_min_n3;
	qc_config.min[4] = (int) passwdqc_min_n4;
	qc_config.max = (int) passwdqc_max;
#endif /* HAVE_LIBPASSWDQC */

	return true;
}

static void
pwquality_register_hook(struct hook_user_register_check *const restrict hdata)
{
	return_if_fail(hdata != NULL);
	return_if_fail(hdata->si != NULL);
	return_if_fail(hdata->password != NULL);

	bool good_password = true;

	if (! pwquality_config_check())
		return;

#ifdef HAVE_CRACKLIB
	const char *cl_reason = NULL;
	if ((cl_reason = FascistCheck(hdata->password, cracklib_dict)))
		good_password = false;
#endif /* HAVE_CRACKLIB */

#ifdef HAVE_LIBPASSWDQC
	const char *qc_reason = NULL;
	if ((qc_reason = passwdqc_check(&qc_config, hdata->password, NULL, NULL)))
		good_password = false;
#endif /* HAVE_LIBPASSWDQC */

	if (good_password)
		return;

#ifdef HAVE_CRACKLIB
	if (cl_reason)
		(void) command_fail(hdata->si, fault_badparams, _("Password is insecure [cracklib]: %s"), cl_reason);
#endif /* HAVE_CRACKLIB */

#ifdef HAVE_LIBPASSWDQC
	if (qc_reason)
		(void) command_fail(hdata->si, fault_badparams, _("Password is insecure [passwdqc]: %s"), qc_reason);
#endif /* HAVE_LIBPASSWDQC */

	if (pwquality_warn_only)
	{
		(void) command_fail(hdata->si, fault_badparams, _("You may want to set a different password with: "
		                                                  "\2/msg %s SET PASSWORD <newpassword>\2"),
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
	(void) command_success_nodata(si, _("Cracklib dictionary path: %s"), cracklib_dict);
#endif /* HAVE_CRACKLIB */

#ifdef HAVE_LIBPASSWDQC
	(void) command_success_nodata(si, _("Configuration for libpasswdqc: min:[%u,%u,%u,%u,%u], max:%u, words:%u"),
	                                    passwdqc_min_n0, passwdqc_min_n1, passwdqc_min_n2, passwdqc_min_n3,
	                                    passwdqc_min_n4, passwdqc_max, passwdqc_words);
#endif /* HAVE_LIBPASSWDQC */

	(void) command_success_nodata(si, _("Registrations will fail with bad passwords: %s"),
	                                  pwquality_warn_only ? _("No") : _("Yes"));
}

static void
mod_init(struct module *const restrict m)
{
	(void) hook_add_user_can_register(&pwquality_register_hook);
	(void) hook_add_operserv_info(&pwquality_osinfo_hook);

	(void) add_bool_conf_item("PWQUALITY_WARN_ONLY", &nicksvs.me->conf_table, 0, &pwquality_warn_only, false);

#ifdef HAVE_CRACKLIB
	(void) add_dupstr_conf_item("CRACKLIB_DICT", &nicksvs.me->conf_table, 0, &cracklib_dict, NULL);
#endif /* HAVE_CRACKLIB */

#ifdef HAVE_LIBPASSWDQC
	(void) add_uint_conf_item("PASSWDQC_MIN_N0", &nicksvs.me->conf_table, 0, &passwdqc_min_n0, 0,
	                          PASSWDQC_PM_MAX, PASSWDQC_N0_DEF);
	(void) add_uint_conf_item("PASSWDQC_MIN_N1", &nicksvs.me->conf_table, 0, &passwdqc_min_n1, 0,
	                          PASSWDQC_PM_MAX, PASSWDQC_N1_DEF);
	(void) add_uint_conf_item("PASSWDQC_MIN_N2", &nicksvs.me->conf_table, 0, &passwdqc_min_n2, 0,
	                          PASSWDQC_PM_MAX, PASSWDQC_N2_DEF);
	(void) add_uint_conf_item("PASSWDQC_MIN_N3", &nicksvs.me->conf_table, 0, &passwdqc_min_n3, 0,
	                          PASSWDQC_PM_MAX, PASSWDQC_N3_DEF);
	(void) add_uint_conf_item("PASSWDQC_MIN_N4", &nicksvs.me->conf_table, 0, &passwdqc_min_n4, 0,
	                          PASSWDQC_PM_MAX, PASSWDQC_N4_DEF);
	(void) add_uint_conf_item("PASSWDQC_MAX", &nicksvs.me->conf_table, 0, &passwdqc_max, PASSWDQC_PM_MIN,
	                          PASSWDQC_PM_MAX, PASSWDQC_PM_DEF);
	(void) add_uint_conf_item("PASSWDQC_WORDS", &nicksvs.me->conf_table, 0, &passwdqc_words, PASSWDQC_PW_MIN,
	                          PASSWDQC_PW_MAX, PASSWDQC_PW_DEF);
#endif /* HAVE_LIBPASSWDQC */
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

#ifdef HAVE_LIBPASSWDQC
	(void) del_conf_item("PASSWDQC_MIN_N0", &nicksvs.me->conf_table);
	(void) del_conf_item("PASSWDQC_MIN_N1", &nicksvs.me->conf_table);
	(void) del_conf_item("PASSWDQC_MIN_N2", &nicksvs.me->conf_table);
	(void) del_conf_item("PASSWDQC_MIN_N3", &nicksvs.me->conf_table);
	(void) del_conf_item("PASSWDQC_MIN_N4", &nicksvs.me->conf_table);
	(void) del_conf_item("PASSWDQC_MAX", &nicksvs.me->conf_table);
	(void) del_conf_item("PASSWDQC_WORDS", &nicksvs.me->conf_table);
#endif /* HAVE_LIBPASSWDQC */
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
