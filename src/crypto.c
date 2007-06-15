/*
 * atheme-services: A collection of minimalist IRC services   
 * crypto.c: Cryptographic module support.
 *
 * Copyright (c) 2005-2007 Atheme Project (http://www.atheme.org)           
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

static char saltbuf[BUFSIZE];
boolean_t crypto_module_loaded = FALSE;

/*
 * crypt_string is just like crypt(3) under UNIX
 * systems. Modules provide this function, otherwise
 * it returns the string unencrypted.
 */
char *(*crypt_string) (char *str, char *salt) = &generic_crypt_string;

char *generic_crypt_string(char *str, char *salt)
{
	return str;
}

/*
 * crypt_verify_password is a frontend to crypt_string().
 */
boolean_t crypt_verify_password(char *uinput, char *pass)
{
	char *cstr = crypt_string(uinput, pass);

	if (!strcmp(cstr, pass))
		return TRUE;

	return FALSE;
}

char *gen_salt(void)
{
	char *ht = gen_pw(6);

	strlcpy(saltbuf, "$1$", BUFSIZE);
	strlcat(saltbuf, ht, BUFSIZE);
	strlcat(saltbuf, "$", BUFSIZE);

	free(ht);

	return saltbuf;
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
