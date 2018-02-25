/*
 * Copyright (c) 2009 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Dummy authentication, always returns failure.
 * This really only serves to have an auth module that always compiles.
 */

#include "atheme.h"

static bool
dummy_auth_user(struct myuser *mu, const char *password)
{
	return false;
}

static void
mod_init(struct module *const restrict m)
{
	auth_user_custom = &dummy_auth_user;

	auth_module_loaded = true;
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	auth_user_custom = NULL;

	auth_module_loaded = false;
}

SIMPLE_DECLARE_MODULE_V1("auth/dummy", MODULE_UNLOAD_CAPABILITY_OK)
