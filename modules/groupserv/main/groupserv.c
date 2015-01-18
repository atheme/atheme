/* groupserv.c - group services
 * Copyright (C) 2010 Atheme Development Group
 */

#include "atheme.h"
#include "groupserv_main.h"

struct gflags ga_flags[] = {
	{ 'F', GA_FOUNDER },
	{ 'f', GA_FLAGS },
	{ 's', GA_SET },
	{ 'c', GA_CHANACS },
	{ 'm', GA_MEMOS },
	{ 'v', GA_VHOST },
	{ 'i', GA_INVITE },
	{ 'b', GA_BAN },
	{ 'A', GA_ACLVIEW },
	{ 0, 0 }
};

struct gflags mg_flags[] = {
	{ 'r', MG_REGNOLIMIT },
	{ 'a', MG_ACSNOLIMIT },
	{ 'o', MG_OPEN },
	{ 'p', MG_PUBLIC },
	{ 0, 0 }
};

groupserv_config_t gs_config;

mowgli_heap_t *mygroup_heap, *groupacs_heap;

void mygroups_init(void)
{
	mygroup_heap = mowgli_heap_create(sizeof(mygroup_t), HEAP_USER, BH_NOW);
	groupacs_heap = mowgli_heap_create(sizeof(groupacs_t), HEAP_CHANACS, BH_NOW);
}

void mygroups_deinit(void)
{
	mowgli_heap_destroy(mygroup_heap);
	mowgli_heap_destroy(groupacs_heap);
}

static void mygroup_delete(mygroup_t *mg)
{
	mowgli_node_t *n, *tn;

	myentity_del(entity(mg));

	MOWGLI_ITER_FOREACH_SAFE(n, tn, mg->acs.head)
	{
		groupacs_t *ga = n->data;

		mowgli_node_delete(&ga->gnode, &mg->acs);
		mowgli_node_delete(&ga->unode, myentity_get_membership_list(ga->mt));
		object_unref(ga);
	}

	metadata_delete_all(mg);
	strshare_unref(entity(mg)->name);
	mowgli_heap_free(mygroup_heap, mg);
}

mygroup_t *mygroup_add(const char *name)
{
	return mygroup_add_id(NULL, name);
}

mygroup_t *mygroup_add_id(const char *id, const char *name)
{
	mygroup_t *mg;

	mg = mowgli_heap_alloc(mygroup_heap);
	object_init(object(mg), NULL, (destructor_t) mygroup_delete);

	entity(mg)->type = ENT_GROUP;

	if (id)
	{
		if (!myentity_find_uid(id))
			mowgli_strlcpy(entity(mg)->id, id, sizeof(entity(mg)->id));
		else
			entity(mg)->id[0] = '\0';
	}
	else
		entity(mg)->id[0] = '\0';

	entity(mg)->name = strshare_get(name);
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
	mowgli_heap_free(groupacs_heap, ga);
}

groupacs_t *groupacs_add(mygroup_t *mg, myentity_t *mt, unsigned int flags)
{
	groupacs_t *ga;

	return_val_if_fail(mg != NULL, NULL);
	return_val_if_fail(mt != NULL, NULL);

	ga = mowgli_heap_alloc(groupacs_heap);
	object_init(object(ga), NULL, (destructor_t) groupacs_des);

	ga->mg = mg;
	ga->mt = mt;
	ga->flags = flags;

	mowgli_node_add(ga, &ga->gnode, &mg->acs);
	mowgli_node_add(ga, &ga->unode, myentity_get_membership_list(mt));

	return ga;
}

groupacs_t *groupacs_find(mygroup_t *mg, myentity_t *mt, unsigned int flags, bool allow_recurse)
{
	mowgli_node_t *n;
	groupacs_t *out = NULL;

	return_val_if_fail(mg != NULL, NULL);
	return_val_if_fail(mt != NULL, NULL);

	mg->visited = true;

	MOWGLI_ITER_FOREACH(n, mg->acs.head)
	{
		groupacs_t *ga = n->data;

		if (out != NULL)
			break;

		if (isgroup(ga->mt) && allow_recurse && !(group(ga->mt)->visited))
		{
			groupacs_t *ga2;

			ga2 = groupacs_find(group(ga->mt), mt, flags, allow_recurse);

			if (ga2 != NULL)
				out = ga;
		}
		else
		{
			if (flags)
			{
				if (ga->mt == mt && ga->mg == mg && (ga->flags & flags))
					out = ga;
			}
			else if (ga->mt == mt && ga->mg == mg)
				out = ga;
		}
	}

	mg->visited = false;

	return out;
}

void groupacs_delete(mygroup_t *mg, myentity_t *mt)
{
	groupacs_t *ga;

	ga = groupacs_find(mg, mt, 0, false);
	if (ga != NULL)
	{
		mowgli_node_delete(&ga->gnode, &mg->acs);
		mowgli_node_delete(&ga->unode, myentity_get_membership_list(mt));
		object_unref(ga);
	}
}

bool groupacs_sourceinfo_has_flag(mygroup_t *mg, sourceinfo_t *si, unsigned int flag)
{
	return groupacs_find(mg, entity(si->smu), flag, true) != NULL;
}

unsigned int groupacs_sourceinfo_flags(mygroup_t *mg, sourceinfo_t *si)
{
	groupacs_t *ga;

	ga = groupacs_find(mg, entity(si->smu), 0, true);
	if (ga == NULL)
		return 0;

	return ga->flags;
}

