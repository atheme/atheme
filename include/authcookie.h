/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Remote authentication ticket management.
 */

#ifndef AUTHCOOKIE_H
#define AUTHCOOKIE_H

#define AUTHCOOKIE_LENGTH 20

struct authcookie
{
	char *ticket;
	struct myuser *myuser;
	time_t expire;
	mowgli_node_t node;
};

extern void authcookie_init(void);
extern struct authcookie *authcookie_create(struct myuser *mu);
extern struct authcookie *authcookie_find(const char *ticket, struct myuser *myuser);
extern void authcookie_destroy(struct authcookie *ac);
extern void authcookie_destroy_all(struct myuser *mu);
extern bool authcookie_validate(const char *ticket, struct myuser *myuser);
extern void authcookie_expire(void *arg);

#endif
