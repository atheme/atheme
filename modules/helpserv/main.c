/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains the main() routine.
 *
 * $Id: main.c 5917 2006-07-18 14:55:02Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"helpserv/main", FALSE, _modinit, _moddeinit,
	"$Id: main.c 5917 2006-07-18 14:55:02Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

list_t hs_cmdtree;
list_t hs_helptree;

/* main services client routine */
static void helpserv(char *origin, uint8_t parc, char *parv[])
{
	char *cmd, *s;
	char orig[BUFSIZE];

	if (!origin)
	{
		slog(LG_DEBUG, "services(): recieved a request with no origin!");
		return;
	}

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

	if (!cmd)
		return;
	if (*cmd == '\001')
	{
		handle_ctcp_common(cmd, origin, helpsvs.nick);
		return;
	}

	/* take the command through the hash table */
	command_exec(helpsvs.me, origin, cmd, &hs_cmdtree);
}

static void helpserv_config_ready(void *unused)
{
	if (helpsvs.me)
		del_service(helpsvs.me);
	helpsvs.me = add_service(helpsvs.nick, helpsvs.user,
				 helpsvs.host, helpsvs.real, helpserv);
	helpsvs.disp = helpsvs.me->disp;

        hook_del_hook("config_ready", helpserv_config_ready);
}

void _modinit(module_t *m)
{
        hook_add_event("config_ready");
        hook_add_hook("config_ready", helpserv_config_ready);

        if (!cold_start)
        {
                helpsvs.me = add_service(helpsvs.nick, helpsvs.user,
			helpsvs.host, helpsvs.real, helpserv);
                helpsvs.disp = helpsvs.me->disp;
        }
}

void _moddeinit(void)
{
        if (helpsvs.me)
	{
                del_service(helpsvs.me);
		helpsvs.me = NULL;
	}
}
