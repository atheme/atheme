/* groupserv.h - group services
 * Copyright (C) 2010 Atheme Development Group
 */

#ifndef GROUPSERV_H
#define GROUPSERV_H

#include "atheme.h"

typedef struct mygroup_ mygroup_t;

struct mygroup_ {
	myentity_t ent;

	list_t acs;
	time_t regtime;
};

E void mygroups_init(void);
E void mygroups_deinit(void);
E mygroup_t *mygroup_add(const char *name);
E mygroup_t *mygroup_find(const char *name);

E void basecmds_init(void);
E void basecmds_deinit(void);

E void gs_db_init(void);
E void gs_db_deinit(void);

E void mygroup_set_chanacs_validator(myentity_t *mt);

/* services plumbing */
E service_t *groupsvs;
E list_t gs_cmdtree;
E list_t gs_helptree;
E list_t conf_gs_table;

#endif /* !GROUPSERV_H */
