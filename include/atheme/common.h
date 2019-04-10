/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2006 Atheme Project (http://atheme.org/)
 *
 * Defines needed by multiple header files.
 */

#ifndef ATHEME_INC_COMMON_H
#define ATHEME_INC_COMMON_H 1

#include <atheme/stdheaders.h>

enum cmd_faultcode
{
	fault_needmoreparams	= 1,
	fault_badparams		= 2,
	fault_nosuch_source	= 3,
	fault_nosuch_target	= 4,
	fault_authfail		= 5,
	fault_noprivs		= 6,
	fault_nosuch_key	= 7,
	fault_alreadyexists	= 8,
	fault_toomany		= 9,
	fault_emailfail		= 10,
	fault_notverified	= 11,
	fault_nochange		= 12,
	fault_already_authed	= 13,
	fault_unimplemented	= 14,
	fault_badauthcookie	= 15,
	fault_internalerror	= 16,
};

/* Causes a warning if value is not of type (or compatible), returning value. */
#define ENSURE_TYPE(value, type) (true ? (value) : (type)0)

/* Returns the size of an array. */
#define ARRAY_SIZE(array) sizeof((array)) / sizeof(*(array))

/* Continue if an assertion fails. */
#define	continue_if_fail(x)						\
	if (!(x)) { 							\
		mowgli_log("critical: Assertion '%s' failed.", #x);	\
		continue;						\
	}

/* strshare.c - stringref management */
typedef const char *stringref;

void strshare_init(void);
stringref strshare_get(const char *str);
stringref strshare_ref(stringref str);
void strshare_unref(stringref str);

#endif /* !ATHEME_INC_COMMON_H */
