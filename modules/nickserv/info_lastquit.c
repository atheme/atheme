/*
 * Copyright (c) 2012 Jilles Tjoelker <jilles@stack.nl>
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Remembers the last quit message of a user.
 */

#include "atheme.h"

static void
user_delete_info_hook(hook_user_delete_t *hdata)
{
	if (hdata->u->myuser == NULL)
		return;
	metadata_add(hdata->u->myuser,
			"private:lastquit:message", hdata->comment);
}

static void
info_hook(hook_user_req_t *hdata)
{
	struct metadata *md;

	if (!(hdata->mu->flags & MU_PRIVATE) || hdata->si->smu == hdata->mu ||
			has_priv(hdata->si, PRIV_USER_AUSPEX))
	{
		md = metadata_find(hdata->mu, "private:lastquit:message");
		if (md != NULL)
			command_success_nodata(hdata->si, "Last quit  : %s",
					md->value);
	}
}

static void
mod_init(struct module *const restrict m)
{
	hook_add_event("user_delete_info");
	hook_add_user_delete_info(user_delete_info_hook);

	hook_add_event("user_info");
	hook_add_first_user_info(info_hook);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	hook_del_user_delete_info(user_delete_info_hook);
	hook_del_user_info(info_hook);
}

SIMPLE_DECLARE_MODULE_V1("nickserv/info_lastquit", MODULE_UNLOAD_CAPABILITY_OK)
