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
};

E void mygroups_init(void);
E mygroup_t *mygroup_add(const char *name);
E mygroup_t *mygroup_find(const char *name);

#endif /* !GROUPSERV_H */
