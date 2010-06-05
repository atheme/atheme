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
#define CMODE_ADMONLY		0x00040000
#define CMODE_SOFTMOD		0x00080000
#define CMODE_REGONLY		0x00100000
#define CMODE_PERM		0x00200000
#define CMODE_STRIP		0x00400000
#define CMODE_NOAMSG		0x00800000
#define CMODE_OPERONLY		0x01000000
#define CMODE_SOFTPRIV		0x02000000
#define CMODE_SSLONLY		0x04000000

#endif
