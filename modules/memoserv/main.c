/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains the main() routine.
 *
 * $Id: main.c 6559 2006-09-29 21:15:10Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"memoserv/main", FALSE, _modinit, _moddeinit,
	"$Id: main.c 6559 2006-09-29 21:15:10Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void on_user_identify(void *vptr);

list_t ms_cmdtree;
list_t ms_helptree;

/* main services client routine */
static void memoserv(sourceinfo_t *si, int parc, char *parv[])
{
	char *cmd;
        char *text;
	char orig[BUFSIZE];

	/* this should never happen */
	if (parv[0][0] == '&')
	{
		slog(LG_ERROR, "services(): got parv with local channel: %s", parv[0]);
		return;
	}

	/* make a copy of the original for debugging */
	strlcpy(orig, parv[parc - 1], BUFSIZE);

	/* lets go through this to get the command */
	cmd = strtok(parv[parc - 1], " ");
	text = strtok(NULL, "");

	if (!cmd)
		return;
	if (*cmd == '\001')
	{
		handle_ctcp_common(cmd, text, si->su->nick, memosvs.nick);
		return;
	}

	/* take the command through the hash table */
	command_exec_split(memosvs.me, si, cmd, text, &ms_cmdtree);
}

static void memoserv_config_ready(void *unused)
{
	if (memosvs.me)
		del_service(memosvs.me);

	memosvs.me = add_service(memosvs.nick, memosvs.user,
				 memosvs.host, memosvs.real,
				 memoserv, &ms_cmdtree);
	memosvs.disp = memosvs.me->disp;

        hook_del_hook("config_ready", memoserv_config_ready);
}

void _modinit(module_t *m)
{
        hook_add_event("config_ready");
        hook_add_hook("config_ready", memoserv_config_ready);
	
	hook_add_event("user_identify");
	hook_add_hook("user_identify", on_user_identify);

        if (!cold_start)
        {
                memosvs.me = add_service(memosvs.nick, memosvs.user,
			memosvs.host, memosvs.real, memoserv, &ms_cmdtree);
                memosvs.disp = memosvs.me->disp;
        }
}

void _moddeinit(void)
{
        if (memosvs.me)
	{
                del_service(memosvs.me);
		memosvs.me = NULL;
	}
}

static void on_user_identify(void *vptr)
{
	user_t *u = vptr;
	myuser_t *mu = u->myuser;
	
	if (mu->memoct_new > 0)
	{
		notice(memosvs.nick, u->nick, "You have %d new memo%s.",
			mu->memoct_new, mu->memoct_new > 1 ? "s" : "");
	}
}
