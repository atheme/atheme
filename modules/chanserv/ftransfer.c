/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService FTRANSFER function.
 *
 * $Id: ftransfer.c 5686 2006-07-03 16:25:03Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/ftransfer", FALSE, _modinit, _moddeinit,
	"$Id: ftransfer.c 5686 2006-07-03 16:25:03Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_ftransfer(char *origin);

command_t cs_ftransfer = { "FTRANSFER", "Forces foundership transfer of a channel.",
                           PRIV_CHAN_ADMIN, cs_cmd_ftransfer };
                                                                                   
list_t *cs_cmdtree;
list_t *cs_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(cs_cmdtree, "chanserv/main", "cs_cmdtree");
	MODULE_USE_SYMBOL(cs_helptree, "chanserv/main", "cs_helptree");

        command_add(&cs_ftransfer, cs_cmdtree);
	help_addentry(cs_helptree, "FTRANSFER", "help/cservice/ftransfer", NULL);
}

void _moddeinit()
{
	command_delete(&cs_ftransfer, cs_cmdtree);
	help_delentry(cs_helptree, "FTRANSFER");
}

static void cs_cmd_ftransfer(char *origin)
{
	myuser_t *tmu;
	mychan_t *mc;
	char *name = strtok(NULL, " ");
	char *newfndr = strtok(NULL, " ");

	if (!name || !newfndr)
	{
		notice(chansvs.nick, origin, STR_INSUFFICIENT_PARAMS, "FTRANSFER");
		notice(chansvs.nick, origin, "Syntax: FTRANSFER <#channel> <newfounder>");
		return;
	}

	if (!(tmu = myuser_find_ext(newfndr)))
	{
		notice(chansvs.nick, origin, "\2%s\2 is not registered.", newfndr);
		return;
	}

	if (!(mc = mychan_find(name)))
	{
		notice(chansvs.nick, origin, "\2%s\2 is not registered.", name);
		return;
	}
	
	if (tmu == mc->founder)
	{
		notice(chansvs.nick, origin, "\2%s\2 is already the founder of \2%s\2.", newfndr, name);
		return;
	}

	/* no maxchans check (intentional -- this is an oper command) */

	snoop("FTRANSFER: %s transferred %s from %s to %s", origin, name, mc->founder->name, newfndr);
	wallops("%s transferred foundership of %s from %s to %s", origin, name, mc->founder->name, newfndr);
	logcommand(chansvs.me, user_find_named(origin), CMDLOG_ADMIN, "%s FTRANSFER from %s to %s", mc->name, mc->founder->name, newfndr);
	notice(chansvs.nick, origin, "Foundership of \2%s\2 has been transferred from \2%s\2 to \2%s\2.",
		name, mc->founder->name, newfndr);

	mc->founder = tmu;
	mc->used = CURRTIME;
	chanacs_change_simple(mc, tmu, NULL, CA_FOUNDER_0, 0, CA_ALL);

	/* delete transfer metadata -- prevents a user from stealing it back */
	metadata_delete(mc, METADATA_CHANNEL, "private:verify:founderchg:newfounder");
	metadata_delete(mc, METADATA_CHANNEL, "private:verify:founderchg:timestamp");
}
