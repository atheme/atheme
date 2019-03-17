/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2012 William Pitcock <nenolod@dereferenced.org>
 * Copyright (C) 2018 Atheme Development Group (https://atheme.github.io/)
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 */

#include <atheme.h>

static struct service *proxyscan = NULL;

static void
mod_init(struct module *const restrict m)
{
	if (! (proxyscan = service_add("proxyscan", NULL)))
	{
		(void) slog(LG_ERROR, "%s: service_add() failed", m->name);

		m->mflags |= MODFLAG_FAIL;
		return;
	}
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	(void) service_delete(proxyscan);
}

SIMPLE_DECLARE_MODULE_V1("proxyscan/main", MODULE_UNLOAD_CAPABILITY_OK)
