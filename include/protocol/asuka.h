/*
 * Copyright (C) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This code contains the channel mode definitions for ircnet ircd.
 *
 * $Id: asuka.h 634 2005-06-28 20:51:25Z nenolod $
 */

#ifndef RATBOX_H
#define RATBOX_H

#define CMODE_BAN       0x00000000      /* IGNORE */
#define CMODE_EXEMPT    0x00000000      /* IGNORE */ 
#define CMODE_INVEX     0x00000000      /* IGNORE */

/* Extended channel modes will eventually go here. */
#define CMODE_NOCOLOR		0x00001000
#define CMODE_NOCTCP		0x00002000
#define CMODE_DELAYED		0x00004000
#define CMODE_AUDITORIUM	0x00008000
#define CMODE_NOQUIT		0x00010000

#endif
