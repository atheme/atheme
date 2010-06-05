/*
 * Copyright (C) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This code contains the channel mode definitions for ircnet ircd.
 *
 */

#ifndef RATBOX_H
#define RATBOX_H


/* Extended channel modes will eventually go here. */
#define CMODE_NOCOLOR		0x00001000
#define CMODE_NOCTCP		0x00002000
#define CMODE_DELAYED		0x00004000
#define CMODE_AUDITORIUM	0x00008000
#define CMODE_NOQUIT		0x00010000
#define CMODE_NONOTICE		0x00020000

#endif
