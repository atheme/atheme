/*
 * Copyright (c) 2005-2006 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains IRC interaction routines.
 *
 * $Id: ctcp-common.c 7779 2007-03-03 13:55:42Z pippijn $
 */

#include "atheme.h"

dictionary_tree_t *ctcptree;

static void ctcp_ping_handler(char *cmd, char *args, char *origin, char *svsnick)
{
	char *s;

	s = strtok(args, "\001");
	if (s != NULL)
		strip(s);
	else
		s = "pong!";

	notice(svsnick, origin, "\001PING %.100s\001", s);
}

static void ctcp_version_handler(char *cmd, char *args, char *origin, char *svsnick)
{
	notice(svsnick, origin,
		"\001VERSION atheme-%s. %s %s %s%s%s%s%s%s%s%s%s%s [%s]\001",
		version, revision, me.name,
		(match_mapping) ? "A" : "",
		(log_force || me.loglevel & (LG_DEBUG | LG_RAWDATA)) ? "d" : "",
		(me.auth) ? "e" : "",
		(config_options.flood_msgs) ? "F" : "",
		(config_options.leave_chans) ? "l" : "",
		(config_options.join_chans) ? "j" : "",
		(chansvs.changets) ? "t" : "",
		(!match_mapping) ? "R" : "", (config_options.raw) ? "r" : "",
		(runflags & RF_LIVE) ? "n" : "",
		ircd->ircdname);
}

static void ctcp_clientinfo_handler(char *cmd, char *args, char *origin, char *svsnick)
{
	/* easter egg :X */
	notice(svsnick, origin, "\001CLIENTINFO 114 97 107 97 117 114\001");
}

void common_ctcp_init(void)
{
	ctcptree = dictionary_create("ctcptree", HASH_SMALL, strcmp);

	dictionary_add(ctcptree, "\001PING", ctcp_ping_handler);
	dictionary_add(ctcptree, "\001VERSION\001", ctcp_version_handler);
	dictionary_add(ctcptree, "\001CLIENTINFO\001", ctcp_clientinfo_handler);
}

unsigned int handle_ctcp_common(sourceinfo_t *si, char *cmd, char *args)
{
	void (*handler)(char *, char *, char *, char *);

	handler = dictionary_retrieve(ctcptree, cmd);

	if (handler != NULL)
	{
		handler(cmd, args, si->su->nick, si->service->name);
		return 1;
	}

	return 0;
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
