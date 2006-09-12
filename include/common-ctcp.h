/*
 * Copyright (c) 2005, 2006 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains IRC interaction routines.
 *
 * $Id: common-ctcp.h 48 2006-07-19 13:51:17Z pippijn $
 */

#ifndef COMMON_CTCP_H
#define COMMON_CTCP_H

E void common_ctcp_init(void);
E unsigned int handle_ctcp_common(char *, char *, char *, char *);

#endif
