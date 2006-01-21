/*
 * Copyright (C) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This code contains the channel mode definitions for ratbox ircd.
 *
 * $Id: hyperion.h 4639 2006-01-21 22:06:41Z jilles $
 */

#ifndef RATBOX_H
#define RATBOX_H


/* Extended channel modes will eventually go here. */
#define CMODE_NOCOLOR	0x00001000	/* hyperion +c */
#define CMODE_REGONLY	0x00002000	/* hyperion +r */
#define CMODE_OPMOD	0x00004000	/* hyperion +z */
#define CMODE_FINVITE	0x00008000	/* hyperion +g */
#define CMODE_EXLIMIT   0x00010000      /* hyperion +L */
#define CMODE_PERM      0x00020000      /* hyperion +P */
#define CMODE_DISFWD    0x00080000      /* hyperion +Q */
#define CMODE_JUPED     0x00100000      /* hyperion +j */
#define CMODE_MODREG	0x00200000	/* hyperion +R */

#endif
