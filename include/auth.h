/*
 * Copyright (c) 2005-2009 Atheme Development Group
 * Rights to this code are as documented under doc/LICENSE.
 *
 * Authentication.
 */

#ifndef AUTH_H
#define AUTH_H

void set_password(struct myuser *mu, const char *newpassword);
bool verify_password(struct myuser *mu, const char *password);

extern bool auth_module_loaded;
extern bool (*auth_user_custom)(struct myuser *mu, const char *password);

#endif
