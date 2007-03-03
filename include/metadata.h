/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Metadata information.
 *
 * $Id: metadata.h 7779 2007-03-03 13:55:42Z pippijn $
 */

#ifndef METADATA_H
#define METADATA_H

struct metadata_ {
	char *name;
	char *value;
	boolean_t private;
};

typedef struct metadata_ metadata_t;

E metadata_t *metadata_add(void *target, int32_t type, const char *name, const char *value);
E void metadata_delete(void *target, int32_t type, const char *name);
E metadata_t *metadata_find(void *target, int32_t type, const char *name);

#define METADATA_USER		1
#define METADATA_CHANNEL	2
#define METADATA_CHANACS	3

#endif

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
