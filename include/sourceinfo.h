/*
 * Copyright (C) 2006 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Data structures for sourceinfo
 *
 * $Id: sourceinfo.h 6587 2006-09-30 22:35:46Z jilles $
 */

#ifndef SOURCEINFO_H
#define SOURCEINFO_H

struct sourceinfo_vtable
{
	const char *description;
	void (*cmd_fail)(sourceinfo_t *si, faultcode_t code, const char *message);
	void (*cmd_success_nodata)(sourceinfo_t *si, const char *message);
	void (*cmd_success_string)(sourceinfo_t *si, const char *result, const char *message);
};

/* structure describing data about a protocol message or service command */
struct sourceinfo_
{
	/* fields describing the source of the message */
	/* at most one of these two can be non-NULL
	 * before server registration, both are NULL, otherwise exactly
	 * one is NULL
	 */
	user_t *su; /* source, if it's a user */
	server_t *s; /* source, if it's a server */

	connection_t *connection;
	myuser_t *smu;

	service_t *service; /* destination service */

	channel_t *c; /* channel this command applies to */

	struct sourceinfo_vtable *v; /* function pointers */
	void *callerdata; /* opaque data pointer for caller */
};

#endif
