/*
 * libmowgli: A collection of useful routines for programming.
 * dicttest.c: Testing of the mowgli.dictionary engine.
 *
 * Copyright (c) 2007 William Pitcock <nenolod -at- sacredspiral.co.uk>
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

#include <mowgli.h>

int main(int argc, const char *argv[])
{
	mowgli_dictionary_t *test_dict;
	mowgli_random_t *r;
	char key[10];
	long ans[100], i;
	int pass = 0, fail = 0;

	mowgli_init();

	test_dict = mowgli_dictionary_create(strcasecmp);
	r = mowgli_random_create();

	for (i = 0; i < 100; i++)
	{
		ans[i] = mowgli_random_int(r);
		snprintf(key, 10, "%ldkey%ld", i, i);
		mowgli_dictionary_add(test_dict, key, (void *) ans[i]);
	}

	for (i = 0; i < 100; i++)
	{
		snprintf(key, 10, "%ldkey%ld", i, i);

		if ( (long) mowgli_dictionary_retrieve(test_dict, key) != ans[i])
		{
			printf("FAIL %ld %p[%p]\n", i, mowgli_dictionary_retrieve(test_dict, key), (void*) ans[i]);
			fail++;
		}
		else
		{
			printf("PASS %ld %p[%p]\n", i, mowgli_dictionary_retrieve(test_dict, key), (void*) ans[i]);
			pass++;
		}
	}

	printf("%d tests failed, %d tests passed.\n", fail, pass);
	return 0;
}
