/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005 William Pitcock, et al.
 * Copyright (C) 2008 ShadowIRCd Development Group
 */

#include "sysconf.h"

#ifndef ATHEME_INC_PROTOCOL_ELEMENTAL_IRCD_H
#define ATHEME_INC_PROTOCOL_ELEMENTAL_IRCD_H 1

#define CMODE_NOCOLOR           0x00001000
#define CMODE_REGONLY           0x00002000
#define CMODE_OPMOD             0x00004000
#define CMODE_FINVITE           0x00008000
#define CMODE_EXLIMIT           0x00010000
#define CMODE_PERM              0x00020000
#define CMODE_FTARGET           0x00040000
#define CMODE_DISFWD            0x00080000
#define CMODE_NOCTCP            0x00100000
#define CMODE_IMMUNE            0x00200000
#define CMODE_ADMINONLY         0x00400000
#define CMODE_OPERONLY          0x00800000
#define CMODE_SSLONLY           0x01000000
#define CMODE_NOACTIONS         0x02000000
#define CMODE_NONOTICE          0x04000000
#define CMODE_NOCAPS            0x08000000
#define CMODE_NOKICKS           0x10000000
#define CMODE_NONICKS           0x20000000
#define CMODE_NOREPEAT          0x40000000
#define CMODE_KICKNOREJOIN      0x80000000

#endif /* !ATHEME_INC_PROTOCOL_ELEMENTAL_IRCD_H */
