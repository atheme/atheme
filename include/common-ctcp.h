/*
 * Copyright (c) 2005, 2006 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains IRC interaction routines.
 *
 * $Id$
 */

#ifndef COMMON_CTCP_H
#define COMMON_CTCP_H

E void common_ctcp_init(void);
E unsigned int handle_ctcp_common(char *, char *, char *);

#endif
