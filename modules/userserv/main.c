/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains the main() routine.
 *
 * $Id: main.c 5620 2006-07-01 15:56:15Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"userserv/main", FALSE, _modinit, _moddeinit,
	"$Id: main.c 5620 2006-07-01 15:56:15Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

list_t us_cmdtree;
list_t us_helptree;

/* main services client routine */
void userserv(char *origin, uint8_t parc, char *parv[])
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
		notice(usersvs.nick, origin, "\001PING %s\001", s);
		return;
	}
	else if (!strcmp(cmd, "\001VERSION\001"))
	{
		notice(usersvs.nick, origin,
		       "\001VERSION atheme-%s. %s %s %s%s%s%s%s%s%s%s%s TS5ow\001",
		       version, revision, me.name,
		       (match_mapping) ? "A" : "",
		       (me.loglevel & LG_DEBUG) ? "d" : "",
		       (me.auth) ? "e" : "",
		       (config_options.flood_msgs) ? "F" : "",
		       (config_options.leave_chans) ? "l" : "", (config_options.join_chans) ? "j" : "", (!match_mapping) ? "R" : "", (config_options.raw) ? "r" : "", (runflags & RF_LIVE) ? "n" : "");

		return;
	}
	else if (!strcmp(cmd, "\001CLIENTINFO\001"))
	{
		/* easter egg :X */
		notice(usersvs.nick, origin, "\001CLIENTINFO 114 97 107 97 117 114\001");
		return;
	}

	/* ctcps we don't care about are ignored */
	else if (*cmd == '\001')
		return;

	/* take the command through the hash table */
	command_exec(usersvs.me, origin, cmd, &us_cmdtree);
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
