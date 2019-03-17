/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005 Atheme Project (http://atheme.org/)
 *
 * Remote authentication ticket management.
 */

#ifndef ATHEME_INC_AUTHCOOKIE_H
#define ATHEME_INC_AUTHCOOKIE_H 1

#include <atheme/attributes.h>
#include <atheme/stdheaders.h>

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
bool authcookie_validate(const char *ticket, struct myuser *myuser) ATHEME_FATTR_WUR;
void authcookie_expire(void *arg);

#endif /* !ATHEME_INC_AUTHCOOKIE_H */
