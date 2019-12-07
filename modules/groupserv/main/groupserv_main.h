/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2010 Atheme Project (http://atheme.org/)
 *
 * groupserv_main.h - group services main module header
 */

#ifndef ATHEME_MOD_GROUPSERV_MAIN_GROUPSERV_MAIN_H
#define ATHEME_MOD_GROUPSERV_MAIN_GROUPSERV_MAIN_H 1

#include <atheme.h>
#include "groupserv_common.h"

extern struct groupserv_config gs_config;

void mygroups_init(void);
void mygroups_deinit(void);
struct mygroup *mygroup_add(const char *name);
struct mygroup *mygroup_add_id(const char *id, const char *name);
struct mygroup *mygroup_find(const char *name);

struct groupacs *groupacs_add(struct mygroup *mg, struct myentity *mt, unsigned int flags);
struct groupacs *groupacs_find(struct mygroup *mg, struct myentity *mt, unsigned int flags, bool allow_recurse);
void groupacs_delete(struct mygroup *mg, struct myentity *mt);

bool groupacs_sourceinfo_has_flag(struct mygroup *mg, struct sourceinfo *si, unsigned int flag);
unsigned int groupacs_sourceinfo_flags(struct mygroup *mg, struct sourceinfo *si);

void gs_db_init(void);
void gs_db_deinit(void);

void gs_hooks_init(void);
void gs_hooks_deinit(void);

void mygroup_set_entity_vtable(struct myentity *mt);
unsigned int mygroup_count_flag(struct mygroup *mg, unsigned int flag);
unsigned int gs_flags_parser(char *flagstring, bool allow_minus, unsigned int flags);
void remove_group_chanacs(struct mygroup *mg);

mowgli_list_t *myentity_get_membership_list(struct myentity *mt);
unsigned int myentity_count_group_flag(struct myentity *mt, unsigned int flagset);

const char *mygroup_founder_names(struct mygroup *mg);
void mygroup_rename(struct mygroup *mg, const char *name);

/* services plumbing */
extern struct service *groupsvs;
extern mowgli_list_t gs_cmdtree;
extern mowgli_list_t conf_gs_table;

#endif /* !ATHEME_MOD_GROUPSERV_MAIN_GROUPSERV_MAIN_H */
