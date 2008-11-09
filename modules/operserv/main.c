/*
 * Copyright (c) 2005-2006 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains the main() routine.
 *
 * $Id: main.c 7779 2007-03-03 13:55:42Z pippijn $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/main", FALSE, _modinit, _moddeinit,
	"$Id: main.c 7779 2007-03-03 13:55:42Z pippijn $",
	"Atheme Development Group <http://www.atheme.org>"
);

list_t os_cmdtree;
list_t os_helptree;
list_t os_conftable;

/* main services client routine */
static void oservice(sourceinfo_t *si, int parc, char *parv[])
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
	command_exec_split(si->service, si, cmd, text, &os_cmdtree);
}

static void operserv_config_ready(void *unused)
{
	opersvs.nick = opersvs.me->nick;
	opersvs.user = opersvs.me->user;
	opersvs.host = opersvs.me->host;
	opersvs.real = opersvs.me->real;
	opersvs.disp = opersvs.me->disp;
}

void _modinit(module_t *m)
{
        hook_add_event("config_ready");
        hook_add_hook("config_ready", operserv_config_ready);

        opersvs.me = service_add("operserv", oservice, &os_cmdtree, &os_conftable);
}

void _moddeinit(void)
{
	if (opersvs.me)
	{
		opersvs.nick = NULL;
		opersvs.user = NULL;
		opersvs.host = NULL;
		opersvs.real = NULL;
		opersvs.disp = NULL;
		service_delete(opersvs.me);
		opersvs.me = NULL;
	}
        hook_del_hook("config_ready", operserv_config_ready);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
