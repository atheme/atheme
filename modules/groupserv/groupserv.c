/* groupserv.c - group services
 * Copyright (C) 2010 Atheme Development Group
 */

#include "atheme.h"
#include "groupserv.h"

struct gflags ga_flags[] = {
	{ 'F', GA_FOUNDER },
	{ 'f', GA_FLAGS },
	{ 's', GA_SET },
	{ 'c', GA_CHANACS },
	{ 'm', GA_MEMOS },
	{ 'v', GA_VHOST },
	{ 0, 0 }
};

struct gflags mg_flags[] = {
	{ 'r', MG_REGNOLIMIT },
	{ 'a', MG_ACSNOLIMIT },
	{ 'o', MG_OPEN },
	{ 'p', MG_PUBLIC },
	{ 0, 0 }
};

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
	node_t *n, *tn;

	myentity_del(entity(mg));

	MOWGLI_ITER_FOREACH_SAFE(n, tn, mg->acs.head)
	{
		groupacs_t *ga = n->data;

		node_del(&ga->gnode, &mg->acs);
		node_del(&ga->unode, myuser_get_membership_list(ga->mu));
		object_unref(ga);
	}

	metadata_delete_all(mg);
	BlockHeapFree(mygroup_heap, mg);
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

	mg->regtime = CURRTIME;

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

static void groupacs_des(groupacs_t *ga)
{
	metadata_delete_all(ga);
	BlockHeapFree(groupacs_heap, ga);
}

groupacs_t *groupacs_add(mygroup_t *mg, myuser_t *mu, unsigned int flags)
{
	groupacs_t *ga;

	return_val_if_fail(mg != NULL, NULL);
	return_val_if_fail(mu != NULL, NULL);

	ga = BlockHeapAlloc(groupacs_heap);
	object_init(object(ga), NULL, (destructor_t) groupacs_des);

	ga->mg = mg;
	ga->mu = mu;
	ga->flags = flags;

	node_add(ga, &ga->gnode, &mg->acs);
	node_add(ga, &ga->unode, myuser_get_membership_list(mu));

	return ga;
}

groupacs_t *groupacs_find(mygroup_t *mg, myuser_t *mu, unsigned int flags)
{
	node_t *n;

	return_val_if_fail(mg != NULL, NULL);
	return_val_if_fail(mu != NULL, NULL);

	MOWGLI_ITER_FOREACH(n, mg->acs.head)
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

void groupacs_delete(mygroup_t *mg, myuser_t *mu)
{
	groupacs_t *ga;

	ga = groupacs_find(mg, mu, 0);
	if (ga != NULL)
	{
		node_del(&ga->gnode, &mg->acs);
		node_del(&ga->unode, myuser_get_membership_list(mu));
		object_unref(ga);
	}
}

bool groupacs_sourceinfo_has_flag(mygroup_t *mg, sourceinfo_t *si, unsigned int flag)
{
	return groupacs_find(mg, si->smu, flag) != NULL;
}

unsigned int mygroup_count_flag(mygroup_t *mg, unsigned int flag)
{
	node_t *n;
	unsigned int count = 0;

	return_val_if_fail(mg != NULL, 0);

	/* optimization: if flags = 0, then that means select everyone, so just
	 * return the list length.
	 */
	if (flag == 0)
		return MOWGLI_LIST_LENGTH(&mg->acs);

	MOWGLI_ITER_FOREACH(n, mg->acs.head)
	{
		groupacs_t *ga = n->data;

		if (ga->flags & flag)
			count++;
	}

	return count;
}

mowgli_list_t *myuser_get_membership_list(myuser_t *mu)
{
	mowgli_list_t *l;

	return_val_if_fail(isuser(mu), NULL);

	l = privatedata_get(mu, "groupserv:membership");
	if (l != NULL)
		return l;

	l = list_create();
	privatedata_set(mu, "groupserv:membership", l);

	return l;	
}

const char *mygroup_founder_names(mygroup_t *mg)
{
        node_t *n;
        groupacs_t *ga;
        static char names[512];

        names[0] = '\0';
        MOWGLI_ITER_FOREACH(n, mg->acs.head)
        {
                ga = n->data;
                if (ga->mu != NULL && ga->flags & GA_FOUNDER)
                {
                        if (names[0] != '\0')
                                strlcat(names, ", ", sizeof names);
                        strlcat(names, entity(ga->mu)->name, sizeof names);
                }
        }
        return names;
}

unsigned int myuser_count_group_flag(myuser_t *mu, unsigned int flagset)
{
	mowgli_list_t *l;
	node_t *n;
	unsigned int count = 0;

	l = myuser_get_membership_list(mu);
	MOWGLI_ITER_FOREACH(n, l->head)
	{
		groupacs_t *ga = n->data;

		if (ga->mu == mu && ga->flags & flagset)
			count++;
	}

	return count;
}
