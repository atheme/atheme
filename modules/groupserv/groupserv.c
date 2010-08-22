/* groupserv.c - group services
 * Copyright (C) 2010 Atheme Development Group
 */

#include "atheme.h"
#include "groupserv.h"

static BlockHeap *mygroup_heap;

void mygroups_init(void)
{
	mygroup_heap = BlockHeapCreate(sizeof(mygroup_t), HEAP_USER);
}

static void mygroup_delete(mygroup_t *mg)
{
	metadata_delete_all(mg);
	/* XXX nothing */
}

mygroup_t *mygroup_add(myuser_t *owner, const char *name)
{
	mygroup_t *mg;

	mg = BlockHeapAlloc(mygroup_heap);
	object_init(object(mg), NULL, (destructor_t) mygroup_delete);

	strlcpy(entity(mg)->name, name, sizeof(entity(mg)->name));
	myentity_put(entity(mg));

	mg->owner = owner;

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
