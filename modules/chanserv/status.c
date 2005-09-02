/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService STATUS function.
 *
 * $Id: status.c 1989 2005-09-01 03:26:45Z nenolod $
 */

#include "atheme.h"

DECLARE_MODULE_V1("chanserv/status", FALSE, _modinit, _moddeinit);

static void cs_cmd_status(char *origin);

command_t cs_status = { "STATUS", "Displays your status in services.",
                         AC_NONE, cs_cmd_status };

list_t *cs_cmdtree;

void _modinit(module_t *m)
{
	cs_cmdtree = module_locate_symbol("chanserv/main", "cs_cmdtree");
        command_add(&cs_status, cs_cmdtree);
}

void _moddeinit()
{
	command_delete(&cs_status, cs_cmdtree);
}

static void cs_cmd_status(char *origin)
{
	user_t *u = user_find(origin);
	char *chan = strtok(NULL, " ");

	if (!u->myuser)
	{
		notice(chansvs.nick, origin, "You are not logged in.");
		return;
	}

	if (chan)
	{
		mychan_t *mc = mychan_find(chan);
		chanacs_t *ca;

		if (*chan != '#')
		{
			notice(chansvs.nick, origin, "Invalid parameters specified for \2STATUS\2.");
			return;
		}

		if (!mc)
		{
			notice(chansvs.nick, origin, "\2%s\2 is not registered.", chan);
			return;
		}

		if (is_founder(mc, u->myuser))
		{
			notice(chansvs.nick, origin, "You are founder on \2%s\2.", mc->name);
			return;
		}

		if (is_xop(mc, u->myuser, CA_VOP))
		{
			notice(chansvs.nick, origin, "You are VOP on \2%s\2.", mc->name);
			return;
		}

		if (is_xop(mc, u->myuser, CA_AOP))
		{
			notice(chansvs.nick, origin, "You are AOP on \2%s\2.", mc->name);
			return;
		}

		if (is_xop(mc, u->myuser, CA_SOP))
		{
			notice(chansvs.nick, origin, "You are SOP on \2%s\2.", mc->name);
			return;
		}

		if ((ca = chanacs_find(mc, u->myuser, 0x0)))
		{
			notice(chansvs.nick, origin, "You have access flags \2%s\2 on \2%s\2.", bitmask_to_flags(ca->level, chanacs_flags), mc->name);
			return;
		}

		notice(chansvs.nick, origin, "You are a normal user on \2%s\2.", mc->name);

		return;
	}

	notice(chansvs.nick, origin, "You are logged in as \2%s\2.", u->myuser->name);

	if (is_sra(u->myuser))
		notice(chansvs.nick, origin, "You are a services root administrator.");

	if (is_admin(u))
		notice(chansvs.nick, origin, "You are a server administrator.");

	if (is_ircop(u))
		notice(chansvs.nick, origin, "You are an IRC operator.");
}
