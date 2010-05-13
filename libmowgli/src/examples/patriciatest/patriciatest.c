/*
 * libmowgli: A collection of useful routines for programming.
 * patriciatest.c: Testing of the patricia tree.
 *
 * Copyright (c) 2008 Jilles Tjoelker
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

void str_canon(char *key)
{
	return;
}

void statscb(const char *line, void *data)
{
	printf("%s\n", line);
}

/* assumes data is key */
static void check_all_retrievable(mowgli_patricia_t *dtree)
{
	mowgli_patricia_iteration_state_t state;
	void *elem, *elem2;

	printf("Checking consistency...");
	fflush(stdout);
	MOWGLI_PATRICIA_FOREACH(elem, &state, dtree)
	{
		elem2 = mowgli_patricia_retrieve(dtree, (const char *)elem);
		if (elem2 == NULL)
			printf("failed to find element %s\n", elem);
		else if (strcmp(elem2, elem))
			printf("element %s != %s\n", elem, elem2);
		else
			printf(".");
		fflush(stdout);
	}
	printf("\n");
}

void test_patricia(void)
{
	mowgli_patricia_t *dtree;
	mowgli_patricia_iteration_state_t state;
	void *elem;

	dtree = mowgli_patricia_create(str_canon);
#define ADD(x) mowgli_patricia_add(dtree, x, x); check_all_retrievable(dtree)
	ADD("alias");
	ADD("foo");
	ADD("bar");
	ADD("baz");
	ADD("splork");
	ADD("rabbit");
	ADD("meow");
	ADD("hi");
	ADD("konnichiwa");
	ADD("absolutely");
	ADD("cat");
	ADD("dog");
	ADD("woof");
	ADD("moon");
	ADD("new");
	ADD("delete");

	printf("size: %u\n", mowgli_patricia_size(dtree));

	MOWGLI_PATRICIA_FOREACH(elem, &state, dtree)
	{
		printf("element -> %s\n", (const char *)elem);
	}
	printf("End of elements\n");
	mowgli_patricia_stats(dtree, statscb, NULL);

	check_all_retrievable(dtree);

#define TESTRETRIEVE(x) elem = mowgli_patricia_retrieve(dtree, x); printf("element %s: %s\n", x, elem ? "YES" : "NO")
	TESTRETRIEVE("meows");
	TESTRETRIEVE("meo");
	TESTRETRIEVE("deletes");
	TESTRETRIEVE("z");
	TESTRETRIEVE("0");

#define TESTDELETE(x) mowgli_patricia_delete(dtree, x); elem = mowgli_patricia_retrieve(dtree, x); printf("deleting %s: %s\n", x, elem ? "STILL PRESENT" : "GONE"); check_all_retrievable(dtree)
	TESTDELETE("YYY");
	TESTDELETE("foo");
	TESTDELETE("splork");
	ADD("spork");
	ADD("foo");
	TESTDELETE("absolutely");
	TESTDELETE("cat");
	ADD("absolutely");
	TESTDELETE("dog");

	mowgli_patricia_destroy(dtree, NULL, NULL);
}

int main(int argc, char *argv[])
{
	mowgli_init();

	test_patricia();
}
