/*
 * Copyright (c) 2011 Alexandria Wolcott
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains the body of StatServ.
 *
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"statserv/main", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Alexandria Wolcott <alyx@sporksmoo.net>"
);

service_t *statsvs;

void _modinit(module_t *m)
{
	statsvs = service_add("statserv", NULL);
}

void _moddeinit(module_unload_intent_t intent)
{
	if (statsvs != NULL)
		service_delete(statsvs);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