unsigned int mygroup_count_flag(mygroup_t *mg, unsigned int flag)
{
	mowgli_node_t *n;
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

mowgli_list_t *myentity_get_membership_list(myentity_t *mt)
{
	mowgli_list_t *l;

	l = privatedata_get(mt, "groupserv:membership");
	if (l != NULL)
		return l;

	l = mowgli_list_create();
	privatedata_set(mt, "groupserv:membership", l);

	return l;
}

const char *mygroup_founder_names(mygroup_t *mg)
{
        mowgli_node_t *n;
        groupacs_t *ga;
        static char names[512];

        names[0] = '\0';
        MOWGLI_ITER_FOREACH(n, mg->acs.head)
        {
                ga = n->data;
                if (ga->mt != NULL && ga->flags & GA_FOUNDER)
                {
                        if (names[0] != '\0')
                                mowgli_strlcat(names, ", ", sizeof names);
                        mowgli_strlcat(names, ga->mt->name, sizeof names);
                }
        }
        return names;
}

unsigned int myentity_count_group_flag(myentity_t *mt, unsigned int flagset)
{
	mowgli_list_t *l;
	mowgli_node_t *n;
	unsigned int count = 0;

	l = myentity_get_membership_list(mt);
	MOWGLI_ITER_FOREACH(n, l->head)
	{
		groupacs_t *ga = n->data;

		if (ga->mt == mt && ga->flags & flagset)
			count++;
	}

	return count;
}

unsigned int gs_flags_parser(char *flagstring, bool allow_minus, unsigned int flags)
{
	char *c;
	unsigned int dir = 0;
	unsigned int flag;
	unsigned char n;

	c = flagstring;
	while (*c)
	{
		flag = 0;
		n = 0;
		switch(*c)
		{
		case '+':
			dir = 0;
			break;
		case '-':
			if (allow_minus)
				dir = 1;
			break;
		case '*':
			if (dir)
				flags = 0;
			else
			{
				/* preserve existing flags except GA_BAN */
				flags |= GA_ALL;
				flags &= ~GA_BAN;
			}
			break;
		default:
			while (ga_flags[n].ch != 0 && flag == 0)
			{
				if (ga_flags[n].ch == *c)
					flag = ga_flags[n].value;
				else
					n++;
			}
			if (flag == 0)
				break;
			if (dir)
				flags &= ~flag;
			else
				flags |= flag;
			break;
		}

		c++;
	}

	return flags;
}

void remove_group_chanacs(mygroup_t *mg)
{
	chanacs_t *ca;
	mychan_t *mc;
	myuser_t *successor;
	mowgli_node_t *n, *tn;

	/* kill all their channels and chanacs */
	MOWGLI_ITER_FOREACH_SAFE(n, tn, entity(mg)->chanacs.head)
	{
		ca = n->data;
		mc = ca->mychan;

		/* attempt succession */
		if (ca->level & CA_FOUNDER && mychan_num_founders(mc) == 1 && (successor = mychan_pick_successor(mc)) != NULL)
		{
			slog(LG_INFO, _("SUCCESSION: \2%s\2 to \2%s\2 from \2%s\2"), mc->name, entity(successor)->name, entity(mg)->name);
			slog(LG_VERBOSE, "myuser_delete(): giving channel %s to %s (unused %lds, founder %s, chanacs %zu)",
					mc->name, entity(successor)->name,
					(long)(CURRTIME - mc->used),
					entity(mg)->name,
					MOWGLI_LIST_LENGTH(&mc->chanacs));
			if (chansvs.me != NULL)
				verbose(mc, "Foundership changed to \2%s\2 because \2%s\2 was dropped.", entity(successor)->name, entity(mg)->name);

			chanacs_change_simple(mc, entity(successor), NULL, CA_FOUNDER_0, 0, NULL);
			if (chansvs.me != NULL)
				myuser_notice(chansvs.nick, successor, "You are now founder on \2%s\2 (as \2%s\2).", mc->name, entity(successor)->name);
			object_unref(ca);
		}
		/* no successor found */
		else if (ca->level & CA_FOUNDER && mychan_num_founders(mc) == 1)
		{
			slog(LG_REGISTER, _("DELETE: \2%s\2 from \2%s\2"), mc->name, entity(mg)->name);
			slog(LG_VERBOSE, "myuser_delete(): deleting channel %s (unused %lds, founder %s, chanacs %zu)",
					mc->name, (long)(CURRTIME - mc->used),
					entity(mg)->name,
					MOWGLI_LIST_LENGTH(&mc->chanacs));

			hook_call_channel_drop(mc);
			if (mc->chan != NULL && !(mc->chan->flags & CHAN_LOG))
				part(mc->name, chansvs.nick);
			object_unref(mc);
		}
		else /* not founder */
			object_unref(ca);
	}
}

/*
 * mygroup_rename(mygroup_t *mg, const char *name)
 *
 * Renames a group.
 *
 * Inputs:
 *      - group to rename
 *      - new name
 *
 * Outputs:
 *      - nothing
 *
 * Side Effects:
 *      - a group is renamed.
 */
void mygroup_rename(mygroup_t *mg, const char *name)
{
	stringref newname;
	char nb[NICKLEN];

	return_if_fail(mg != NULL);
	return_if_fail(name != NULL);
	return_if_fail(strlen(name) < NICKLEN);

	mowgli_strlcpy(nb, entity(mg)->name, NICKLEN);
	newname = strshare_get(name);

	myentity_del(entity(mg));

	strshare_unref(entity(mg)->name);
	entity(mg)->name = newname;

	myentity_put(entity(mg));
}
