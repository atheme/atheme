/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2010 Atheme Project (http://atheme.org/)
 *
 * groupserv.h - group services public interface
 */

#include "stdheaders.h"

#ifndef ATHEME_MOD_GROUPSERV_GROUPSERV_H
#define ATHEME_MOD_GROUPSERV_GROUPSERV_H 1

#include "structures.h"

struct groupserv_config
{
	char *          join_flags;
	unsigned int    maxgroups;
	unsigned int    maxgroupacs;
	bool            enable_open_groups;
};

struct groupserv_core_symbols
{
	struct groupserv_config *   config;
	struct service **           groupsvs;
	struct groupacs *         (*groupacs_add)(struct mygroup *, struct myentity *, unsigned int);
	void                      (*groupacs_delete)(struct mygroup *, struct myentity *);
	struct groupacs *         (*groupacs_find)(struct mygroup *, const struct myentity *, unsigned int, bool);
	unsigned int              (*groupacs_sourceinfo_flags)(struct mygroup *, const struct sourceinfo *);
	bool                      (*groupacs_sourceinfo_has_flag)(struct mygroup *, const struct sourceinfo *, unsigned int);
	unsigned int              (*gs_flags_parser)(const char *, bool, unsigned int);
	struct mygroup *          (*mygroup_add_id)(const char *, const char *);
	struct mygroup *          (*mygroup_add)(const char *);
	unsigned int              (*mygroup_count_flag)(const struct mygroup *, unsigned int);
	struct mygroup *          (*mygroup_find)(const char *);
	const char *              (*mygroup_founder_names)(struct mygroup *);
	void                      (*mygroup_rename)(struct mygroup *, const char *);
	unsigned int              (*myentity_count_group_flag)(struct myentity *, unsigned int);
	mowgli_list_t *           (*myentity_get_membership_list)(struct myentity *);
	void                      (*remove_group_chanacs)(struct mygroup *);
};

#endif /* !ATHEME_MOD_GROUPSERV_GROUPSERV_H */
