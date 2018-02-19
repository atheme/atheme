/* list_common.h - /ns list common definitions
 * Copyright (C) 2010 Atheme Development Group
 */

#ifndef NSLIST_COMMON_H
#define NSLIST_COMMON_H

#include "atheme.h"

enum list_opttype
{
	OPT_BOOL,
	OPT_INT,
	OPT_STRING,
	OPT_FLAG,
	OPT_AGE,
};

struct list_param
{
	enum list_opttype opttype;
	bool (*is_match)(const struct mynick *mn, const void *arg);
};

#endif /* !NSLIST_COMMON_H */
