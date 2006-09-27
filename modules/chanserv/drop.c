/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService DROP function.
 *
 * $Id: drop.c 6517 2006-09-27 17:49:58Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/drop", FALSE, _modinit, _moddeinit,
	"$Id: drop.c 6517 2006-09-27 17:49:58Z jilles $",
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
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "DROP");
		command_fail(si, fault_needmoreparams, "Syntax: DROP <#channel>");
		return;
	}

	if (*name != '#')
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "DROP");
		command_fail(si, fault_badparams, "Syntax: DROP <#channel>");
		return;
	}

	if (!(mc = mychan_find(name)))
	{
		command_fail(si, fault_nosuch_target, "\2%s\2 is not registered.", name);
		return;
	}

	if (!is_founder(mc, si->smu) && !has_priv(si->su, PRIV_CHAN_ADMIN))
	{
		command_fail(si, fault_noprivs, "You are not authorized to perform this operation.");
		return;
	}

	if (metadata_find(mc, METADATA_CHANNEL, "private:close:closer") && !has_priv(si->su, PRIV_CHAN_ADMIN))
	{
		logcommand(chansvs.me, si->su, CMDLOG_REGISTER, "%s failed DROP (closed)", mc->name);
		command_fail(si, fault_noprivs, "The channel \2%s\2 is closed; it cannot be dropped.", mc->name);
		return;
	}

	if (!is_founder(mc, si->smu))
	{
		logcommand(chansvs.me, si->su, CMDLOG_ADMIN, "%s DROP", mc->name);
		wallops("%s dropped the channel \2%s\2", si->su->nick, name);
	}
	else
		logcommand(chansvs.me, si->su, CMDLOG_REGISTER, "%s DROP", mc->name);

	snoop("DROP: \2%s\2 by \2%s\2 as \2%s\2", mc->name, si->su->nick, si->smu->name);

	hook_call_event("channel_drop", mc);
	if ((config_options.chan && irccasecmp(mc->name, config_options.chan)) || !config_options.chan)
		part(mc->name, chansvs.nick);
	mychan_delete(mc->name);
	command_success_nodata(si, "The channel \2%s\2 has been dropped.", name);
	return;
}
