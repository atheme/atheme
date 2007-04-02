/*
 * Copyright (C) 2005-2007 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Data structures for flags to bitmask processing routines.
 *
 * $Id: flags.h 8027 2007-04-02 10:47:18Z nenolod $
 */

#ifndef FLAGS_H
#define FLAGS_H

/* flags stuff */
struct flags_table
{
	char flag;
	int value;
};

E unsigned int ca_all;
E struct flags_table chanacs_flags[];

E void flags_make_bitmasks(const char *string, struct flags_table table[], unsigned int *addflags, unsigned int *removeflags);
E unsigned int flags_to_bitmask(const char *, struct flags_table[], unsigned int flags);
E char *bitmask_to_flags(unsigned int, struct flags_table[]);
E char *bitmask_to_flags2(unsigned int, unsigned int, struct flags_table[]);
E unsigned int allow_flags(unsigned int flags);
E void update_chanacs_flags(void);

#endif

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
