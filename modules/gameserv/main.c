/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains the main() routine.
 *
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"gameserv/main", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

service_t *gs;
mowgli_list_t gs_conftable;

void _modinit(module_t *m)
{
	gs = service_add("gameserv", NULL, &gs_conftable);
}

void _moddeinit(void)
{
        if (gs != NULL)
                service_delete(gs);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
