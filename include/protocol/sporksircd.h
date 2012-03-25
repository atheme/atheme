/*
 * Copyright (C) 2005 William Pitcock, et al.
 * Copyright (c) 2008 ShadowIRCd Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This code contains the channel mode definitions for shadowircd.
 *
 */

#ifndef SPORKSIRCD_H
#define SPORKSIRCD_H

/* Extended channel modes will eventually go here. */
/* Note that these are involved in atheme.db file format */
#define CMODE_NOCOLOR   0x00001000      /* hyperion +c */
#define CMODE_REGONLY   0x00002000      /* hyperion +r */
#define CMODE_OPMOD     0x00004000      /* hyperion +z */
#define CMODE_FINVITE   0x00008000      /* hyperion +g */
#define CMODE_EXLIMIT   0x00010000      /* charybdis +L */
#define CMODE_PERM      0x00020000      /* charybdis +P */
#define CMODE_FTARGET   0x00040000      /* charybdis +F */
#define CMODE_DISFWD    0x00080000      /* charybdis +Q */
#define CMODE_NOCTCP    0x00100000      /* charybdis +C */
#define CMODE_IMMUNE    0x00200000      /* sporksircd +M */
#define CMODE_ADMINONLY 0x00400000      /* sporksircd +A */
#define CMODE_OPERONLY  0x00800000      /* sporksircd +O */
#define CMODE_SSLONLY   0x01000000      /* sporksircd +Z */
#define CMODE_NONOTICE 0x02000000      /* sporksircd +T */
#define CMODE_NOCAPS   0x04000000      /* sporksircd +G */
#define CMODE_NOKICKS  0x08000000      /* sporksircd +E */
#define CMODE_NONICKS  0x10000000      /* sporksircd +N */
#define CMODE_NOREPEAT 0x20000000      /* sporksircd +K */
#define CMODE_KICKNOREJOIN 0x40000000   /* sporksircd +J */
#define CMODE_ENABLECENSOR 0x80000000  /* sporksircd +W */

#endif
