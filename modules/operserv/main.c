/*
 * Copyright (c) 2005-2006 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains the main() routine.
 */

#include "atheme.h"

static struct service *opersvs = NULL;

static void
mod_init(struct module *const restrict m)
{
        opersvs = service_add("operserv", NULL);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	if (opersvs != NULL)
		service_delete(opersvs);
}

SIMPLE_DECLARE_MODULE_V1("operserv/main", MODULE_UNLOAD_CAPABILITY_OK)
