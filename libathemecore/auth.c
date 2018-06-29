/*
 * atheme-services: A collection of minimalist IRC services
 * auth.c: Authentication.
 *
 * Copyright (c) 2005-2009 Atheme Project (http://www.atheme.org)
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "atheme.h"

bool auth_module_loaded = false;
bool (*auth_user_custom)(struct myuser *mu, const char *password);

void
set_password(struct myuser *const restrict mu, const char *const restrict password)
{
	if (! mu || ! password)
		return;

	const char *const hash = crypt_password(password);

	if (hash)
	{
		mu->flags |= MU_CRYPTPASS;

		(void) mowgli_strlcpy(mu->pass, hash, sizeof mu->pass);
	}
	else
	{
		mu->flags &= ~MU_CRYPTPASS;

		(void) mowgli_strlcpy(mu->pass, password, sizeof mu->pass);
		(void) slog(LG_ERROR, "%s: failed to encrypt password for account '%s'", __func__, entity(mu)->name);
	}
}

bool
verify_password(struct myuser *const restrict mu, const char *const restrict password)
{
	if (! mu || ! password)
		return false;

	if (auth_module_loaded && auth_user_custom)
		return auth_user_custom(mu, password);

	if (! (mu->flags & MU_CRYPTPASS))
	{
		(void) slog(LG_INFO, _("%s: verifying unencrypted password for account '%s'!"), __func__,
		                     entity(mu)->name);

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
		(void) slog(LG_INFO, _("%s: transitioning from crypt scheme '%s' to '%s' for account '%s'"),
		                     __func__, ci->id, ci_default->id, entity(mu)->name);
	else if (verify_flags & PWVERIFY_FLAG_RECRYPT)
		(void) slog(LG_INFO, _("%s: re-encrypting password for account '%s'"), __func__, entity(mu)->name);
	else
		// Verification succeeded and re-encrypting not required, nothing more to do
		return true;

	if (! (new_hash = ci_default->crypt(password, NULL)))
		(void) slog(LG_ERROR, _("%s: hash generation failed"), __func__);
	else
		(void) mowgli_strlcpy(mu->pass, new_hash, sizeof mu->pass);

	// Verification succeeded and user's password (possibly) re-encrypted
	return true;
}
