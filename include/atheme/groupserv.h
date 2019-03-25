/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2010 Atheme Project (http://atheme.org/)
 * Copyright (C) 2018-2019 Atheme Development Group (https://atheme.github.io/)
 */

#include "stdheaders.h"

#ifndef ATHEME_INC_GROUPSERV_H
#define ATHEME_INC_GROUPSERV_H 1

#include "entity.h"
#include "object.h"
#include "structures.h"

#define MG_REGNOLIMIT           0x00000001U
#define MG_ACSNOLIMIT           0x00000002U
#define MG_OPEN                 0x00000004U
#define MG_PUBLIC               0x00000008U

#define GA_FOUNDER              0x00000001U
#define GA_FLAGS                0x00000002U
#define GA_CHANACS              0x00000004U
#define GA_MEMOS                0x00000008U
#define GA_SET                  0x00000010U
#define GA_VHOST                0x00000020U
#define GA_BAN                  0x00000040U
#define GA_INVITE               0x00000080U
#define GA_ACLVIEW              0x00000100U

#define GA_ALL_OLD              (GA_FLAGS | GA_CHANACS | GA_MEMOS | GA_SET | GA_VHOST | GA_INVITE)
#define GA_ALL                  (GA_FLAGS | GA_CHANACS | GA_MEMOS | GA_SET | GA_VHOST | GA_INVITE | GA_ACLVIEW)

struct mygroup
{
	struct myentity ent;
	mowgli_list_t   acs;
	time_t          regtime;
	unsigned int    flags;
	bool            visited;
};

struct groupacs
{
	struct atheme_object    parent;
	struct mygroup *        mg;
	struct myentity *       mt;
	unsigned int            flags;
	mowgli_node_t           gnode;
	mowgli_node_t           unode;
};

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
	const char *              (*mygroup_founder_names)(const struct mygroup *);
	void                      (*mygroup_rename)(struct mygroup *, const char *);
	unsigned int              (*myentity_count_group_flag)(struct myentity *, unsigned int);
	mowgli_list_t *           (*myentity_get_membership_list)(struct myentity *);
	void                      (*remove_group_chanacs)(struct mygroup *);
};

#endif /* !ATHEME_MOD_GROUPSERV_GROUPSERV_H */
