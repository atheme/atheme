/*
 * Copyright (c) 2011 William Pitcock <nenolod@dereferenced.org>
 *
 * Rights to this code are as documented in doc/LICENSE.
 */

#include "atheme.h"
#include "exttarget.h"

DECLARE_MODULE_V1
(
	"exttarget/main", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

mowgli_patricia_t *exttarget_tree = NULL;

static void exttarget_find(hook_myentity_req_t *req)
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

	/* i is now the name of the exttarget.  j is the parameter. */
	val = mowgli_patricia_retrieve(exttarget_tree, i);
	if (val != NULL)
		req->entity = val(j);
}

void _modinit(module_t *m)
{
	exttarget_tree = mowgli_patricia_create(strcasecanon);

	hook_add_event("myentity_find");
	hook_add_myentity_find(exttarget_find);
}

void _moddeinit(module_unload_intent_t intent)
{
	hook_del_myentity_find(exttarget_find);

	mowgli_patricia_destroy(exttarget_tree, NULL, NULL);
}
