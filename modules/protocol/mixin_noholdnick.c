/*
 * Copyright (c) 2008 Jilles Tjoelker
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Module to disable holdnick enforcers.
 */

#include "atheme.h"

SIMPLE_DECLARE_MODULE_V1("protocol/mixin_noholdnick", MODULE_UNLOAD_CAPABILITY_OK,
                         _modinit, _moddeinit);

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

void _moddeinit(module_unload_intent_t intent)
{

	ircd->flags |= oldflag;
}
