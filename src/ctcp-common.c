/*
 * Copyright (c) 2005, 2006 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains IRC interaction routines.
 *
 * $Id$
 */

#include "atheme.h"

uint8_t handle_ctcp_common(char *cmd, const char *origin, const char *svsnick)
{
	char *s;

	/* ctcp? case-sensitive as per rfc */
	if (!strcmp(cmd, "\001PING"))
	{
		if (!(s = strtok(NULL, " ")))
			s = " 0 ";

		strip(s);
		notice(svsnick, origin, "\001PING %s\001", s);
		return 1;
	}
	else if (!strcmp(cmd, "\001VERSION\001"))
	{
		notice(svsnick, origin,
		       "\001VERSION atheme-%s. %s %s %s%s%s%s%s%s%s%s%s TS5ow\001",
		       version, revision, me.name,
		       (match_mapping) ? "A" : "",
		       (me.loglevel & LG_DEBUG) ? "d" : "",
		       (me.auth) ? "e" : "",
		       (config_options.flood_msgs) ? "F" : "",
		       (config_options.leave_chans) ? "l" : "",
		       (config_options.join_chans) ? "j" : "",
		       (!match_mapping) ? "R" : "", (config_options.raw) ? "r" : "",
		       (runflags & RF_LIVE) ? "n" : "");

		return 1;
	}
	else if (!strcmp(cmd, "\001CLIENTINFO\001"))
	{
		/* easter egg :X */
		notice(svsnick, origin, "\001CLIENTINFO 114 97 107 97 117 114\001");
		return 1;
	}

	return 0;
}
