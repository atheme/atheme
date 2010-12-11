/*
 * Copyright (C) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This code contains the channel mode definitions for charybdis ircd.
 *
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
#define CMODE_NOCTCP    0x00100000      /* charybdis +C */

#define CMODE_NPC	0x00200000	/* charybdis extensions/m_roleplay.c +N */
#define CMODE_SSLONLY	0x00400000	/* charybdis extensions/chm_sslonly.c +S */
#define CMODE_OPERONLY	0x00800000	/* charybdis extensions/chm_operonly.c +O */
#define CMODE_ADMINONLY	0x01000000	/* charybdis extensions/chm_adminonly.c +A */

#endif
