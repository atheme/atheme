/* groupserv_common.h - group services common definitions
 * Copyright (C) 2010 Atheme Development Group
 */

#ifndef GROUPSERV_COMMON_H
#define GROUPSERV_COMMON_H

#include "atheme.h"

struct groupserv_config
{
    unsigned int maxgroups;
    unsigned int maxgroupacs;
    bool enable_open_groups;
    char *join_flags;
};

#endif /* !GROUPSERV_H */
