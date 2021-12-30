/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2009 Atheme Project (http://atheme.org/)
 * Copyright (C) 2018 Atheme Development Group (https://atheme.github.io/)
 *
 * This file contains the main() routine.
 */

#include <atheme.h>
#include "hostserv.h"

static struct service *hostsvs = NULL;

static void
on_user_identify(struct user *u)
{
	struct myuser *mu = u->myuser;
	struct metadata *md;
	char buf[NICKLEN + 20];

	snprintf(buf, sizeof buf, "private:usercloak:%s", u->nick);
	md = metadata_find(mu, buf);
	if (md == NULL)
		md = metadata_find(mu, "private:usercloak");
	if (md == NULL)
		return;

	do_sethost(u, md->value);
}

static void
mod_init(struct module *const restrict m)
{
	if (! (hostsvs = service_add("hostserv", NULL)))
	{
		(void) slog(LG_ERROR, "%s: service_add() failed", m->name);

		m->mflags |= MODFLAG_FAIL;
		return;
	}

	(void) hook_add_first_user_identify(&on_user_identify);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	(void) hook_del_user_identify(&on_user_identify);

	(void) service_delete(hostsvs);
}

SIMPLE_DECLARE_MODULE_V1("hostserv/main", MODULE_UNLOAD_CAPABILITY_OK)
