/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains the main() routine.
 *
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"memoserv/main", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void on_user_identify(user_t *u);
static void on_user_away(user_t *u);

mowgli_list_t ms_conftable;

service_t *memosvs = NULL;

void _modinit(module_t *m)
{
	hook_add_event("user_identify");
	hook_add_user_identify(on_user_identify);

	hook_add_event("user_away");
	hook_add_user_away(on_user_away);

	memosvs = service_add("memoserv", NULL, &ms_conftable);
}

void _moddeinit(void)
{
        if (memosvs != NULL)
                service_delete(memosvs);
}

static void on_user_identify(user_t *u)
{
	myuser_t *mu = u->myuser;
	
	if (mu->memoct_new > 0)
	{
		notice(memosvs->me->nick, u->nick, ngettext(N_("You have %d new memo."),
						       N_("You have %d new memos."),
						       mu->memoct_new), mu->memoct_new);
		notice(memosvs->me->nick, u->nick, _("To read them, type /%s%s READ NEW"),
					ircd->uses_rcommand ? "" : "msg ", memosvs->disp);
	}
}

static void on_user_away(user_t *u)
{
	myuser_t *mu;
	mynick_t *mn;

	if (u->flags & UF_AWAY)
		return;
	mu = u->myuser;
	if (mu == NULL)
	{
		mn = mynick_find(u->nick);
		if (mn != NULL && myuser_access_verify(u, mn->owner))
			mu = mn->owner;
	}
	if (mu == NULL)
		return;
	if (mu->memoct_new > 0)
	{
		notice(memosvs->me->nick, u->nick, ngettext(N_("You have %d new memo."),
						       N_("You have %d new memos."),
						       mu->memoct_new), mu->memoct_new);
		notice(memosvs->me->nick, u->nick, _("To read them, type /%s%s READ NEW"),
					ircd->uses_rcommand ? "" : "msg ", memosvs->disp);
	}
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
