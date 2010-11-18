/* groupserv - group services
 * Copyright (c) 2010 Atheme Development Group
 */

#include "atheme.h"
#include "groupserv.h"

DECLARE_MODULE_V1
(
	"groupserv/main", true, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

service_t *groupsvs;
mowgli_list_t conf_gs_table;

void _modinit(module_t *m)
{
	mygroups_init();
	groupsvs = service_add("groupserv", NULL, &conf_gs_table);
	add_uint_conf_item("MAXGROUPS", &conf_gs_table, 0, &maxgroups, 0, 65535, 5);
	add_uint_conf_item("MAXGROUPACS", &conf_gs_table, 0, &maxgroupacs, 0, 65535, 0);
	add_bool_conf_item("ENABLE_OPEN_GROUPS", &conf_gs_table, 0, &enable_open_groups, false);

	gs_db_init();
	gs_hooks_init();
	basecmds_init();
	set_init();
}

void _moddeinit(void)
{
	gs_db_deinit();
	gs_hooks_deinit();
	basecmds_deinit();
	set_deinit();
	del_conf_item("MAXGROUPS", &conf_gs_table);
	del_conf_item("MAXGROUPACS", &conf_gs_table);
	del_conf_item("ENABLE_OPEN_GROUPS", &conf_gs_table);

	if (groupsvs)
		service_delete(groupsvs);

	mygroups_deinit();
}
