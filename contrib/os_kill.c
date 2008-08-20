/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Kill users through services, requested by christel@freenode.
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/kill", FALSE, _modinit, _moddeinit,
	"$Id: os_pingspam.c 7785 2007-03-03 15:54:32Z pippijn $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void os_cmd_kill(sourceinfo_t *si, int parc, char *parv[]);

command_t os_kill = { "KILL", "Kill a user with Services.", PRIV_OMODE, 2, os_cmd_kill };

list_t *os_cmdtree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(os_cmdtree, "operserv/main", "os_cmdtree");
	command_add(&os_kill, os_cmdtree);
}

void _moddeinit()
{
	command_delete(&os_kill, os_cmdtree);
}

static void os_cmd_kill(sourceinfo_t *si, int parc, char *parv[])
{
	user_t *target;

	if(!parv[0] || !parv[1])
	{
		notice(opersvs.nick, si->su->nick, "Usage: \2KILL\2 <target> <reason>");
		return;
	}

	if(!(target = user_find_named(parv[0])))
	{
		notice(opersvs.nick, si->su->nick, "\2%s\2 is not on the network", target);
		return;
	}

	kill_user(NULL, target, parv[1]);

	snoop("KILL: \2%s\2 -> \2%s\2 (\2%s\2)", get_oper_name(si), target->nick, parv[1]);
	command_success_nodata(si, "\2%s\2 has been killed.", target->nick);
}
