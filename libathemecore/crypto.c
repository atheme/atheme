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

static const char *generic_crypt_string(const char *str, const char *salt)
{
	return str;
}

static const char *generic_gen_salt(void)
{
	static char buf[BUFSIZE];
	char *ht = random_string(6);

	mowgli_strlcpy(buf, "$1$", BUFSIZE);
	mowgli_strlcat(buf, ht, BUFSIZE);
	mowgli_strlcat(buf, "$", BUFSIZE);

	free(ht);

	return buf;
}

static const crypt_impl_t fallback_crypt_impl = {
	.id = "plaintext",
	.crypt = &generic_crypt_string,
	.salt = &generic_gen_salt,
};

const crypt_impl_t *crypt_get_default_provider(void)
{
	crypt_impl_t *ci;

	if (!MOWGLI_LIST_LENGTH(&crypt_impl_list))
		return &fallback_crypt_impl;

	/* top of stack should handle string crypting, should be populated by now */
	return_val_if_fail(crypt_impl_list.head != NULL, &fallback_crypt_impl);
	ci = crypt_impl_list.head->data;

	/* ensure the provider is populated */
	return_val_if_fail(ci->crypt != NULL, &fallback_crypt_impl);
	return_val_if_fail(ci->salt != NULL, &fallback_crypt_impl);

	return ci;
}

/*
 * crypt_string is just like crypt(3) under UNIX
 * systems. Modules provide this function, otherwise
 * it returns the string unencrypted.
 */
const char *crypt_string(const char *key, const char *salt)
{
	const crypt_impl_t *ci = crypt_get_default_provider();

	return ci->crypt(key, salt);
}

const char *gen_salt(void)
{
	const crypt_impl_t *ci = crypt_get_default_provider();

	return ci->salt();
}

void crypt_register(crypt_impl_t *impl)
{
	return_if_fail(impl != NULL);

	if (impl->crypt == NULL)
		impl->crypt = &generic_crypt_string;
	if (impl->salt == NULL)
		impl->salt = &generic_gen_salt;

	mowgli_node_add(impl, &impl->node, &crypt_impl_list);

	crypto_module_loaded = MOWGLI_LIST_LENGTH(&crypt_impl_list) > 0 ? true : false;
}

void crypt_unregister(crypt_impl_t *impl)
{
	return_if_fail(impl != NULL);

	mowgli_node_delete(&impl->node, &crypt_impl_list);

	crypto_module_loaded = MOWGLI_LIST_LENGTH(&crypt_impl_list) > 0 ? true : false;
}

/*
 * crypt_verify_password is a frontend to crypt_string().
 */
const crypt_impl_t *crypt_verify_password(const char *uinput, const char *pass)
{
	mowgli_node_t *n;
	const char *cstr;

	MOWGLI_ITER_FOREACH(n, crypt_impl_list.head)
	{
		crypt_impl_t *ci;

		ci = n->data;
		cstr = ci->crypt(uinput, pass);

		if (!strcmp(cstr, pass))
			return ci;
	}

	cstr = fallback_crypt_impl.crypt(uinput, pass);

	if (!strcmp(cstr, pass))
		return &fallback_crypt_impl;

	return NULL;
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
