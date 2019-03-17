/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2010 William Pitcock, et al.
 *
 * Data structures for flags to bitmask processing routines.
 */

#ifndef ATHEME_INC_FLAGS_H
#define ATHEME_INC_FLAGS_H 1

#include <atheme/stdheaders.h>
#include <atheme/structures.h>

/* flags stuff */
struct flags_table
{
	unsigned int    value;
	unsigned int    restrictflags;
	bool            def;
	const char *    name;
};

struct gflags
{
	char            ch;
	unsigned int    value;
};

extern unsigned int ca_all;
extern struct flags_table chanacs_flags[256];

unsigned int flags_associate(unsigned char flag, unsigned int restrictflags, bool def, const char *name);
void flags_clear(unsigned char flag);
unsigned int flags_find_slot(void);

void flags_make_bitmasks(const char *string, unsigned int *addflags, unsigned int *removeflags);
unsigned int flags_to_bitmask(const char *, unsigned int flags);
char *bitmask_to_flags(unsigned int);
char *bitmask_to_flags2(unsigned int, unsigned int);
unsigned int allow_flags(struct mychan *mc, unsigned int flags);
void update_chanacs_flags(void);

extern const struct gflags mu_flags[];
extern const struct gflags mc_flags[];
extern const struct gflags mg_flags[];
extern const struct gflags ga_flags[];
extern const struct gflags soper_flags[];

char *gflags_tostr(const struct gflags *gflags, unsigned int flags);
bool gflags_fromstr(const struct gflags *gflags, const char *f, unsigned int *res);

unsigned int xflag_lookup(const char *name);
unsigned int xflag_apply(unsigned int in, const char *name);
const char *xflag_tostr(unsigned int flags);

#endif /* !ATHEME_INC_FLAGS_H */
