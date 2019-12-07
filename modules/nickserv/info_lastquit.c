/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2012 Jilles Tjoelker <jilles@stack.nl>
 *
 * Remembers the last quit message of a user.
 */

#include <atheme.h>

static void
user_delete_info_hook(struct hook_user_delete_info *hdata)
{
	if (hdata->u->myuser == NULL)
		return;
	metadata_add(hdata->u->myuser,
			"private:lastquit:message", hdata->comment);
}

static void
info_hook(struct hook_user_req *hdata)
{
	struct metadata *md;

	if (!(hdata->mu->flags & MU_PRIVATE) || hdata->si->smu == hdata->mu ||
			has_priv(hdata->si, PRIV_USER_AUSPEX))
	{
		md = metadata_find(hdata->mu, "private:lastquit:message");
		if (md != NULL)
			command_success_nodata(hdata->si, _("Last quit  : %s"),
					md->value);
	}
}

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "nickserv/info")

	hook_add_user_delete_info(user_delete_info_hook);
	hook_add_first_user_info(info_hook);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	hook_del_user_delete_info(user_delete_info_hook);
	hook_del_user_info(info_hook);
}

SIMPLE_DECLARE_MODULE_V1("nickserv/info_lastquit", MODULE_UNLOAD_CAPABILITY_OK)
