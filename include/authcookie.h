/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Remote authentication ticket management.
 */

#ifndef ATHEME_INC_AUTHCOOKIE_H
#define ATHEME_INC_AUTHCOOKIE_H 1

#define AUTHCOOKIE_LENGTH 20

struct authcookie
{
	char *          ticket;
	struct myuser * myuser;
	time_t          expire;
	mowgli_node_t   node;
};

void authcookie_init(void);
struct authcookie *authcookie_create(struct myuser *mu);
struct authcookie *authcookie_find(const char *ticket, struct myuser *myuser);
void authcookie_destroy(struct authcookie *ac);
void authcookie_destroy_all(struct myuser *mu);
bool authcookie_validate(const char *ticket, struct myuser *myuser);
void authcookie_expire(void *arg);

#endif /* !ATHEME_INC_AUTHCOOKIE_H */
