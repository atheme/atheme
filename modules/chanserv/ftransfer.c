/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService FTRANSFER function.
 *
 * $Id: ftransfer.c 6337 2006-09-10 15:54:41Z pippijn $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/ftransfer", FALSE, _modinit, _moddeinit,
	"$Id: ftransfer.c 6337 2006-09-10 15:54:41Z pippijn $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_ftransfer(sourceinfo_t *si, int parc, char *parv[]);

command_t cs_ftransfer = { "FTRANSFER", "Forces foundership transfer of a channel.",
                           PRIV_CHAN_ADMIN, 2, cs_cmd_ftransfer };
                                                                                   
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

static void cs_cmd_ftransfer(sourceinfo_t *si, int parc, char *parv[])
{
	myuser_t *tmu;
	mychan_t *mc;
	char *name = parv[0];
	char *newfndr = parv[1];

	if (!name || !newfndr)
	{
		notice(chansvs.nick, si->su->nick, STR_INSUFFICIENT_PARAMS, "FTRANSFER");
		notice(chansvs.nick, si->su->nick, "Syntax: FTRANSFER <#channel> <newfounder>");
		return;
	}

	if (!(tmu = myuser_find_ext(newfndr)))
	{
		notice(chansvs.nick, si->su->nick, "\2%s\2 is not registered.", newfndr);
		return;
	}

	if (!(mc = mychan_find(name)))
	{
		notice(chansvs.nick, si->su->nick, "\2%s\2 is not registered.", name);
		return;
	}
	
	if (tmu == mc->founder)
	{
		notice(chansvs.nick, si->su->nick, "\2%s\2 is already the founder of \2%s\2.", newfndr, name);
		return;
	}

	/* no maxchans check (intentional -- this is an oper command) */

	snoop("FTRANSFER: %s transferred %s from %s to %s", si->su->nick, name, mc->founder->name, newfndr);
	wallops("%s transferred foundership of %s from %s to %s", si->su->nick, name, mc->founder->name, newfndr);
	logcommand(chansvs.me, si->su, CMDLOG_ADMIN, "%s FTRANSFER from %s to %s", mc->name, mc->founder->name, newfndr);
	verbose(mc, "Foundership transfer from \2%s\2 to \2%s\2 forced by %s administration.", mc->founder->name, newfndr, me.netname);
	notice(chansvs.nick, si->su->nick, "Foundership of \2%s\2 has been transferred from \2%s\2 to \2%s\2.",
		name, mc->founder->name, newfndr);

	mc->founder = tmu;
	mc->used = CURRTIME;
	chanacs_change_simple(mc, tmu, NULL, CA_FOUNDER_0, 0, CA_ALL);

	/* delete transfer metadata -- prevents a user from stealing it back */
	metadata_delete(mc, METADATA_CHANNEL, "private:verify:founderchg:newfounder");
	metadata_delete(mc, METADATA_CHANNEL, "private:verify:founderchg:timestamp");
}
