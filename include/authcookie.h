/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Remote authentication ticket management.
 *
 * $Id: authcookie.h 7779 2007-03-03 13:55:42Z pippijn $
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
E void authcookie_destroy_all(myuser_t *mu);
E boolean_t authcookie_validate(char *ticket, myuser_t *myuser);
E void authcookie_expire(void *arg);

#endif

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
