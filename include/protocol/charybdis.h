/*
 * Copyright (C) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This code contains the channel mode definitions for charybdis ircd.
 *
 * $Id: charybdis.h 2223 2005-09-12 14:29:54Z jilles $
 */

#ifndef RATBOX_H
#define RATBOX_H

#define CMODE_BAN       0x00000000      /* IGNORE */
#define CMODE_EXEMPT    0x00000000      /* IGNORE */ 
#define CMODE_INVEX     0x00000000      /* IGNORE */
#define CMODE_QUIET     0x00000000	/* IGNORE */
#define CMODE_DENY      0x00000000	/* IGNORE */

/* Extended channel modes will eventually go here. */
#define CMODE_NOCOLOR	0x00001000	/* hyperion +c */
#define CMODE_REGONLY	0x00002000	/* hyperion +r */
#define CMODE_OPMOD	0x00004000	/* hyperion +z */
#define CMODE_FINVITE	0x00008000	/* hyperion +g */
#define CMODE_EXLIMIT   0x00010000      /* charybdis +L */
#define CMODE_PERM      0x00020000      /* charybdis +P */
#define CMODE_FTARGET   0x00040000      /* charybdis +F */
#define CMODE_DISFWD    0x00080000      /* charybdis +Q */

#endif
