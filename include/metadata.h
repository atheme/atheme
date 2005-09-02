/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Metadata information.
 *
 * $Id: metadata.h 1590 2005-08-10 05:45:54Z nenolod $
 */

#ifndef METADATA_H
#define METADATA_H

struct metadata_ {
	char *name;
	char *value;
	boolean_t private;
};

typedef struct metadata_ metadata_t;

extern metadata_t *metadata_add(void *target, int32_t type, char *name, char *value);
extern void metadata_delete(void *target, int32_t type, char *name);
extern metadata_t *metadata_find(void *target, int32_t type, char *name);

#define METADATA_USER		1
#define METADATA_CHANNEL	2
#define METADATA_CHANACS	3

#endif

