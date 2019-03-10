/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2011 William Pitcock <nenolod@dereferenced.org>
 *
 * main.c - rpgserv main() routine.
 * based on elly's rpgserv for atheme-6.x --nenolod
 */

#include <atheme.h>

static struct service *rpgserv = NULL;

static void
mod_init(struct module *const restrict m)
{
	if (! (rpgserv = service_add("rpgserv", NULL)))
	{
		(void) slog(LG_ERROR, "%s: service_add() failed", m->name);

		m->mflags |= MODFLAG_FAIL;
		return;
	}
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	(void) service_delete(rpgserv);
}

SIMPLE_DECLARE_MODULE_V1("rpgserv/main", MODULE_UNLOAD_CAPABILITY_OK)
