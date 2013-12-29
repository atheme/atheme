/* groupserv_main.h - group services main module header
 * Copyright (C) 2010 Atheme Development Group
 */

#ifndef GROUPSERV_MAIN_H
#define GROUPSERV_MAIN_H

#include "atheme.h"
#include "groupserv_common.h"

E groupserv_config_t gs_config;

E void mygroups_init(void);
E void mygroups_deinit(void);
E mygroup_t *mygroup_add(const char *name);
E mygroup_t *mygroup_add_id(const char *id, const char *name);
E mygroup_t *mygroup_find(const char *name);

E groupacs_t *groupacs_add(mygroup_t *mg, myuser_t *mu, unsigned int flags);
E groupacs_t *groupacs_find(mygroup_t *mg, myuser_t *mu, unsigned int flags);
E void groupacs_delete(mygroup_t *mg, myuser_t *mu);
E bool groupacs_sourceinfo_has_flag(mygroup_t *mg, sourceinfo_t *si, unsigned int flag);

E void gs_db_init(void);
E void gs_db_deinit(void);

E void gs_hooks_init(void);
E void gs_hooks_deinit(void);

E void mygroup_set_chanacs_validator(myentity_t *mt);
E unsigned int mygroup_count_flag(mygroup_t *mg, unsigned int flag);
E unsigned int myuser_count_group_flag(myuser_t *mu, unsigned int flagset);
E unsigned int gs_flags_parser(char *flagstring, int allow_minus, unsigned int flags);
E void remove_group_chanacs(mygroup_t *mg);


E mowgli_list_t *myuser_get_membership_list(myuser_t *mu);

E const char *mygroup_founder_names(mygroup_t *mg);

/* services plumbing */
E service_t *groupsvs;
E mowgli_list_t gs_cmdtree;
E mowgli_list_t conf_gs_table;
E struct gflags ga_flags[];
E struct gflags mg_flags[];



#endif
