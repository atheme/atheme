/*
 * Copyright (c) 2005-2009 Atheme Development Group
 * Rights to this code are as documented under doc/LICENSE.
 *
 * Authentication.
 */

#ifndef ATHEME_INC_AUTH_H
#define ATHEME_INC_AUTH_H 1

void set_password(struct myuser *mu, const char *newpassword);
bool verify_password(struct myuser *mu, const char *password);

extern bool auth_module_loaded;
extern bool (*auth_user_custom)(struct myuser *mu, const char *password);

#endif /* !ATHEME_INC_AUTH_H */
