/*
 * Copyright (C) 2006 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Data structures for sourceinfo
 *
 * $Id$
 */

#ifndef SOURCEINFO_H
#define SOURCEINFO_H

/* structure describing data about a protocol message or service command */
struct sourceinfo_
{
	/* fields describing the source of the message */
	char *origin; /* textual origin XXX remove me */
	/* at most one of these two can be non-NULL
	 * before server registration, both are NULL, otherwise exactly
	 * one is NULL
	 */
	user_t *su; /* source, if it's a user */
	server_t *s; /* source, if it's a server */

	service_t *sptr; /* destination service */

	channel_t *c; /* channel this command applies to */
};

#endif
