/*
 * Copyright (c) 2005-2009 Atheme Development Group
 * Rights to this code are as documented under doc/LICENSE.
 *
 * Authentication.
 *
 */

#ifndef AUTH_H
#define AUTH_H

E void set_password(myuser_t *mu, const char *newpassword);
E bool verify_password(myuser_t *mu, const char *password);

E bool auth_module_loaded;
E bool (*auth_user_custom)(myuser_t *mu, const char *password);

#endif

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
