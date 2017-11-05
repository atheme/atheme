/*
 * Copyright (c) 2005-2006 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains the main() routine.
 */

#include "atheme.h"

DECLARE_MODULE_V1("operserv/main", MODULE_UNLOAD_CAPABILITY_OK,
                  _modinit, _moddeinit,
                  PACKAGE_STRING, VENDOR_STRING);

service_t *opersvs = NULL;

void _modinit(module_t *m)
{
        opersvs = service_add("operserv", NULL);
}

void _moddeinit(module_unload_intent_t intent)
{
	if (opersvs != NULL)
		service_delete(opersvs);
}
