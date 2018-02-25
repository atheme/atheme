/* groupserv - group services
 * Copyright (c) 2010 Atheme Development Group
 */

#include "atheme.h"
#include "groupserv_main.h"

struct service *groupsvs;

struct groupserv_persist_record
{
	int version;

	mowgli_heap_t *mygroup_heap;
	mowgli_heap_t *groupacs_heap;
};

extern mowgli_heap_t *mygroup_heap, *groupacs_heap;

static void
mod_init(struct module ATHEME_VATTR_UNUSED *const restrict m)
{
	struct groupserv_persist_record *rec = mowgli_global_storage_get("atheme.groupserv.main.persist");

	if (rec == NULL)
		mygroups_init();
	else
	{
		struct myentity_iteration_state iter;
		struct myentity *grp;

		mygroup_heap = rec->mygroup_heap;
		groupacs_heap = rec->groupacs_heap;

		mowgli_global_storage_free("atheme.groupserv.main.persist");
		free(rec);

		MYENTITY_FOREACH_T(grp, &iter, ENT_GROUP)
		{
			continue_if_fail(isgroup(grp));

			mygroup_set_chanacs_validator(grp);
		}
	}

	groupsvs = service_add("groupserv", NULL);
	add_uint_conf_item("MAXGROUPS", &groupsvs->conf_table, 0, &gs_config.maxgroups, 0, 65535, 5);
	add_uint_conf_item("MAXGROUPACS", &groupsvs->conf_table, 0, &gs_config.maxgroupacs, 0, 65535, 0);
	add_bool_conf_item("ENABLE_OPEN_GROUPS", &groupsvs->conf_table, 0, &gs_config.enable_open_groups, false);
	add_dupstr_conf_item("JOIN_FLAGS", &groupsvs->conf_table, 0, &gs_config.join_flags, "+");

	gs_db_init();
	gs_hooks_init();
}

static void
mod_deinit(const enum module_unload_intent intent)
{
	gs_db_deinit();
	gs_hooks_deinit();
	del_conf_item("MAXGROUPS", &groupsvs->conf_table);
	del_conf_item("MAXGROUPACS", &groupsvs->conf_table);
	del_conf_item("ENABLE_OPEN_GROUPS", &groupsvs->conf_table);
	del_conf_item("JOIN_FLAGS", &groupsvs->conf_table);

	if (groupsvs)
		service_delete(groupsvs);

	switch (intent)
	{
		case MODULE_UNLOAD_INTENT_RELOAD:
		{
			struct groupserv_persist_record *const rec = smalloc(sizeof *rec);

			rec->version = 1;
			rec->mygroup_heap = mygroup_heap;
			rec->groupacs_heap = groupacs_heap;

			mowgli_global_storage_put("atheme.groupserv.main.persist", rec);
			break;
		}

		case MODULE_UNLOAD_INTENT_PERM:
		default:
			mygroups_deinit();
			break;
	}
}

SIMPLE_DECLARE_MODULE_V1("groupserv/main", MODULE_UNLOAD_CAPABILITY_RELOAD_ONLY)
