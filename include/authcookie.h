/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Remote authentication ticket management.
 *
 * $Id: authcookie.h 3305 2005-10-31 00:19:14Z alambert $
 */

#ifndef AUTHCOOKIE_H
#define AUTHCOOKIE_H

typedef struct authcookie_ authcookie_t;

struct authcookie_ {
	char *ticket;
	myuser_t *myuser;
	time_t expire;
	node_t node;
};

extern void authcookie_init(void);
extern authcookie_t *authcookie_create(myuser_t *mu);
extern authcookie_t *authcookie_find(char *ticket, myuser_t *myuser);
extern void authcookie_destroy(authcookie_t *ac);
extern boolean_t authcookie_validate(char *ticket, myuser_t *myuser);

#endif
