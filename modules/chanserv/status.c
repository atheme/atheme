/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService STATUS function.
 *
 * $Id: status.c 5686 2006-07-03 16:25:03Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/status", FALSE, _modinit, _moddeinit,
	"$Id: status.c 5686 2006-07-03 16:25:03Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_status(char *origin);

command_t cs_status = { "STATUS", "Displays your status in services.",
                         AC_NONE, cs_cmd_status };

list_t *cs_cmdtree;
list_t *cs_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(cs_cmdtree, "chanserv/main", "cs_cmdtree");
	MODULE_USE_SYMBOL(cs_helptree, "chanserv/main", "cs_helptree");

        command_add(&cs_status, cs_cmdtree);
	help_addentry(cs_helptree, "STATUS", "help/cservice/status", NULL);
}

void _moddeinit()
{
	command_delete(&cs_status, cs_cmdtree);
	help_delentry(cs_helptree, "STATUS");
}

static void cs_cmd_status(char *origin)
{
	user_t *u = user_find_named(origin);
	char *chan = strtok(NULL, " ");

	if (!u->myuser)
	{
		notice(chansvs.nick, origin, "You are not logged in.");
		return;
	}

	if (chan)
	{
		mychan_t *mc = mychan_find(chan);
		uint32_t flags;

		if (*chan != '#')
		{
			notice(chansvs.nick, origin, STR_INVALID_PARAMS, "STATUS");
			return;
		}

		if (!mc)
		{
			notice(chansvs.nick, origin, "\2%s\2 is not registered.", chan);
			return;
		}

		logcommand(chansvs.me, u, CMDLOG_GET, "%s STATUS", mc->name);
		
		if (metadata_find(mc, METADATA_CHANNEL, "private:close:closer"))
		{
			notice(chansvs.nick, origin, "\2%s\2 is closed.", chan);
			return;
		}

		if (is_founder(mc, u->myuser))
			notice(chansvs.nick, origin, "You are founder on \2%s\2.", mc->name);

		flags = chanacs_user_flags(mc, u);
		if (flags & CA_AKICK)
			notice(chansvs.nick, origin, "You are banned from \2%s\2.", mc->name);
		else if (flags != 0)
		{
			notice(chansvs.nick, origin, "You have access flags \2%s\2 on \2%s\2.", bitmask_to_flags(flags, chanacs_flags), mc->name);
		}
		else
			notice(chansvs.nick, origin, "You are a normal user on \2%s\2.", mc->name);

		return;
	}

	logcommand(chansvs.me, u, CMDLOG_GET, "STATUS");
	notice(chansvs.nick, origin, "You are logged in as \2%s\2.", u->myuser->name);

	if (is_soper(u->myuser))
	{
		operclass_t *operclass;

		operclass = u->myuser->soper->operclass;
		if (operclass == NULL)
			notice(chansvs.nick, origin, "You are a services root administrator.");
		else
			notice(chansvs.nick, origin, "You are a services operator of class %s.", operclass->name);
	}

	if (is_admin(u))
		notice(chansvs.nick, origin, "You are a server administrator.");

	if (is_ircop(u))
		notice(chansvs.nick, origin, "You are an IRC operator.");
}
