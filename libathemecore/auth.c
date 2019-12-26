/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2015 Atheme Project (http://atheme.org/)
 * Copyright (C) 2017-2018 Atheme Development Group (https://atheme.github.io/)
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * atheme-services: A collection of minimalist IRC services
 * auth.c: Authentication.
 */

#include <atheme.h>
#include "internal.h"

bool auth_module_loaded = false;
bool (*auth_user_custom)(struct myuser *mu, const char *password) ATHEME_FATTR_WUR;

void
set_password(struct myuser *const restrict mu, const char *const restrict password)
{
	if (! mu || ! password)
		return;

	const char *const hash = crypt_password(password);

	(void) smemzero(mu->pass, sizeof mu->pass);

	if (hash)
	{
		mu->flags |= MU_CRYPTPASS;

		(void) mowgli_strlcpy(mu->pass, hash, sizeof mu->pass);
	}
	else
	{
		mu->flags &= ~MU_CRYPTPASS;

		(void) mowgli_strlcpy(mu->pass, password, sizeof mu->pass);
		(void) slog(LG_ERROR, "%s: failed to encrypt password for account '%s'",
		                      MOWGLI_FUNC_NAME, entity(mu)->name);
	}
}

bool ATHEME_FATTR_WUR
verify_password(struct myuser *const restrict mu, const char *const restrict password)
{
	if (! mu || ! password)
		return false;

	if (auth_module_loaded && auth_user_custom)
		return auth_user_custom(mu, password);

	if (! (mu->flags & MU_CRYPTPASS))
	{
		(void) slog(LG_INFO, "%s: verifying unencrypted password for account '%s'!",
		                     MOWGLI_FUNC_NAME, entity(mu)->name);

		return (strcmp(mu->pass, password) == 0);
	}

	const char *new_hash;
	const struct crypt_impl *ci, *ci_default;
	unsigned int verify_flags = PWVERIFY_FLAG_NONE;

	if (! (ci = crypt_verify_password(password, mu->pass, &verify_flags)))
		// Verification failure
		return false;

	if (! (ci_default = crypt_get_default_provider()))
		// Verification succeeded but we don't have a module that can create new password hashes
		return true;

	if (ci != ci_default)
		(void) slog(LG_INFO, "%s: transitioning from crypt scheme '%s' to '%s' for account '%s'",
		                     MOWGLI_FUNC_NAME, ci->id, ci_default->id, entity(mu)->name);
	else if (verify_flags & PWVERIFY_FLAG_RECRYPT)
		(void) slog(LG_INFO, "%s: re-encrypting password for account '%s'",
		                     MOWGLI_FUNC_NAME, entity(mu)->name);
	else
		// Verification succeeded and re-encrypting not required, nothing more to do
		return true;

	if (! (new_hash = ci_default->crypt(password, NULL)))
	{
		(void) slog(LG_ERROR, "%s: hash generation failed", MOWGLI_FUNC_NAME);
	}
	else
	{
		(void) smemzero(mu->pass, sizeof mu->pass);
		(void) mowgli_strlcpy(mu->pass, new_hash, sizeof mu->pass);
	}

	// Verification succeeded and user's password (possibly) re-encrypted
	return true;
}
