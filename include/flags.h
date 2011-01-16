/*
 * Copyright (C) 2005-2010 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Data structures for flags to bitmask processing routines.
 */

#ifndef FLAGS_H
#define FLAGS_H

/* flags stuff */
struct flags_table
{
	unsigned int value;
	unsigned int restrictflags;
	bool def;
	const char *name;
};

E unsigned int ca_all;
E struct flags_table chanacs_flags[255];

E unsigned int flags_associate(unsigned char flag, unsigned int restrictflags, bool def, const char *name);
E void flags_clear(unsigned char flag);
E unsigned int flags_find_slot(void);

E void flags_make_bitmasks(const char *string, unsigned int *addflags, unsigned int *removeflags);
E unsigned int flags_to_bitmask(const char *, unsigned int flags);
E char *bitmask_to_flags(unsigned int);
E char *bitmask_to_flags2(unsigned int, unsigned int);
E unsigned int allow_flags(mychan_t *mc, unsigned int flags);
E void update_chanacs_flags(void);

typedef struct gflags {
	char ch;
	unsigned int value;
} gflags_t;

E struct gflags mu_flags[];
E struct gflags mc_flags[];
E struct gflags soper_flags[];

E char *gflags_tostr(gflags_t *gflags, unsigned int flags);
E bool gflags_fromstr(gflags_t *gflags, const char *f, unsigned int *res);

E unsigned int xflag_lookup(const char *name);
E unsigned int xflag_apply(unsigned int in, const char *name);
E const char *xflag_tostr(unsigned int flags);

#endif

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
