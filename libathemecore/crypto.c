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
bool crypto_module_loaded = false;

const crypt_impl_t *
crypt_get_default_provider(void)
{
	if (!MOWGLI_LIST_LENGTH(&crypt_impl_list))
		return NULL;

	if (!crypt_impl_list.head)
		return NULL;

	return crypt_impl_list.head->data;
}

const char *
gen_salt(void)
{
	const crypt_impl_t *const ci = crypt_get_default_provider();

	if (!ci)
		return NULL;

	return ci->salt();
}

const char *
crypt_string(const char *const restrict password, const char *restrict salt)
{
	const crypt_impl_t *const ci = crypt_get_default_provider();

	if (!ci)
		return NULL;

	if (!salt || !*salt)
		salt = ci->salt();

	return ci->crypt(password, salt);
}

static void
crypt_log_modchg(const char *const restrict caller, const char *const restrict which,
                 const crypt_impl_t *const restrict impl)
{
	const unsigned int level = (runflags & RF_STARTING) ? LG_DEBUG : LG_INFO;
	const crypt_impl_t *const ci = crypt_get_default_provider();

	(void) slog(level, "%s: %s crypto provider '%s'", caller, which, impl->id);

	if (ci == impl)
		(void) slog(level, "%s: default crypto provider is now '%s'", caller, ci->id);
	else if (ci != NULL)
		(void) slog(level, "%s: default crypto provider is still '%s'", caller, ci->id);
	else
		(void) slog(LG_ERROR, "%s: no crypto provider is available!", caller);
}

void
crypt_register(crypt_impl_t *impl)
{
	return_if_fail(impl != NULL);

	mowgli_node_add(impl, &impl->node, &crypt_impl_list);

	crypto_module_loaded = MOWGLI_LIST_LENGTH(&crypt_impl_list) > 0 ? true : false;

	(void) crypt_log_modchg(__func__, "registered", impl);
}

void
crypt_unregister(crypt_impl_t *impl)
{
	return_if_fail(impl != NULL);

	mowgli_node_delete(&impl->node, &crypt_impl_list);

	crypto_module_loaded = MOWGLI_LIST_LENGTH(&crypt_impl_list) > 0 ? true : false;

	(void) crypt_log_modchg(__func__, "unregistered", impl);
}

const crypt_impl_t *
crypt_verify_password(const char *const restrict password, const char *const restrict crypt_str)
{
	mowgli_node_t *n;

	MOWGLI_ITER_FOREACH(n, crypt_impl_list.head)
	{
		const crypt_impl_t *const ci = n->data;
		const char *const result = ci->crypt(password, crypt_str);

		if (result != NULL && !strcmp(result, crypt_str))
			return ci;
	}

	return NULL;
}
