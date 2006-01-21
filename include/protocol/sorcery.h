/*
 * Copyright (C) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This code contains the channel mode definitions for ratbox ircd.
 *
 * $Id: sorcery.h 4639 2006-01-21 22:06:41Z jilles $
 */

#ifndef RATBOX_H
#define RATBOX_H


/* Extended channel modes will eventually go here. */
#define CMODE_NOCOLOR	0x00001000	/* bahamut +c */
#define CMODE_MODREG	0x00002000	/* bahamut +M */
#define CMODE_REGONLY	0x00004000	/* bahamut +R */
#define CMODE_OPERONLY  0x00008000      /* bahamut +O */

#define CMODE_OWNER     0x10000000      /* unreal +q */
#define CMODE_PROTECT   0x20000000      /* unreal +a */
#define CMODE_HALFOP    0x40000000      /* unreal +h */

#endif
