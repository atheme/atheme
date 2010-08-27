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

typedef struct groupacs_ groupacs_t;

#define GA_FOUNDER		0x00000001
#define GA_FLAGS		0x00000002
#define GA_CHANACS		0x00000004
#define GA_MEMOS		0x00000008
#define GA_ALL			(GA_FLAGS | GA_CHANACS | GA_MEMOS)

struct groupacs_ {
	object_t parent;

	mygroup_t *mg;
	myuser_t *mu;
	unsigned int flags;

	node_t node;
};

E void mygroups_init(void);
E void mygroups_deinit(void);
E mygroup_t *mygroup_add(const char *name);
E mygroup_t *mygroup_find(const char *name);

E groupacs_t *groupacs_add(mygroup_t *mg, myuser_t *mu, unsigned int flags);
E groupacs_t *groupacs_find(mygroup_t *mg, myuser_t *mu, unsigned int flags);
E void groupacs_delete(mygroup_t *mg, myuser_t *mu);
E bool groupacs_sourceinfo_has_flag(mygroup_t *mg, sourceinfo_t *si, unsigned int flag);

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
E struct gflags ga_flags[];

#endif /* !GROUPSERV_H */
