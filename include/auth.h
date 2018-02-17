/*
 * Copyright (c) 2005-2009 Atheme Development Group
 * Rights to this code are as documented under doc/LICENSE.
 *
 * Authentication.
 */

#ifndef AUTH_H
#define AUTH_H

extern void set_password(myuser_t *mu, const char *newpassword);
extern bool verify_password(myuser_t *mu, const char *password);

extern bool auth_module_loaded;
extern bool (*auth_user_custom)(myuser_t *mu, const char *password);

#endif
