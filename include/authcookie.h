/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Remote authentication ticket management.
 *
 * $Id: authcookie.h 2453 2005-09-30 01:14:59Z nenolod $
 */

#ifndef AUTHCOOKIE_H
#define AUTHCOOKIE_H

typedef struct authcookie_ authcookie_t;

struct authcookie_ {
	char *ticket;
	myuser_t *myuser;
	time_t expires;
	node_t node;
};

extern void authcookie_init(void);
extern char *authcookie_create(myuser_t *mu);
extern authcookie_t *authcookie_find(char *ticket, myuser_t *myuser);
extern void authcookie_destroy(authcookie_t *ac);
extern boolean_t authcookie_validate(char *ticket, myuser_t *myuser);

#endif
