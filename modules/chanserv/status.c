/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService STATUS function.
 *
 * $Id: status.c 6813 2006-10-21 20:18:14Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/status", FALSE, _modinit, _moddeinit,
	"$Id: status.c 6813 2006-10-21 20:18:14Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_status(sourceinfo_t *si, int parc, char *parv[]);

command_t cs_status = { "STATUS", "Displays your status in services.",
                         AC_NONE, 1, cs_cmd_status };

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

static void cs_cmd_status(sourceinfo_t *si, int parc, char *parv[])
{
	char *chan = parv[0];

	if (chan)
	{
		mychan_t *mc = mychan_find(chan);
		uint32_t flags;

		if (*chan != '#')
		{
			command_fail(si, fault_badparams, STR_INVALID_PARAMS, "STATUS");
			return;
		}

		if (!mc)
		{
			command_fail(si, fault_nosuch_target, "\2%s\2 is not registered.", chan);
			return;
		}

		logcommand(si, CMDLOG_GET, "%s STATUS", mc->name);
		
		if (metadata_find(mc, METADATA_CHANNEL, "private:close:closer"))
		{
			command_fail(si, fault_noprivs, "\2%s\2 is closed.", chan);
			return;
		}

		if (is_founder(mc, si->smu))
			command_success_nodata(si, "You are founder on \2%s\2.", mc->name);

		flags = chanacs_source_flags(mc, si);
		if (flags & CA_AKICK)
			command_success_nodata(si, "You are banned from \2%s\2.", mc->name);
		else if (flags != 0)
		{
			command_success_nodata(si, "You have access flags \2%s\2 on \2%s\2.", bitmask_to_flags(flags, chanacs_flags), mc->name);
		}
		else
			command_success_nodata(si, "You are a normal user on \2%s\2.", mc->name);

		return;
	}

	logcommand(si, CMDLOG_GET, "STATUS");
	if (!si->smu)
		command_success_nodata(si, "You are not logged in.");
	else
	{
		command_success_nodata(si, "You are logged in as \2%s\2.", si->smu->name);

		if (is_soper(si->smu))
		{
			operclass_t *operclass;

			operclass = si->smu->soper->operclass;
			if (operclass == NULL)
				command_success_nodata(si, "You are a services root administrator.");
			else
				command_success_nodata(si, "You are a services operator of class %s.", operclass->name);
		}
	}

	if (is_admin(si->su))
		command_success_nodata(si, "You are a server administrator.");

	if (is_ircop(si->su))
		command_success_nodata(si, "You are an IRC operator.");
}
