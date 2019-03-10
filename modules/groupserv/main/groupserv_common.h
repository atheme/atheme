/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2010 Atheme Project (http://atheme.org/)
 *
 * groupserv_common.h - group services common definitions
 */

#ifndef ATHEME_MOD_GROUPSERV_MAIN_GROUPSERV_COMMON_H
#define ATHEME_MOD_GROUPSERV_MAIN_GROUPSERV_COMMON_H 1

#include <atheme.h>

struct groupserv_config
{
    unsigned int maxgroups;
    unsigned int maxgroupacs;
    bool enable_open_groups;
    char *join_flags;
};

#endif /* !ATHEME_MOD_GROUPSERV_MAIN_GROUPSERV_COMMON_H */
