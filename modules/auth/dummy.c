/*
 * Copyright (c) 2009 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Dummy authentication, always returns failure.
 * This really only serves to have an auth module that always compiles.
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"auth/dummy", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static bool dummy_auth_user(myuser_t *mu, const char *password)
{
	return false;
}

void _modinit(module_t *m)
{
	auth_user_custom = &dummy_auth_user;

	auth_module_loaded = true;
}

void _moddeinit(void)
{
	auth_user_custom = NULL;

	auth_module_loaded = false;
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
