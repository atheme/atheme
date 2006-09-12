/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains the main() routine.
 *
 * $Id: main.c 6359 2006-09-12 23:18:15Z pippijn $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"userserv/main", FALSE, _modinit, _moddeinit,
	"$Id: main.c 6359 2006-09-12 23:18:15Z pippijn $",
	"Atheme Development Group <http://www.atheme.org>"
);

list_t us_cmdtree;
list_t us_helptree;

/* main services client routine */
static void userserv(sourceinfo_t *si, int parc, char *parv[])
{
	char orig[BUFSIZE];
	char *cmd;
	char *text;

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
		handle_ctcp_common(cmd, text, si->su->nick, usersvs.nick);
		return;
	}

	/* take the command through the hash table */
	command_exec_split(usersvs.me, si, cmd, text, &us_cmdtree);
}

static void userserv_config_ready(void *unused)
{
	if (usersvs.me)
		del_service(usersvs.me);

	usersvs.me = add_service(usersvs.nick, usersvs.user,
			usersvs.host, usersvs.real, userserv);
	usersvs.disp = usersvs.me->disp;

	if (nicksvs.nick)
	{
		slog(LG_ERROR, "idiotic conf detected: userserv enabled but nickserv{} block present, ignoring nickserv");
		free(nicksvs.nick);
		nicksvs.nick = NULL;
	}

	hook_del_hook("config_ready", userserv_config_ready);
}

void _modinit(module_t *m)
{
	hook_add_event("config_ready");
	hook_add_hook("config_ready", userserv_config_ready);

	if (!cold_start)
	{
		usersvs.me = add_service(usersvs.nick, usersvs.user,
			usersvs.host, usersvs.real, userserv);
		usersvs.disp = usersvs.me->disp;
	}
	authservice_loaded++;
}

void _moddeinit(void)
{
	if (usersvs.me)
	{
		del_service(usersvs.me);
		usersvs.me = NULL;
	}
	authservice_loaded--;
}
