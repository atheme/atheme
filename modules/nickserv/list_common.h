/* list_common.h - /ns list common definitions
 * Copyright (C) 2010 Atheme Development Group
 */

#ifndef NSLIST_COMMON_H
#define NSLIST_COMMON_H

#include "atheme.h"

typedef enum {
	OPT_BOOL,
	OPT_INT,
	OPT_STRING,
	OPT_FLAG,
	OPT_AGE,
} list_opttype_t;

typedef struct {
	list_opttype_t opttype;
	bool (*is_match)(const mynick_t *mn, const void *arg);
} list_param_t;

#endif /* !NSLIST_COMMON_H */
