/*
 * libmowgli: A collection of useful routines for programming.
 * patriciatest2.c: More testing of the patricia tree.
 *
 * Copyright (c) 2008-2010 Jilles Tjoelker
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

#define TESTSIZE 10000

int errors = 0;

void str_canon(char *key)
{
	return;
}

void statscb(const char *line, void *data)
{
}

/* assumes data is key */
static void check_all_retrievable(mowgli_patricia_t *dtree)
{
	mowgli_patricia_iteration_state_t state;
	void *elem, *elem2;
	unsigned int n1 = 0, n2;

	mowgli_patricia_stats(dtree, statscb, NULL);
	n2 = mowgli_patricia_size(dtree);
	MOWGLI_PATRICIA_FOREACH(elem, &state, dtree)
	{
		elem2 = mowgli_patricia_retrieve(dtree, (const char *)elem);
		if (elem2 == NULL)
		{
			errors++;
			printf("failed to find element %s\n",
					(const char *)elem);
		}
		else if (strcmp(elem2, elem))
		{
			printf("element %s != %s\n",
					(const char *)elem,
					(const char *)elem2);
			errors++;
		}
		n1++;
		if (n1 > n2 * 2)
			break;
	}
	if (n1 != n2)
	{
		errors++;
		printf("number of iterated elements %u != size %u\n", n1, n2);
	}
}

void test_patricia(void)
{
	mowgli_patricia_t *dtree;
	int i, j;
	char buf[100], *strings[TESTSIZE];

	srandom(12346);
	for (i = 0; i < TESTSIZE; i++)
	{
		for (j = 0; j < 40; j++)
			buf[j] = 'a' + random() % 26;
		buf[20 + random() % 20] = '\0';
		strings[i] = strdup(buf);
	}

	dtree = mowgli_patricia_create(str_canon);

	for (i = 0; i < TESTSIZE; i++)
	{
		mowgli_patricia_add(dtree, strings[i], strings[i]);
		check_all_retrievable(dtree);
	}

	check_all_retrievable(dtree);

	for (i = 0; i < TESTSIZE / 2; i++)
	{
		mowgli_patricia_delete(dtree, strings[i]);
		if (mowgli_patricia_retrieve(dtree, strings[i]))
		{
			printf("still retrievable after delete: %s\n",
					strings[i]);
			errors++;
		}
		check_all_retrievable(dtree);
	}

	for (i = 0; i < TESTSIZE / 2; i++)
	{
		mowgli_patricia_add(dtree, strings[i], strings[i]);
		check_all_retrievable(dtree);
	}

	for (i = 0; i < TESTSIZE; i++)
	{
		mowgli_patricia_delete(dtree, strings[i]);
		if (mowgli_patricia_retrieve(dtree, strings[i]))
		{
			printf("still retrievable after delete: %s\n",
					strings[i]);
			errors++;
		}
		check_all_retrievable(dtree);
	}

	mowgli_patricia_destroy(dtree, NULL, NULL);

	for (i = 0; i < TESTSIZE; i++)
		free(strings[i]);
}

int main(int argc, char *argv[])
{
	mowgli_init();

	test_patricia();

	return errors == 0 ? 0 : 1;
}
