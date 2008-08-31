/*
 * Copyright (C) 2005 William Pitcock, et al.
 * Copyright (c) 2008 ShadowIRCd Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This code contains the channel mode definitions for charybdis ircd.
 *
 * $Id: charybdis.h 4899 2006-03-14 02:36:14Z nenolod $
 */

#ifndef RATBOX_H
#define RATBOX_H


/* Extended channel modes will eventually go here. */
#define CMODE_NOCOLOR	0x00001000	/* hyperion +c */
#define CMODE_REGONLY	0x00002000	/* hyperion +r */
#define CMODE_OPMOD	0x00004000	/* hyperion +z */
#define CMODE_FINVITE	0x00008000	/* hyperion +g */
#define CMODE_EXLIMIT   0x00010000      /* charybdis +L */
#define CMODE_PERM      0x00020000      /* charybdis +P */
#define CMODE_FTARGET   0x00040000      /* charybdis +F */
#define CMODE_DISFWD    0x00080000      /* charybdis +Q */
#define CMODE_IMMUNE    0x00100000      /* shadowircd +M */
#define CMODE_NOCTCP    0x00200000      /* shadowircd +C */
#define CMODE_ADMINONLY 0x00400000      /* shadowircd +A */
#define CMODE_OPERONLY  0x00800000      /* shadowircd +O */
#define CMODE_SSLONLY   0x01000000      /* shadowircd +z */
#define CMODE_NOACTIONS 0x02000000      /* shadowircd +D */
#define CMODE_NOSPAM	0x04000000	/* shadowircd +X */
#define CMODE_SCOLOR	0x08000000	/* shadowircd +S */
#define CMODE_NONOTICE	0x10000000	/* shadowircd +T */
#define CMODE_MODNOREG	0x20000000	/* shadowircd +R */

#endif
