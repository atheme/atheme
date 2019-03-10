/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2010 Atheme Project (http://atheme.org/)
 *
 * groupserv.h - group services public interface
 *
 * Include this header for modules other than groupserv/main
 * that need to access group functionality.
 */

#ifndef ATHEME_MOD_GROUPSERV_GROUPSERV_H
#define ATHEME_MOD_GROUPSERV_GROUPSERV_H 1

#include <atheme.h>

#include "main/groupserv_common.h"

struct mygroup * (*mygroup_add)(const char *name);
struct mygroup * (*mygroup_find)(const char *name);
struct mygroup * (*mygroup_rename)(struct mygroup *mg, const char *name);

unsigned int (*mygroup_count_flag)(struct mygroup *mg, unsigned int flag);
unsigned int (*myentity_count_group_flag)(struct myentity *mu, unsigned int flagset);

struct groupacs * (*groupacs_add)(struct mygroup *mg, struct myentity *mt, unsigned int flags);
struct groupacs * (*groupacs_find)(struct mygroup *mg, struct myentity *mt, unsigned int flags, bool allow_recurse);
void (*groupacs_delete)(struct mygroup *mg, struct myentity *mt);

bool (*groupacs_sourceinfo_has_flag)(struct mygroup *mg, struct sourceinfo *si, unsigned int flag);
unsigned int (*groupacs_sourceinfo_flags)(struct mygroup *mg, struct sourceinfo *si);
unsigned int (*gs_flags_parser)(char *flagstring, int allow_minus, unsigned int flags);
mowgli_list_t * (*myentity_get_membership_list)(struct myentity *mu);
const char * (*mygroup_founder_names)(struct mygroup *mg);
void (*remove_group_chanacs)(struct mygroup *mg);

struct groupserv_config *gs_config;

static inline void use_groupserv_main_symbols(struct module *m)
{
    MODULE_TRY_REQUEST_DEPENDENCY(m, "groupserv/main");
    MODULE_TRY_REQUEST_SYMBOL(m, mygroup_add, "groupserv/main", "mygroup_add");
    MODULE_TRY_REQUEST_SYMBOL(m, mygroup_find, "groupserv/main", "mygroup_find");
    MODULE_TRY_REQUEST_SYMBOL(m, mygroup_rename, "groupserv/main", "mygroup_rename");
    MODULE_TRY_REQUEST_SYMBOL(m, mygroup_count_flag, "groupserv/main", "mygroup_count_flag");
    MODULE_TRY_REQUEST_SYMBOL(m, myentity_count_group_flag, "groupserv/main", "myentity_count_group_flag");
    MODULE_TRY_REQUEST_SYMBOL(m, groupacs_add, "groupserv/main", "groupacs_add");
    MODULE_TRY_REQUEST_SYMBOL(m, groupacs_find, "groupserv/main", "groupacs_find");
    MODULE_TRY_REQUEST_SYMBOL(m, groupacs_delete, "groupserv/main", "groupacs_delete");
    MODULE_TRY_REQUEST_SYMBOL(m, groupacs_sourceinfo_has_flag, "groupserv/main", "groupacs_sourceinfo_has_flag");
    MODULE_TRY_REQUEST_SYMBOL(m, groupacs_sourceinfo_flags, "groupserv/main", "groupacs_sourceinfo_flags");

    MODULE_TRY_REQUEST_SYMBOL(m, gs_flags_parser, "groupserv/main", "gs_flags_parser");
    MODULE_TRY_REQUEST_SYMBOL(m, myentity_get_membership_list, "groupserv/main", "myentity_get_membership_list");
    MODULE_TRY_REQUEST_SYMBOL(m, mygroup_founder_names, "groupserv/main", "mygroup_founder_names");
    MODULE_TRY_REQUEST_SYMBOL(m, remove_group_chanacs, "groupserv/main", "remove_group_chanacs");

    MODULE_TRY_REQUEST_SYMBOL(m, gs_config, "groupserv/main", "gs_config");
}

#ifndef IN_GROUPSERV_SET

mowgli_patricia_t *gs_set_cmdtree;

static inline void use_groupserv_set_symbols(struct module *m)
{
    MODULE_TRY_REQUEST_DEPENDENCY(m, "groupserv/set");

    mowgli_patricia_t **gs_set_cmdtree_tmp;
    MODULE_TRY_REQUEST_SYMBOL(m, gs_set_cmdtree_tmp, "groupserv/set", "gs_set_cmdtree");
    gs_set_cmdtree = *gs_set_cmdtree_tmp;
}

#endif

#endif /* !ATHEME_MOD_GROUPSERV_GROUPSERV_H */
