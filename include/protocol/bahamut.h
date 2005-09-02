/*
 * Copyright (C) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This code contains the channel mode definitions for ratbox ircd.
 *
 * $Id: bahamut.h 174 2005-05-29 07:56:25Z nenolod $
 */

#ifndef RATBOX_H
#define RATBOX_H

#define CMODE_BAN       0x00000000      /* IGNORE */
#define CMODE_EXEMPT    0x00000000      /* IGNORE */ 
#define CMODE_INVEX     0x00000000      /* IGNORE */

/* Extended channel modes will eventually go here. */
#define CMODE_NOCOLOR	0x00001000	/* bahamut +c */
#define CMODE_MODREG	0x00002000	/* bahamut +M */
#define CMODE_REGONLY	0x00004000	/* bahamut +R */
#define CMODE_OPERONLY  0x00008000      /* bahamut +O */

#endif
