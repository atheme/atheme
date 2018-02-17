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
	myuser_t *myuser;
	time_t expire;
	mowgli_node_t node;
};

extern void authcookie_init(void);
extern struct authcookie *authcookie_create(myuser_t *mu);
extern struct authcookie *authcookie_find(const char *ticket, myuser_t *myuser);
extern void authcookie_destroy(struct authcookie *ac);
extern void authcookie_destroy_all(myuser_t *mu);
extern bool authcookie_validate(const char *ticket, myuser_t *myuser);
extern void authcookie_expire(void *arg);

#endif
