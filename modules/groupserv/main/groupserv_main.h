/* groupserv_main.h - group services main module header
 * Copyright (C) 2010 Atheme Development Group
 */

#ifndef GROUPSERV_MAIN_H
#define GROUPSERV_MAIN_H

#include "atheme.h"
#include "groupserv_common.h"

extern struct groupserv_config gs_config;

extern void mygroups_init(void);
extern void mygroups_deinit(void);
extern struct mygroup *mygroup_add(const char *name);
extern struct mygroup *mygroup_add_id(const char *id, const char *name);
extern struct mygroup *mygroup_find(const char *name);

extern struct groupacs *groupacs_add(struct mygroup *mg, struct myentity *mt, unsigned int flags);
extern struct groupacs *groupacs_find(struct mygroup *mg, struct myentity *mt, unsigned int flags, bool allow_recurse);
extern void groupacs_delete(struct mygroup *mg, struct myentity *mt);

extern bool groupacs_sourceinfo_has_flag(struct mygroup *mg, struct sourceinfo *si, unsigned int flag);

extern void gs_db_init(void);
extern void gs_db_deinit(void);

extern void gs_hooks_init(void);
extern void gs_hooks_deinit(void);

extern void mygroup_set_chanacs_validator(struct myentity *mt);
extern unsigned int mygroup_count_flag(struct mygroup *mg, unsigned int flag);
extern unsigned int gs_flags_parser(char *flagstring, bool allow_minus, unsigned int flags);
extern void remove_group_chanacs(struct mygroup *mg);

extern mowgli_list_t *myentity_get_membership_list(struct myentity *mt);
extern unsigned int myentity_count_group_flag(struct myentity *mt, unsigned int flagset);

extern const char *mygroup_founder_names(struct mygroup *mg);

/* services plumbing */
extern struct service *groupsvs;
extern mowgli_list_t gs_cmdtree;
extern mowgli_list_t conf_gs_table;
extern gflags_t ga_flags[];
extern gflags_t mg_flags[];

#endif
