/*
 * Copyright (c) 2005 Robin Burchell, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the ChanServ LIST function.
 *
 * $Id: list.c 8027 2007-04-02 10:47:18Z nenolod $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/list", false, _modinit, _moddeinit,
	"$Id: list.c 8027 2007-04-02 10:47:18Z nenolod $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_list(sourceinfo_t *si, int parc, char *parv[]);

command_t cs_list = { "LIST", N_("Lists channels registered matching a given pattern."), PRIV_CHAN_AUSPEX, 1, cs_cmd_list };

list_t *cs_cmdtree;
list_t *cs_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(cs_cmdtree, "chanserv/main", "cs_cmdtree");
	MODULE_USE_SYMBOL(cs_helptree, "chanserv/main", "cs_helptree");

	command_add(&cs_list, cs_cmdtree);
	help_addentry(cs_helptree, "LIST", "help/cservice/list", NULL);
}

void _moddeinit()
{
	command_delete(&cs_list, cs_cmdtree);
	help_delentry(cs_helptree, "LIST");
}

static void cs_cmd_list(sourceinfo_t *si, int parc, char *parv[])
{
	mychan_t *mc;
	char *chanpattern = parv[0];
	char buf[BUFSIZE];
	unsigned int matches = 0;
	mowgli_patricia_iteration_state_t state;

	if (!chanpattern)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "LIST");
		command_fail(si, fault_needmoreparams, _("Syntax: LIST <channel pattern>"));
		return;
	}

	command_success_nodata(si, _("Channels matching pattern \2%s\2:"), chanpattern);

	MOWGLI_PATRICIA_FOREACH(mc, &state, mclist)
	{
		if (!match(chanpattern, mc->name))
		{
			/* in the future we could add a LIMIT parameter */
			*buf = '\0';

			if (metadata_find(mc, "private:mark:setter")) {
				strlcat(buf, "\2[marked]\2", BUFSIZE);
			}
			if (metadata_find(mc, "private:close:closer")) {
				if (*buf)
					strlcat(buf, " ", BUFSIZE);

				strlcat(buf, "\2[closed]\2", BUFSIZE);
			}
			if (mc->flags & MC_HOLD) {
				if (*buf)
					strlcat(buf, " ", BUFSIZE);

				strlcat(buf, "\2[held]\2", BUFSIZE);
			}

			command_success_nodata(si, "- %s (%s) %s", mc->name, mychan_founder_names(mc), buf);
			matches++;
		}
	}

	logcommand(si, CMDLOG_ADMIN, "LIST: \2%s\2 (%d matches)", chanpattern, matches);
	if (matches == 0)
		command_success_nodata(si, _("No channel matched pattern \2%s\2"), chanpattern);
	else
		command_success_nodata(si, ngettext(N_("\2%d\2 match for pattern \2%s\2"), N_("\2%d\2 matches for pattern \2%s\2"), matches), matches, chanpattern);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
