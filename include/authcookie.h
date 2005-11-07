/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Remote authentication ticket management.
 *
 * $Id: authcookie.h 3627 2005-11-07 07:54:35Z alambert $
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

E void authcookie_init(void);
E authcookie_t *authcookie_create(myuser_t *mu);
E authcookie_t *authcookie_find(char *ticket, myuser_t *myuser);
E void authcookie_destroy(authcookie_t *ac);
E boolean_t authcookie_validate(char *ticket, myuser_t *myuser);
E void authcookie_expire();

#endif
