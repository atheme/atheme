/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains the main() routine.
 *
 * $Id: main.c 6337 2006-09-10 15:54:41Z pippijn $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"nickserv/main", FALSE, _modinit, _moddeinit,
	"$Id: main.c 6337 2006-09-10 15:54:41Z pippijn $",
	"Atheme Development Group <http://www.atheme.org>"
);

list_t ns_cmdtree;
list_t ns_helptree;

/* main services client routine */
static void nickserv(sourceinfo_t *si, int parc, char *parv[])
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
		handle_ctcp_common(cmd, si->su->nick, nicksvs.nick);
		return;
	}

	/* take the command through the hash table */
	command_exec_split(nicksvs.me, si, cmd, text, &ns_cmdtree);
}

static void nickserv_config_ready(void *unused)
{
        if (nicksvs.me)
                del_service(nicksvs.me);

        nicksvs.me = add_service(nicksvs.nick, nicksvs.user,
                                 nicksvs.host, nicksvs.real, nickserv);
        nicksvs.disp = nicksvs.me->disp;

	if (usersvs.nick)
	{
		slog(LG_ERROR, "idiotic conf detected: nickserv enabled but userserv{} block present, ignoring userserv");
		free(usersvs.nick);
		usersvs.nick = NULL;
	}

        hook_del_hook("config_ready", nickserv_config_ready);
}

void _modinit(module_t *m)
{
        hook_add_event("config_ready");
        hook_add_hook("config_ready", nickserv_config_ready);

        if (!cold_start)
        {
                nicksvs.me = add_service(nicksvs.nick, nicksvs.user,
			nicksvs.host, nicksvs.real, nickserv);
                nicksvs.disp = nicksvs.me->disp;
        }
	authservice_loaded++;
}

void _moddeinit(void)
{
        if (nicksvs.me)
	{
                del_service(nicksvs.me);
		nicksvs.me = NULL;
	}
	authservice_loaded--;
}
