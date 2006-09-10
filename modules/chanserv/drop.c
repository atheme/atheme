/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService DROP function.
 *
 * $Id: drop.c 6337 2006-09-10 15:54:41Z pippijn $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/drop", FALSE, _modinit, _moddeinit,
	"$Id: drop.c 6337 2006-09-10 15:54:41Z pippijn $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_drop(sourceinfo_t *si, int parc, char *parv[]);

command_t cs_drop = { "DROP", "Drops a channel registration.",
                        AC_NONE, 1, cs_cmd_drop };

list_t *cs_cmdtree;
list_t *cs_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(cs_cmdtree, "chanserv/main", "cs_cmdtree");
	MODULE_USE_SYMBOL(cs_helptree, "chanserv/main", "cs_helptree");

        command_add(&cs_drop, cs_cmdtree);
	help_addentry(cs_helptree, "DROP", "help/cservice/drop", NULL);
}

void _moddeinit()
{
	command_delete(&cs_drop, cs_cmdtree);
	help_delentry(cs_helptree, "DROP");
}

static void cs_cmd_drop(sourceinfo_t *si, int parc, char *parv[])
{
	mychan_t *mc;
	char *name = parv[0];

	if (!name)
	{
		notice(chansvs.nick, si->su->nick, STR_INSUFFICIENT_PARAMS, "DROP");
		notice(chansvs.nick, si->su->nick, "Syntax: DROP <#channel>");
		return;
	}

	if (*name != '#')
	{
		notice(chansvs.nick, si->su->nick, STR_INVALID_PARAMS, "DROP");
		notice(chansvs.nick, si->su->nick, "Syntax: DROP <#channel>");
		return;
	}

	if (!(mc = mychan_find(name)))
	{
		notice(chansvs.nick, si->su->nick, "\2%s\2 is not registered.", name);
		return;
	}

	if (!is_founder(mc, si->su->myuser) && !has_priv(si->su, PRIV_CHAN_ADMIN))
	{
		notice(chansvs.nick, si->su->nick, "You are not authorized to perform this operation.");
		return;
	}

	if (metadata_find(mc, METADATA_CHANNEL, "private:close:closer") && !has_priv(si->su, PRIV_CHAN_ADMIN))
	{
		logcommand(chansvs.me, si->su, CMDLOG_REGISTER, "%s failed DROP (closed)", mc->name);
		notice(chansvs.nick, si->su->nick, "The channel \2%s\2 is closed; it cannot be dropped.", mc->name);
		return;
	}

	if (!is_founder(mc, si->su->myuser))
	{
		logcommand(chansvs.me, si->su, CMDLOG_ADMIN, "%s DROP", mc->name);
		wallops("%s dropped the channel \2%s\2", si->su->nick, name);
	}
	else
		logcommand(chansvs.me, si->su, CMDLOG_REGISTER, "%s DROP", mc->name);

	snoop("DROP: \2%s\2 by \2%s\2 as \2%s\2", mc->name, si->su->nick, si->su->myuser->name);

	hook_call_event("channel_drop", mc);
	if ((config_options.chan && irccasecmp(mc->name, config_options.chan)) || !config_options.chan)
		part(mc->name, chansvs.nick);
	mychan_delete(mc->name);
	notice(chansvs.nick, si->su->nick, "The channel \2%s\2 has been dropped.", name);
	return;
}
