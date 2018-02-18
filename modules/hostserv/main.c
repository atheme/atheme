/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains the main() routine.
 */

#include "atheme.h"
#include "hostserv.h"

static void on_user_identify(user_t *u);
struct service *hostsvs;

static void
mod_init(module_t *const restrict m)
{
	hook_add_event("user_identify");
	hook_add_user_identify(on_user_identify);

	hostsvs = service_add("hostserv", NULL);
}

static void
mod_deinit(const module_unload_intent_t intent)
{
	if (hostsvs != NULL)
		service_delete(hostsvs);

	hook_del_user_identify(on_user_identify);
}

static void on_user_identify(user_t *u)
{
	myuser_t *mu = u->myuser;
	metadata_t *md;
	char buf[NICKLEN + 20];

	snprintf(buf, sizeof buf, "private:usercloak:%s", u->nick);
	md = metadata_find(mu, buf);
	if (md == NULL)
		md = metadata_find(mu, "private:usercloak");
	if (md == NULL)
		return;

	do_sethost(u, md->value);
}

SIMPLE_DECLARE_MODULE_V1("hostserv/main", MODULE_UNLOAD_CAPABILITY_OK)
