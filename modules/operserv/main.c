/*
 * Copyright (c) 2005-2006 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains the main() routine.
 *
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/main", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

mowgli_list_t os_conftable;

service_t *opersvs = NULL;

void _modinit(module_t *m)
{
        opersvs = service_add("operserv", NULL, &os_conftable);
}

void _moddeinit(void)
{
	if (opersvs != NULL)
		service_delete(opersvs);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
