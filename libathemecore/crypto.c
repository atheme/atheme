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

const crypt_impl_t *
crypt_get_default_provider(void)
{
	mowgli_node_t *n;

	if (!MOWGLI_LIST_LENGTH(&crypt_impl_list))
		return NULL;

	MOWGLI_ITER_FOREACH(n, crypt_impl_list.head)
	{
		const crypt_impl_t *const ci = n->data;

		if (ci->salt != NULL && ci->crypt != NULL)
			return ci;
	}

	return NULL;
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
crypt_string(const char *const restrict password, const char *restrict parameters)
{
	const crypt_impl_t *const ci = crypt_get_default_provider();

	if (!ci)
		return NULL;

	if (!parameters || !*parameters)
		parameters = ci->salt();

	if (!parameters)
		return NULL;

	return ci->crypt(password, parameters);
}

static void
crypt_log_modchg(const char *const restrict caller, const char *const restrict which,
                 const crypt_impl_t *const restrict impl)
{
	const unsigned int level = (runflags & RF_STARTING) ? LG_DEBUG : LG_INFO;
	const crypt_impl_t *const ci = crypt_get_default_provider();

	(void) slog(level, "%s: %s crypto provider '%s'", caller, which, impl->id);

	if (ci)
		(void) slog(level, "%s: default crypto provider is (now) '%s'", caller, ci->id);
	else
		(void) slog(LG_ERROR, "%s: no crypto provider is available!", caller);
}

void
crypt_register(crypt_impl_t *const restrict impl)
{
	return_if_fail(impl != NULL);
	return_if_fail(impl->id != NULL);

	if ((impl->salt != NULL && impl->crypt != NULL) || impl->verify != NULL)
	{
		mowgli_node_add(impl, &impl->node, &crypt_impl_list);
		(void) crypt_log_modchg(__func__, "registered", impl);
	}
	else
		(void) slog(LG_ERROR, "%s: crypto provider '%s' provides neither salt/crypt nor verify methods",
		                      __func__, impl->id);
}

void
crypt_unregister(crypt_impl_t *const restrict impl)
{
	return_if_fail(impl != NULL);

	if ((impl->salt != NULL && impl->crypt != NULL) || impl->verify != NULL)
	{
		mowgli_node_delete(&impl->node, &crypt_impl_list);
		(void) crypt_log_modchg(__func__, "unregistered", impl);
	}
}

const crypt_impl_t *
crypt_verify_password(const char *const restrict password, const char *const restrict parameters)
{
	mowgli_node_t *n;

	if (!MOWGLI_LIST_LENGTH(&crypt_impl_list))
		return NULL;

	MOWGLI_ITER_FOREACH(n, crypt_impl_list.head)
	{
		const crypt_impl_t *const ci = n->data;

		if (ci->verify != NULL)
		{
			if (ci->verify(password, parameters))
				return ci;
		}
		else if (ci->crypt != NULL)
		{
			const char *const result = ci->crypt(password, parameters);

			if (result != NULL && strcmp(result, parameters) == 0)
				return ci;
		}
	}

	return NULL;
}
