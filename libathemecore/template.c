/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2014 Atheme Project (http://atheme.org/)
 * Copyright (C) 2018 Atheme Development Group (https://atheme.github.io/)
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * atheme-services: A collection of minimalist IRC services
 * template.c: Functions to work with predefined flags collections
 */

#include <atheme.h>
#include "internal.h"

mowgli_patricia_t *global_template_dict = NULL;

void
fix_global_template_flags(void)
{
	struct default_template *def_t;
	mowgli_patricia_iteration_state_t state;

	MOWGLI_PATRICIA_FOREACH(def_t, &state, global_template_dict)
	{
		def_t->flags &= ca_all;
	}
}

void
set_global_template_flags(const char *name, unsigned int flags)
{
	if (global_template_dict == NULL)
		global_template_dict = mowgli_patricia_create(strcasecanon);

	struct default_template *def_t = mowgli_patricia_retrieve(global_template_dict, name);

	if (def_t != NULL)
	{
		def_t->flags = flags;
		return;
	}

	def_t = smalloc(sizeof *def_t);
	def_t->flags = flags;
	mowgli_patricia_add(global_template_dict, name, def_t);

	slog(LG_DEBUG, "set_global_template_flags(): add %s", name);
}

unsigned int
get_global_template_flags(const char *name)
{
	struct default_template *def_t;

	if (global_template_dict == NULL)
		global_template_dict = mowgli_patricia_create(strcasecanon);

	def_t = mowgli_patricia_retrieve(global_template_dict, name);
	if (def_t == NULL)
		return 0;

	return def_t->flags;
}

static void
release_global_template_data(const char *key, void *data, void *privdata)
{
	slog(LG_DEBUG, "release_global_template_data(): delete %s", key);

	sfree(data);
}

void
clear_global_template_flags(void)
{
	if (global_template_dict == NULL)
		return;

	mowgli_patricia_destroy(global_template_dict, release_global_template_data, NULL);
	global_template_dict = NULL;
}

/* name1=value1 name2=value2 name3=value3... */
const char *
getitem(const char *str, const char *name)
{
	char *p;
	static char result[300];
	int l;

	l = strlen(name);
	for (;;)
	{
		p = strchr(str, '=');
		if (p == NULL)
			return NULL;
		if (p - str == l && !strncasecmp(str, name, p - str))
		{
			mowgli_strlcpy(result, p, sizeof result);
			p = strchr(result, ' ');
			if (p != NULL)
				*p = '\0';
			return result;
		}
		str = strchr(p, ' ');
		if (str == NULL)
			return NULL;
		while (*str == ' ')
			str++;
	}
}

unsigned int
get_template_flags(struct mychan *mc, const char *name)
{
	struct metadata *md;
	const char *d;

	if (mc != NULL)
	{
		md = metadata_find(mc, "private:templates");
		if (md != NULL)
		{
			d = getitem(md->value, name);
			if (d != NULL)
				return flags_to_bitmask(d, 0);
		}
	}

	return get_global_template_flags(name);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
