/*
 * libmowgli: A collection of useful routines for programming.
 * listsort.c: Testing of the list sorting routine.
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

int str_comparator(mowgli_node_t *n, mowgli_node_t *n2, void *opaque)
{
	int ret; 
	ret = strcasecmp(n->data, n2->data);

	return ret;
}

void test_strings(void)
{
	mowgli_list_t l = { NULL, NULL, 0 };
	mowgli_node_t *n, *tn;

	mowgli_node_add("foo", mowgli_node_create(), &l);
	mowgli_node_add("bar", mowgli_node_create(), &l);
	mowgli_node_add("baz", mowgli_node_create(), &l);
	mowgli_node_add("splork", mowgli_node_create(), &l);
	mowgli_node_add("rabbit", mowgli_node_create(), &l);
	mowgli_node_add("meow", mowgli_node_create(), &l);
	mowgli_node_add("hi", mowgli_node_create(), &l);
	mowgli_node_add("konnichiwa", mowgli_node_create(), &l);
	mowgli_node_add("absolutely", mowgli_node_create(), &l);
	mowgli_node_add("cat", mowgli_node_create(), &l);
	mowgli_node_add("dog", mowgli_node_create(), &l);
	mowgli_node_add("woof", mowgli_node_create(), &l);
	mowgli_node_add("moon", mowgli_node_create(), &l);
	mowgli_node_add("new", mowgli_node_create(), &l);
	mowgli_node_add("delete", mowgli_node_create(), &l);
	mowgli_node_add("alias", mowgli_node_create(), &l);

	mowgli_list_sort(&l, str_comparator, NULL);

	printf("\nString test results\n");

	MOWGLI_LIST_FOREACH_SAFE(n, tn, l.head)
	{
		printf("  %s\n", (char*) n->data);
		mowgli_node_delete(n, &l);
	}
}

int int_comparator(mowgli_node_t *n, mowgli_node_t *n2, void *opaque)
{
	long a = (long) n->data;
	long b = (long) n2->data;

	return a - b;
}

void test_integers(void)
{
	mowgli_list_t l = { NULL, NULL, 0 };
	mowgli_node_t *n, *tn;
	
	mowgli_node_add((void *) 3, mowgli_node_create(), &l);
	mowgli_node_add((void *) 2, mowgli_node_create(), &l);
	mowgli_node_add((void *) 4, mowgli_node_create(), &l);
	mowgli_node_add((void *) 1, mowgli_node_create(), &l);

	mowgli_list_sort(&l, int_comparator, NULL);

	printf("\nInteger test results\n");

	MOWGLI_LIST_FOREACH_SAFE(n, tn, l.head)
	{
		printf("  %ld\n", (long) n->data);
		mowgli_node_delete(n, &l);
	}
}

int main(int argc, char *argv[])
{
	mowgli_init();

	test_strings();
	test_integers();
    return EXIT_SUCCESS;
}
