/*
 * Copyright (C) 2005-2007 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Data structures for flags to bitmask processing routines.
 *
 * $Id: flags.h 7779 2007-03-03 13:55:42Z pippijn $
 */

#ifndef FLAGS_H
#define FLAGS_H

/* flags stuff */
struct flags_table
{
	char flag;
	int value;
};

E uint32_t ca_all;
E struct flags_table chanacs_flags[];

E void flags_make_bitmasks(const char *string, struct flags_table table[], uint32_t *addflags, uint32_t *removeflags);
E uint32_t flags_to_bitmask(const char *, struct flags_table[], uint32_t flags);
E char *bitmask_to_flags(uint32_t, struct flags_table[]);
E char *bitmask_to_flags2(uint32_t, uint32_t, struct flags_table[]);
E uint32_t allow_flags(uint32_t flags);
E void update_chanacs_flags(void);

#endif

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
