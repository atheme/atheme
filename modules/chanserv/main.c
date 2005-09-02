/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains the main() routine.
 *
 * $Id: main.c 1993 2005-09-01 04:21:34Z nenolod $
 */

#include "atheme.h"

DECLARE_MODULE_V1("chanserv/main", FALSE, _modinit, _moddeinit);

list_t cs_cmdtree;
list_t cs_fcmdtree;

/* main services client routine */
static void chanserv(char *origin, uint8_t parc, char *parv[])
{
	char *cmd, *s;
	char orig[BUFSIZE];
	boolean_t is_fcommand = FALSE;
	hook_cmessage_data_t cdata;

        if (!origin)
        {
                slog(LG_DEBUG, "services(): recieved a request with no origin!");
                return;
        }

	/* this should never happen */
	if (parv[parc - 2][0] == '&')
	{
		slog(LG_ERROR, "services(): got parv with local channel: %s", parv[0]);
		return;
	}

	/* is this a fantasy command? */
	if (parv[parc - 2][0] == '#' && chansvs.fantasy == TRUE)
		is_fcommand = TRUE;
	else if (parv[parc - 2][0] == '#')
		return;

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
		notice(chansvs.nick, origin, "\001PING %s\001", s);
		return;
	}
	else if (!strcmp(cmd, "\001VERSION\001"))
	{
		notice(chansvs.nick, origin,
		       "\001VERSION atheme-%s. %s %s %s%s%s%s%s%s%s%s%s TS5ow\001",
		       version, revision, me.name,
		       (match_mapping) ? "A" : "",
		       (me.loglevel & LG_DEBUG) ? "d" : "",
		       (me.auth) ? "e" : "",
		       (config_options.flood_msgs) ? "F" : "",
		       (config_options.leave_chans) ? "l" : "",
		       (config_options.join_chans) ? "j" : "",
		       (config_options.leave_chans) ? "l" : "", (config_options.join_chans) ? "j" : "", (!match_mapping) ? "R" : "", (config_options.raw) ? "r" : "", (runflags & RF_LIVE) ? "n" : "");

		return;
	}
	else if (!strcmp(cmd, "\001CLIENTINFO\001"))
	{
		/* easter egg :X */
		notice(chansvs.nick, origin, "\001CLIENTINFO 114 97 107 97 117 114\001");
		return;
	}

	/* ctcps we don't care about are ignored */
	else if (*cmd == '\001')
		return;

	/* take the command through the hash table */
	if (!is_fcommand)
		command_exec(chansvs.disp, origin, cmd, &cs_cmdtree);
	else
	{
		fcommand_exec(parv[parc - 2], origin, cmd, &cs_fcmdtree);

		cdata.c = channel_find(parv[parc - 2]);
		cdata.msg = parv[parc - 1];
		hook_call_event("channel_message", &cdata);
	}
}

static void chanserv_config_ready(void *unused)
{
	if (chansvs.me)
		del_service(chansvs.me);

	chansvs.me = add_service(chansvs.nick, chansvs.user,
				 chansvs.host, chansvs.real, chanserv);
	chansvs.disp = chansvs.me->disp;

	hook_del_hook("config_ready", chanserv_config_ready);
}

void _modinit(module_t *m)
{
	hook_add_event("config_ready");
	hook_add_hook("config_ready", chanserv_config_ready);

	if (!cold_start)
	{
		chansvs.me = add_service(chansvs.nick, chansvs.user, chansvs.host, chansvs.real, chanserv);
		chansvs.disp = chansvs.me->disp;
	}
}

void _moddeinit(void)
{
	if (chansvs.me)
		del_service(chansvs.me);
}
