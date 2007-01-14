/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains the main() routine.
 *
 * $Id: main.c 6657 2006-10-04 21:22:47Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"gameserv/main", FALSE, _modinit, _moddeinit,
	"$Id: main.c 6657 2006-10-04 21:22:47Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

list_t gs_cmdtree;
list_t gs_helptree;

/* main services client routine */
static void gameserv(sourceinfo_t *si, int parc, char *parv[])
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
		handle_ctcp_common(si, cmd, text);
		return;
	}

	/* take the command through the hash table */
	command_exec_split(si->service, si, cmd, text, &gs_cmdtree);
}

static void gameserv_config_ready(void *unused)
{
	if (gamesvs.me)
		del_service(gamesvs.me);

	gamesvs.me = add_service(gamesvs.nick, gamesvs.user,
				 gamesvs.host, gamesvs.real,
				 gameserv, &gs_cmdtree);
	gamesvs.disp = gamesvs.me->disp;

        hook_del_hook("config_ready", gameserv_config_ready);
}

void _modinit(module_t *m)
{
        hook_add_event("config_ready");
        hook_add_hook("config_ready", gameserv_config_ready);

        if (!cold_start)
        {
                gamesvs.me = add_service(gamesvs.nick, gamesvs.user,
			gamesvs.host, gamesvs.real, gameserv, &gs_cmdtree);
                gamesvs.disp = gamesvs.me->disp;
        }
}

void _moddeinit(void)
{
        if (gamesvs.me)
	{
                del_service(gamesvs.me);
		gamesvs.me = NULL;
	}
}

