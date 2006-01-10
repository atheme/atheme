/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains the main() routine.
 *
 * $Id: main.c 4559 2006-01-10 12:04:41Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"helpserv/main", FALSE, _modinit, _moddeinit,
	"$Id: main.c 4559 2006-01-10 12:04:41Z jilles $",
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

	/* ctcp? case-sensitive as per rfc */
	if (!strcmp(cmd, "\001PING"))
	{
		if (!(s = strtok(NULL, " ")))
			s = " 0 ";

		strip(s);
		notice(helpsvs.nick, origin, "\001PING %s\001", s);
		return;
	}
	else if (!strcmp(cmd, "\001VERSION\001"))
	{
		notice(helpsvs.nick, origin,
		       "\001VERSION atheme-%s. %s %s %s%s%s%s%s%s%s%s%s TS5ow\001",
		       version, revision, me.name,
		       (match_mapping) ? "A" : "",
		       (me.loglevel & LG_DEBUG) ? "d" : "",
		       (me.auth) ? "e" : "",
		       (config_options.flood_msgs) ? "F" : "",
		       (config_options.leave_chans) ? "l" : "",
		       (config_options.join_chans) ? "j" : "",
		       (config_options.leave_chans) ? "l" : "", 
		       (config_options.join_chans) ? "j" : "", 
		       (!match_mapping) ? "R" : "", 
		       (config_options.raw) ? "r" : "", 
		       (runflags & RF_LIVE) ? "n" : "");

		return;
	}
	else if (!strcmp(cmd, "\001KRALN\001"))
	{
		notice(helpsvs.nick,origin,
			"\001KRALN No one can help you now... \001");
	}

	/* ctcps we don't care about are ignored */
	else if (*cmd == '\001')
		return;

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
