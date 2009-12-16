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
	"gameserv/main", false, _modinit, _moddeinit,
	"$Id: main.c 6657 2006-10-04 21:22:47Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

list_t gs_cmdtree;
list_t gs_helptree;
list_t gs_conftable;

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
	gamesvs.nick = gamesvs.me->nick;
	gamesvs.user = gamesvs.me->user;
	gamesvs.host = gamesvs.me->host;
	gamesvs.real = gamesvs.me->real;
}

void _modinit(module_t *m)
{
        hook_add_event("config_ready");
        hook_add_config_ready(gameserv_config_ready);

	gamesvs.me = service_add("gameserv", gameserv, &gs_cmdtree, &gs_conftable);
}

void _moddeinit(void)
{
        if (gamesvs.me)
	{
		gamesvs.nick = NULL;
		gamesvs.user = NULL;
		gamesvs.host = NULL;
		gamesvs.real = NULL;
                service_delete(gamesvs.me);
		gamesvs.me = NULL;
	}
	hook_del_config_ready(gameserv_config_ready);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
