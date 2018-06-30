/*
 * atheme-services: A collection of minimalist IRC services
 * crypto.c: Cryptographic module support.
 *
 * Copyright (c) 2012 William Pitcock <nenolod@dereferenced.org>.
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
	if (! impl || ! impl->id || ! (impl->crypt || impl->verify))
	{
		(void) slog(LG_ERROR, "%s: invalid parameters (BUG)", __func__);
		return;
	}

	mowgli_node_t *const n = mowgli_node_create();

	if (! n)
	{
		(void) slog(LG_ERROR, "%s: mowgli_node_create: memory allocation failure", __func__);
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
	(void) crypt_log_modchg(__func__, "registered", impl);
}

void
crypt_unregister(const struct crypt_impl *const restrict impl)
{
	if (! impl || ! impl->id || ! (impl->crypt || impl->verify))
	{
		(void) slog(LG_ERROR, "%s: invalid parameters (BUG)", __func__);
		return;
	}

	mowgli_node_t *n, *tn;

	MOWGLI_ITER_FOREACH_SAFE(n, tn, crypt_impl_list.head)
	{
		if (n->data == impl)
		{
			(void) mowgli_node_delete(n, &crypt_impl_list);
			(void) mowgli_node_free(n);

			(void) crypt_log_modchg(__func__, "unregistered", impl);
			return;
		}
	}

	(void) slog(LG_ERROR, "%s: could not find provider '%s' to unregister", __func__, impl->id);
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
			(void) slog(LG_DEBUG, "%s: skipping incapable provider '%s'", __func__, ci->id);
			continue;
		}

		encryption_capable_module = true;

		const char *const result = ci->crypt(password, NULL);

		if (! result)
		{
			(void) slog(LG_ERROR, "%s: ci->crypt() failed for provider '%s'", __func__, ci->id);
			continue;
		}

		(void) slog(LG_DEBUG, "%s: encrypted password with provider '%s'", __func__, ci->id);
		return result;
	}

	if (encryption_capable_module)
		(void) slog(LG_ERROR, "%s: all encryption-capable crypto providers failed", __func__);
	else
		(void) slog(LG_ERROR, "%s: no encryption-capable crypto provider is available!", __func__);

	return NULL;
}
