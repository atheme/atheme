/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2011 William Pitcock <nenolod@dereferenced.org>
 */

#include <atheme.h>
#include "exttarget.h"

// Imported by other modules/exttarget/*.so */
extern mowgli_patricia_t *exttarget_tree;
mowgli_patricia_t *exttarget_tree = NULL;

static void
exttarget_find(struct hook_myentity_req *req)
{
	char buf[BUFSIZE];
	char *i, *j;
	entity_validate_f val;

	return_if_fail(req != NULL);

	if (req->name == NULL || *req->name != '$')
		return;

	mowgli_strlcpy(buf, req->name, BUFSIZE);

	i = (buf + 1);
	if ((j = strchr(buf, ':')) != NULL)
		*j++ = '\0';

	// i is now the name of the exttarget.  j is the parameter.
	val = mowgli_patricia_retrieve(exttarget_tree, i);
	if (val != NULL)
		req->entity = val(j);
}

static void
mod_init(struct module ATHEME_VATTR_UNUSED *const restrict m)
{
	exttarget_tree = mowgli_patricia_create(strcasecanon);

	hook_add_myentity_find(exttarget_find);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	hook_del_myentity_find(exttarget_find);

	mowgli_patricia_destroy(exttarget_tree, NULL, NULL);
}

SIMPLE_DECLARE_MODULE_V1("exttarget/main", MODULE_UNLOAD_CAPABILITY_OK)
