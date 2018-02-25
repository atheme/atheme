/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains the main() routine.
 */

#include "atheme.h"
#include <limits.h>

static void on_user_identify(struct user *u);
static void on_user_away(struct user *u);

struct service *memosvs = NULL;
/*struct memoserv_conf *memosvs_conf;*/
unsigned int maxmemos;

static void
mod_init(struct module *const restrict m)
{
	hook_add_event("user_identify");
	hook_add_user_identify(on_user_identify);

	hook_add_event("user_away");
	hook_add_user_away(on_user_away);

	memosvs = service_add("memoserv", NULL);

	add_uint_conf_item("MAXMEMOS", &memosvs->conf_table, 0, &maxmemos, 1, INT_MAX, 30);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	hook_del_user_identify(on_user_identify);
	hook_del_user_away(on_user_away);

        if (memosvs != NULL)
                service_delete(memosvs);
}

static void
on_user_identify(struct user *u)
{
	struct myuser *mu = u->myuser;

	if (mu->memoct_new > 0)
	{
		notice(memosvs->me->nick, u->nick, ngettext(N_("You have %d new memo."),
						       N_("You have %d new memos."),
						       mu->memoct_new), mu->memoct_new);
		notice(memosvs->me->nick, u->nick, _("To read them, type /%s%s READ NEW"),
					ircd->uses_rcommand ? "" : "msg ", memosvs->disp);
	}
	if (mu->memos.count >= maxmemos)
	{
		notice(memosvs->me->nick, u->nick, _("Your memo inbox is full! Please "
		                                     "delete memos you no longer need."));
	}
}

static void
on_user_away(struct user *u)
{
	struct myuser *mu;
	struct mynick *mn;

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
	if (mu->memos.count >= maxmemos)
	{
		notice(memosvs->me->nick, u->nick, _("Your memo inbox is full! Please "
		                                     "delete memos you no longer need."));
	}
}

SIMPLE_DECLARE_MODULE_V1("memoserv/main", MODULE_UNLOAD_CAPABILITY_OK)
