/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2018 Atheme Development Group (https://atheme.github.io/)
 *
 * Old POSIX module which just wraps around the 2 that replaced it.
 */

#include "atheme.h"

static void
mod_init(struct module *const restrict m)
{
	(void) slog(LG_INFO, "%s: this module has been replaced! Please read dist/atheme.conf.example", m->name);

#if defined(HAVE_CRYPT) && defined(ATHEME_ENABLE_LEGACY_PWCRYPTO)

	(void) slog(LG_INFO, "%s: loading the 2 modules that replaced me for config compatibility ...", m->name);

	MODULE_TRY_REQUEST_DEPENDENCY(m, "crypto/crypt3-md5");
	MODULE_TRY_REQUEST_DEPENDENCY(m, "crypto/crypt3-des");

#else /* HAVE_CRYPT && ATHEME_ENABLE_LEGACY_PWCRYPTO */

	(void) slog(LG_ERROR, "%s: crypt(3) and/or legacy password crypto modules are unavailable", m->name);

	m->mflags |= MODFLAG_FAIL;

#endif /* !HAVE_CRYPT || !ATHEME_ENABLE_LEGACY_PWCRYPTO */
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{

}

SIMPLE_DECLARE_MODULE_V1("crypto/posix", MODULE_UNLOAD_CAPABILITY_OK)
