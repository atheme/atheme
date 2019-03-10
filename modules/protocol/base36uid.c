/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2011 Atheme Project (http://atheme.org/)
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * atheme-services: A collection of minimalist IRC services
 * base36uid.c: UID management.
 */

#include <atheme.h>

static char new_uid[10]; // allow for \0
static unsigned int uindex = 0;

static void
add_one_to_uid(unsigned int i)
{
	if (i != strlen(me.numeric))	// Not reached server SID portion yet?
	{
		if (new_uid[i] == 'Z')
			new_uid[i] = '0';
		else if (new_uid[i] == '9')
		{
			new_uid[i] = 'A';
			add_one_to_uid(i - 1);
		}
		else
			new_uid[i] = new_uid[i] + 1;
	}
	else
	{
		if (new_uid[i] == 'Z')
			for (i = strlen(me.numeric); i < 9; i++)
				new_uid[i] = 'A';
		else
			new_uid[i] = new_uid[i] + 1;
	}
}

static void
base36_uid_init(const char *sid)
{
	unsigned int i;
	char buf[BUFSIZE];

	if (ircd->uses_p10)
	{
		me.numeric = sstrdup(uinttobase64(buf, (uint64_t) atoi(me.numeric), 2));
		uindex = 5;
	}
	else
		uindex = 9;


	memset(new_uid, 0, sizeof(new_uid));

	if (me.numeric != NULL)
		memcpy(new_uid, me.numeric, strlen(me.numeric));
	else
		return;

	for (i = 0; i < strlen(me.numeric); i++)
		if (new_uid[i] == '\0')
			new_uid[i] = 'A';

	for (i = strlen(me.numeric); i < uindex; i++)
		new_uid[i] = 'A';
}

static const char *
base36_uid_get(void)
{
	add_one_to_uid(uindex - 1); // index from 0
	return (new_uid);
}

static void
mod_init(struct module ATHEME_VATTR_UNUSED *const restrict m)
{
	static const struct uid_provider base36_gen = {
		.uid_init       = &base36_uid_init,
		.uid_get        = &base36_uid_get,
	};

	uid_provider_impl = &base36_gen;
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{

}

SIMPLE_DECLARE_MODULE_V1("protocol/base36uid", MODULE_UNLOAD_CAPABILITY_NEVER)
