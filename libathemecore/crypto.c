/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2012-2015 William Pitcock <nenolod@dereferenced.org>
 * Copyright (C) 2015-2018 Aaron M. D. Jones <me@aaronmdjones.net>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * atheme-services: A collection of minimalist IRC services
 * crypto.c: Cryptographic module support.
 */

#include <atheme.h>
#include "internal.h"

static mowgli_list_t crypt_impl_list = { NULL, NULL, 0 };

static inline void
crypt_log_modchg(const char *const restrict caller, const char *const restrict which,
                 const struct crypt_impl *const restrict impl)
{
	const unsigned int level = (runflags & RF_STARTING) ? LG_DEBUG : LG_INFO;
	const struct crypt_impl *const ci = crypt_get_default_provider();

	(void) slog(level, "%s: %s crypto provider '%s'", caller, which, impl->id);

	if (ci)
		(void) slog(level, "%s: default crypto provider is (now) '%s'", caller, ci->id);
	else
		(void) slog(LG_ERROR, "%s: no encryption-capable crypto provider is available!", caller);
}

void
crypt_register(const struct crypt_impl *const restrict impl)
{
	if (! impl || ! impl->id || ! *impl->id || ! (impl->crypt || impl->verify))
	{
		(void) slog(LG_ERROR, "%s: invalid parameters (BUG)", MOWGLI_FUNC_NAME);
		return;
	}

	mowgli_node_t *const n = mowgli_node_create();

	if (! n)
	{
		(void) slog(LG_ERROR, "%s: mowgli_node_create: memory allocation failure", MOWGLI_FUNC_NAME);
		return;
	}

	/* Here we cast it to (void *) because mowgli_node_add() expects that; it cannot be made const because then
	 * it would have to return a (const void *) too which would cause multiple warnings any time it is actually
	 * storing, and thus gets assigned to, a pointer to a mutable object.
	 *
	 * To avoid the cast generating a diagnostic due to dropping a const qualifier, we first cast to uintptr_t.
	 * This is not unprecedented in this codebase; libathemecore/strshare.c does the same thing.
	 */
	(void) mowgli_node_add((void *) ((uintptr_t) impl), n, &crypt_impl_list);
	(void) crypt_log_modchg(MOWGLI_FUNC_NAME, "registered", impl);
}

void
crypt_unregister(const struct crypt_impl *const restrict impl)
{
	if (! impl || ! impl->id || ! *impl->id || ! (impl->crypt || impl->verify))
	{
		(void) slog(LG_ERROR, "%s: invalid parameters (BUG)", MOWGLI_FUNC_NAME);
		return;
	}

	mowgli_node_t *n, *tn;

	MOWGLI_ITER_FOREACH_SAFE(n, tn, crypt_impl_list.head)
	{
		if (n->data == impl)
		{
			(void) mowgli_node_delete(n, &crypt_impl_list);
			(void) mowgli_node_free(n);

			(void) crypt_log_modchg(MOWGLI_FUNC_NAME, "unregistered", impl);
			return;
		}
	}

	(void) slog(LG_ERROR, "%s: could not find provider '%s' to unregister", MOWGLI_FUNC_NAME, impl->id);
}

const struct crypt_impl *
crypt_get_default_provider(void)
{
	mowgli_node_t *n;

	MOWGLI_ITER_FOREACH(n, crypt_impl_list.head)
	{
		const struct crypt_impl *const ci = n->data;

		if (ci->crypt)
			return ci;
	}

	return NULL;
}

const struct crypt_impl *
crypt_get_named_provider(const char *const restrict id)
{
	mowgli_node_t *n;

	MOWGLI_ITER_FOREACH(n, crypt_impl_list.head)
	{
		const struct crypt_impl *const ci = n->data;

		if (strcasecmp(ci->id, id) == 0)
			return ci;
	}

	return NULL;
}

const struct crypt_impl * ATHEME_FATTR_WUR
crypt_verify_password(const char *const restrict password, const char *const restrict parameters,
                      unsigned int *const restrict flags)
{
	mowgli_node_t *n;

	if (flags)
		*flags = PWVERIFY_FLAG_NONE;

	MOWGLI_ITER_FOREACH(n, crypt_impl_list.head)
	{
		const struct crypt_impl *const ci = n->data;

		if (ci->verify)
		{
			unsigned int myflags = PWVERIFY_FLAG_NONE;

			if (ci->verify(password, parameters, &myflags))
			{
				if (flags)
					*flags = myflags;

				return ci;
			}

			/* If password verification failed and the password hash was produced
			 * by the module we just tried, there's no point continuing to test it
			 * against the other modules. This saves some CPU time.
			 */
			if (myflags & PWVERIFY_FLAG_MYMODULE)
				return NULL;

			continue;
		}

		if (ci->crypt)
		{
			const char *const result = ci->crypt(password, parameters);

			if (result && strcmp(result, parameters) == 0)
				return ci;

			continue;
		}
	}

	return NULL;
}

const char *
crypt_password(const char *const restrict password)
{
	bool encryption_capable_module = false;

	mowgli_node_t *n;

	MOWGLI_ITER_FOREACH(n, crypt_impl_list.head)
	{
		const struct crypt_impl *const ci = n->data;

		if (! ci->crypt)
		{
			(void) slog(LG_DEBUG, "%s: skipping incapable provider '%s'", MOWGLI_FUNC_NAME, ci->id);
			continue;
		}

		encryption_capable_module = true;

		const char *const result = ci->crypt(password, NULL);

		if (! result)
		{
			(void) slog(LG_ERROR, "%s: ci->crypt() failed for provider '%s'", MOWGLI_FUNC_NAME, ci->id);
			continue;
		}

		(void) slog(LG_DEBUG, "%s: encrypted password with provider '%s'", MOWGLI_FUNC_NAME, ci->id);
		return result;
	}

	if (encryption_capable_module)
		(void) slog(LG_ERROR, "%s: all encryption-capable crypto providers failed", MOWGLI_FUNC_NAME);
	else
		(void) slog(LG_ERROR, "%s: no encryption-capable crypto provider is available!", MOWGLI_FUNC_NAME);

	return NULL;
}
