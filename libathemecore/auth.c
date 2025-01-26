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

bool ATHEME_FATTR_WUR
set_password(struct myuser *const restrict mu, const char *const restrict password)
{
	return_val_if_fail(mu != NULL, false);
	return_val_if_fail(password != NULL, false);
	return_val_if_fail(*password != 0x00, false);

	const char *const hash = crypt_password(password);

	if (! hash)
	{
		(void) slog(LG_DEBUG, "%s: failed to encrypt password for account '%s'",
		                      MOWGLI_FUNC_NAME, entity(mu)->name);
		return false;
	}

	mu->flags |= MU_CRYPTPASS;

	(void) smemzero(mu->pass, sizeof mu->pass);
	(void) mowgli_strlcpy(mu->pass, hash, sizeof mu->pass);
	(void) hook_call_myuser_changed_password_or_hash(mu);

	return true;
}

bool ATHEME_FATTR_WUR
verify_password(struct myuser *const restrict mu, const char *const restrict password)
{
	return_val_if_fail(mu != NULL, false);
	return_val_if_fail(password != NULL, false);
	return_val_if_fail(*password != 0x00, false);

	if (auth_module_loaded && auth_user_custom)
		return auth_user_custom(mu, password);

	if (! (mu->flags & MU_CRYPTPASS))
	{
		(void) slog(LG_INFO, "%s: verifying unencrypted password for account '%s'!",
		                     MOWGLI_FUNC_NAME, entity(mu)->name);

		/* This is not a constant-time comparison, but it can't be, because we do not have, at
		 * this point, a crypto module with a fixed-length output (and thus could use a constant-
		 * time memory comparison function) to verify it.
		 */
		if (strcmp(mu->pass, password) != 0)
			return false;

		/* Try set_password() instead of the default crypto provider (chosen below) on the basis
		 * that any encrypted password is better than a plaintext one. If the default crypto
		 * provider were to fail, it would remain unencrypted, while set_password() will fall
		 * back to any other encryption-capable crypto providers if the default crypto provider
		 * fails.
		 *
		 *   -- amdj
		 */
		if (! set_password(mu, password))
			(void) slog(LG_DEBUG, "%s: failed to encrypt password for account '%s'",
			                      MOWGLI_FUNC_NAME, entity(mu)->name);

		// Verification succeeded, hopefully password encrypted
		return true;
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
		(void) slog(LG_DEBUG, "%s: failed to re-encrypt password for account '%s'",
		                      MOWGLI_FUNC_NAME, entity(mu)->name);

		// Verification succeeded and re-encrypting password failed
		return true;
	}

	(void) smemzero(mu->pass, sizeof mu->pass);
	(void) mowgli_strlcpy(mu->pass, new_hash, sizeof mu->pass);
	(void) hook_call_myuser_changed_password_or_hash(mu);

	// Verification succeeded and user's password re-encrypted
	return true;
}
