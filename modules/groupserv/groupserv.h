/* groupserv.h - group services
 * Copyright (C) 2010 Atheme Development Group
 */

#ifndef GROUPSERV_H
#define GROUPSERV_H

#include "atheme.h"

typedef struct mygroup_ mygroup_t;

#define MG_REGNOLIMIT		0x00000001
#define MG_ACSNOLIMIT		0x00000002
#define MG_OPEN			0x00000004
#define MG_PUBLIC			0x00000008

struct mygroup_ {
	myentity_t ent;

	mowgli_list_t acs;
	time_t regtime;

	unsigned int flags;
};

typedef struct groupacs_ groupacs_t;
unsigned int maxgroups;
unsigned int maxgroupacs;
bool enable_open_groups;
char *join_flags;

#define GA_FOUNDER		0x00000001
#define GA_FLAGS		0x00000002
#define GA_CHANACS		0x00000004
#define GA_MEMOS		0x00000008
#define GA_SET			0x00000010
#define GA_VHOST		0x00000020
#define GA_BAN			0x00000040
#define GA_ALL			(GA_FLAGS | GA_CHANACS | GA_MEMOS | GA_SET | GA_VHOST)

#define PRIV_GROUP_ADMIN "group:admin"
#define PRIV_GROUP_AUSPEX "group:auspex"

struct groupacs_ {
	object_t parent;

	mygroup_t *mg;
	myuser_t *mu;
	unsigned int flags;

	mowgli_node_t gnode;
	mowgli_node_t unode;
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

E void gs_hooks_init(void);
E void gs_hooks_deinit(void);

E void set_init(void);
E void set_deinit(void);

E void mygroup_set_chanacs_validator(myentity_t *mt);
E unsigned int mygroup_count_flag(mygroup_t *mg, unsigned int flag);
E unsigned int myuser_count_group_flag(myuser_t *mu, unsigned int flagset);


E mowgli_list_t *myuser_get_membership_list(myuser_t *mu);

E const char *mygroup_founder_names(mygroup_t *mg);

/* services plumbing */
E service_t *groupsvs;
E mowgli_list_t gs_cmdtree;
E mowgli_list_t conf_gs_table;
E struct gflags ga_flags[];
E struct gflags mg_flags[];

#endif /* !GROUPSERV_H */
