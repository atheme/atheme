/* groupserv.c - group services
 * Copyright (C) 2010 Atheme Development Group
 */

#include "atheme.h"
#include "groupserv.h"

static BlockHeap *mygroup_heap, *groupacs_heap;

void mygroups_init(void)
{
	mygroup_heap = BlockHeapCreate(sizeof(mygroup_t), HEAP_USER);
	groupacs_heap = BlockHeapCreate(sizeof(groupacs_t), HEAP_CHANACS);
}

void mygroups_deinit(void)
{
	BlockHeapDestroy(mygroup_heap);
	BlockHeapDestroy(groupacs_heap);
}

static void mygroup_delete(mygroup_t *mg)
{
	metadata_delete_all(mg);
	/* XXX nothing */
}

mygroup_t *mygroup_add(const char *name)
{
	mygroup_t *mg;

	mg = BlockHeapAlloc(mygroup_heap);
	object_init(object(mg), NULL, (destructor_t) mygroup_delete);

	entity(mg)->type = ENT_GROUP;

	strlcpy(entity(mg)->name, name, sizeof(entity(mg)->name));
	myentity_put(entity(mg));

	mygroup_set_chanacs_validator(entity(mg));

	return mg;
}

mygroup_t *mygroup_find(const char *name)
{
	myentity_t *mg = myentity_find(name);

	if (mg == NULL)
		return NULL;

	if (!isgroup(mg))
		return NULL;

	return group(mg);
}

static void groupacs_delete(groupacs_t *ga)
{
	metadata_delete_all(ga);
	/* XXX nothing */
}

groupacs_t *groupacs_add(mygroup_t *mg, myuser_t *mu, unsigned int flags)
{
	groupacs_t *ga;

	return_val_if_fail(mg != NULL, NULL);
	return_val_if_fail(mu != NULL, NULL);

	ga = BlockHeapAlloc(groupacs_heap);
	object_init(object(ga), NULL, (destructor_t) groupacs_delete);

	ga->mg = mg;
	ga->mu = mu;
	ga->flags = flags;

	node_add(ga, &ga->node, &mg->acs);
}

groupacs_t *groupacs_find(mygroup_t *mg, myuser_t *mu, unsigned int flags)
{
	node_t *n;

	return_val_if_fail(mg != NULL, NULL);
	return_val_if_fail(mu != NULL, NULL);

	LIST_FOREACH(n, mg->acs.head)
	{
		groupacs_t *ga = n->data;

		if (flags)
		{
			if (ga->mu == mu && ga->mg == mg && (ga->flags & flags))
				return ga;
		}
		else if (ga->mu == mu && ga->mg == mg)
			return ga;
	}

	return NULL;
}
