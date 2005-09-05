/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService FTRANSFER function.
 *
 * $Id: ftransfer.c 2127 2005-09-05 00:44:16Z nenolod $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/ftransfer", FALSE, _modinit, _moddeinit,
	"$Id: ftransfer.c 2127 2005-09-05 00:44:16Z nenolod $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_ftransfer(char *origin);

command_t cs_ftransfer = { "FTRANSFER", "Forces foundership transfer of a channel.",
                           AC_IRCOP, cs_cmd_ftransfer };
                                                                                   
list_t *cs_cmdtree;

void _modinit(module_t *m)
{
	cs_cmdtree = module_locate_symbol("chanserv/main", "cs_cmdtree");
        command_add(&cs_ftransfer, cs_cmdtree);
}

void _moddeinit()
{
	command_delete(&cs_ftransfer, cs_cmdtree);
}

static void cs_cmd_ftransfer(char *origin)
{
	myuser_t *tmu;
	mychan_t *mc;
	char *name = strtok(NULL, " ");
	char *newfndr = strtok(NULL, " ");

	if (!name || !newfndr)
	{
		notice(chansvs.nick, origin, "Insufficient parameters specified for \2FTRANSFER\2.");
		notice(chansvs.nick, origin, "Syntax: FTRANSFER <#channel> <newfounder>");
		return;
	}

	if (!(tmu = myuser_find(newfndr)))
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
	notice(chansvs.nick, origin, "Foundership of \2%s\2 has been transferred from \2%s\2 to \2%s\2.",
		name, mc->founder->name, newfndr);

	/* from cs_set_founder */
	chanacs_delete(mc, mc->founder, CA_FOUNDER);
	chanacs_delete(mc, tmu, CA_VOP);
	chanacs_delete(mc, tmu, CA_HOP);
	chanacs_delete(mc, tmu, CA_AOP);
	chanacs_delete(mc, tmu, CA_SOP);
	mc->founder = tmu;
	mc->used = CURRTIME;
	chanacs_add(mc, tmu, CA_FOUNDER);

	/* delete transfer metadata -- prevents a user from stealing it back */
	metadata_delete(mc, METADATA_CHANNEL, "private:verify:founderchg:newfounder");
	metadata_delete(mc, METADATA_CHANNEL, "private:verify:founderchg:timestamp");
}
