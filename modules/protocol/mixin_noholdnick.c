/*
 * Copyright (c) 2008 Jilles Tjoelker
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Module to disable holdnick enforcers.
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"protocol/mixin_noholdnick", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

int oldflag;

void _modinit(module_t *m)
{

	if (ircd == NULL)
	{
		slog(LG_ERROR, "Module %s must be loaded after a protocol module.", m->name);
		m->mflags = MODTYPE_FAIL;
		return;
	}
	oldflag = ircd->flags & IRCD_HOLDNICK;
	ircd->flags &= ~IRCD_HOLDNICK;
}

void _moddeinit()
{

	ircd->flags |= oldflag;
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
