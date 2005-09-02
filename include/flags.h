/*
 * Copyright (C) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Data structures for flags to bitmask processing routines.
 *
 * $Id: flags.h 154 2005-05-29 06:42:48Z nenolod $
 */

#ifndef FLAGS_H
#define FLAGS_H

/* flags stuff */
#define FLAGS_ADD       0x1
#define FLAGS_DEL       0x2

struct flags_table
{
	char flag;
	int value;
};

#endif
