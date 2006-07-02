/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains the main() routine.
 *
 * $Id: main.c 5668 2006-07-02 18:34:02Z gxti $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"saslserv/main", FALSE, _modinit, _moddeinit,
	"$Id: main.c 5668 2006-07-02 18:34:02Z gxti $",
	"Atheme Development Group <http://www.atheme.org>"
);

list_t sasl_mechanisms;

/* main services client routine */
static void saslserv(char *origin, uint8_t parc, char *parv[])
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
		notice(saslsvs.nick, origin, "\001PING %s\001", s);
		return;
	}
	else if (!strcmp(cmd, "\001VERSION\001"))
	{
		notice(saslsvs.nick, origin,
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
		/* easter eggs are mandatory these days */
		notice(saslsvs.nick, origin, "\001CLIENTINFO LILITH\001");
		return;
	}

	/* ctcps we don't care about are ignored */
	else if (*cmd == '\001')
		return;

	notice(saslsvs.nick, origin, "This service exists to identify connecting clients to the network. It has no public interface.");
}

static void saslserv_config_ready(void *unused)
{
        if (saslsvs.me)
                del_service(saslsvs.me);

        saslsvs.me = add_service(saslsvs.nick, saslsvs.user,
                                 saslsvs.host, saslsvs.real, saslserv);

        hook_del_hook("config_ready", saslserv_config_ready);
}

void _modinit(module_t *m)
{
        hook_add_event("config_ready");
        hook_add_hook("config_ready", saslserv_config_ready);

        if (!cold_start)
        {
                saslsvs.me = add_service(saslsvs.nick, saslsvs.user,
			saslsvs.host, saslsvs.real, saslserv);
        }
	authservice_loaded++;
}

void _moddeinit(void)
{
        if (saslsvs.me)
	{
                del_service(saslsvs.me);
		saslsvs.me = NULL;
	}
	authservice_loaded--;
}
