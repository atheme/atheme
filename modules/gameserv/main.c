/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2007 Atheme Project (http://atheme.org/)
 * Copyright (C) 2018 Atheme Development Group (https://atheme.github.io/)
 *
 * This file contains the main() routine.
 */

#include <atheme.h>

static struct service *gamesvs = NULL;

static void
mod_init(struct module *const restrict m)
{
	if (! (gamesvs = service_add("gameserv", NULL)))
	{
		(void) slog(LG_ERROR, "%s: service_add() failed", m->name);

		m->mflags |= MODFLAG_FAIL;
		return;
	}
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	(void) service_delete(gamesvs);
}

SIMPLE_DECLARE_MODULE_V1("gameserv/main", MODULE_UNLOAD_CAPABILITY_OK)
