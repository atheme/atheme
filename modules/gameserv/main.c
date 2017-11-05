/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains the main() routine.
 */

#include "atheme.h"

DECLARE_MODULE_V1("gameserv/main", MODULE_UNLOAD_CAPABILITY_OK,
                  _modinit, _moddeinit,
                  PACKAGE_STRING, VENDOR_STRING);

service_t *gs;

void _modinit(module_t *m)
{
	gs = service_add("gameserv", NULL);
}

void _moddeinit(module_unload_intent_t intent)
{
        if (gs != NULL)
                service_delete(gs);
}
