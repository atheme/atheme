/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2008-2012 Atheme Project (http://atheme.org/)
 * Copyright (C) 2018 Atheme Development Group (https://atheme.github.io/)
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * atheme-services: A collection of minimalist IRC services
 * strshare.c: Shared strings.
 */

#include <atheme.h>
#include "internal.h"

static mowgli_patricia_t *strshare_dict = NULL;

struct strshare
{
	int refcount;
};

void
strshare_init(void)
{
	strshare_dict = mowgli_patricia_create(noopcanon);
}

stringref
strshare_get(const char *str)
{
	struct strshare *ss;

	if (str == NULL)
		return NULL;

	ss = mowgli_patricia_retrieve(strshare_dict, str);
	if (ss != NULL)
		ss->refcount++;
	else
	{
		ss = smalloc((sizeof *ss) + strlen(str) + 1);
		ss->refcount = 1;
		strcpy((char *)(ss + 1), str);
		mowgli_patricia_add(strshare_dict, (char *)(ss + 1), ss);
	}

	return (char *)(ss + 1);
}

stringref
strshare_ref(stringref str)
{
	struct strshare *ss;

	if (str == NULL)
		return NULL;

	/* intermediate cast to suppress gcc -Wcast-qual */
	ss = (struct strshare *)(uintptr_t)str - 1;
	ss->refcount++;

	return str;
}

void
strshare_unref(stringref str)
{
	struct strshare *ss;

	if (str == NULL)
		return;

	/* intermediate cast to suppress gcc -Wcast-qual */
	ss = (struct strshare *)(uintptr_t)str - 1;
	ss->refcount--;
	if (ss->refcount == 0)
	{
		mowgli_patricia_delete(strshare_dict, str);
		sfree(ss);
	}
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
