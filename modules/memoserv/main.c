/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005 Atheme Project (http://atheme.org/)
 * Copyright (C) 2018 Atheme Development Group (https://atheme.github.io/)
 *
 * This file contains the main() routine.
 */

#include <atheme.h>

static struct service *memosvs = NULL;

// Imported by modules/memoserv/send*.so
extern unsigned int maxmemos;
unsigned int maxmemos;

static void
on_user_identify(struct user *u)
{
	struct myuser *mu = u->myuser;

	if (mu->memoct_new > 0)
	{
		notice(memosvs->me->nick, u->nick, "You have %u new memo(s).", mu->memoct_new);
		notice(memosvs->me->nick, u->nick, "To read them, type \2/msg %s READ NEW\2", memosvs->disp);
	}
	if (mu->memos.count >= maxmemos)
	{
		notice(memosvs->me->nick, u->nick, "Your memo inbox is full! Please "
		                                   "delete memos you no longer need.");
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
		notice(memosvs->me->nick, u->nick, "You have %u new memo(s).", mu->memoct_new);
		notice(memosvs->me->nick, u->nick, "To read them, type \2/msg %s READ NEW\2", memosvs->disp);
	}
	if (mu->memos.count >= maxmemos)
	{
		notice(memosvs->me->nick, u->nick, "Your memo inbox is full! Please "
		                                   "delete memos you no longer need.");
	}
}

static void
mod_init(struct module *const restrict m)
{
	if (! (memosvs = service_add("memoserv", NULL)))
	{
		(void) slog(LG_ERROR, "%s: service_add() failed", m->name);

		m->mflags |= MODFLAG_FAIL;
		return;
	}

	(void) hook_add_user_identify(&on_user_identify);
	(void) hook_add_user_away(&on_user_away);

	(void) add_uint_conf_item("MAXMEMOS", &memosvs->conf_table, 0, &maxmemos, 1, INT_MAX, 30);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	(void) hook_del_user_identify(&on_user_identify);
	(void) hook_del_user_away(&on_user_away);

	(void) service_delete(memosvs);
}

SIMPLE_DECLARE_MODULE_V1("memoserv/main", MODULE_UNLOAD_CAPABILITY_OK)
