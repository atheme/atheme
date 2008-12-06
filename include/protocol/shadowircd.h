/*
 * Copyright (C) 2005 William Pitcock, et al.
 * Copyright (c) 2008 ShadowIRCd Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This code contains the channel mode definitions for shadowircd.
 *
 * $Id$
 */

#ifndef SHADOWIRCD_H
#define SHADOWIRCD_H

/* Extended channel modes will eventually go here. */
/* Note that these are involved in atheme.db file format */
#define CMODE_IMMUNE    0x00100000      /* shadowircd +M */
#define CMODE_NOCTCP    0x00200000      /* shadowircd +C */
#define CMODE_ADMINONLY 0x00400000      /* shadowircd +A */
#define CMODE_OPERONLY  0x00800000      /* shadowircd +O */
#define CMODE_SSLONLY   0x01000000      /* shadowircd +Z */
#define CMODE_NOACTIONS 0x02000000      /* shadowircd +D */
#define CMODE_NOSPAM	0x04000000	/* shadowircd +X */
#define CMODE_SCOLOR	0x08000000	/* shadowircd +S */
#define CMODE_NONOTICE	0x10000000	/* shadowircd +T */
#define CMODE_MODNOREG	0x20000000	/* shadowircd +R */
#define CMODE_NOCAPS	0x40000000	/* shadowircd +G */
#define CMODE_NOKICKS	0x80000000	/* shadowircd +E */
#define CMODE_NONICKS	0x00000020	/* shadowircd +N */
#define CMODE_NOREPEAT	0x00000200	/* shadowircd +K */

#endif
