/*
 * Copyright (c) 2012 Jilles Tjoelker <jilles@stack.nl>
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Remembers the last quit message of a user.
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"nickserv/info_lastquit", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void user_delete_info_hook(hook_user_delete_t *hdata)
{
	if (hdata->u->myuser == NULL)
		return;
	metadata_add(hdata->u->myuser,
			"private:lastquit:message", hdata->comment);
}

static void info_hook(hook_user_req_t *hdata)
{
	metadata_t *md;

	if (!(hdata->mu->flags & MU_PRIVATE) || hdata->si->smu == hdata->mu ||
			has_priv(hdata->si, PRIV_USER_AUSPEX))
	{
		md = metadata_find(hdata->mu, "private:lastquit:message");
		if (md != NULL)
			command_success_nodata(hdata->si, "Last quit  : %s",
					md->value);
	}
}

void _modinit(module_t *m)
{
	hook_add_event("user_delete_info");
	hook_add_user_delete_info(user_delete_info_hook);

	hook_add_event("user_info");
	hook_add_first_user_info(info_hook);
}

void _moddeinit(module_unload_intent_t intent)
{
	hook_del_user_delete_info(user_delete_info_hook);
	hook_del_user_info(info_hook);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
