/*
 * Copyright (C) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This code contains the channel mode definitions for ratbox ircd.
 *
 */

#ifndef RATBOX_H
#define RATBOX_H

#define CMODE_FLOOD	0x00000000	/* IGNORE */

/* Extended channel modes will eventually go here. */
#define CMODE_NOCOLOR	0x00001000
#define CMODE_NOBOTS	0x00002000
#define CMODE_SSLONLY	0x00004000
#define CMODE_NOREPEAT	0x00008000
#define CMODE_KNOCK	0x00010000
#define CMODE_STICKY	0x00020000
#define CMODE_NOQUIT	0x00040000
#define CMODE_REGONLY	0x00080000
#define CMODE_NOSPAM	0x00100000

#endif
