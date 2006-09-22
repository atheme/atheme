/*
 * Copyright (c) 2005 Robin Burchell, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the ChanServ LIST function.
 *
 * $Id: list.c 6427 2006-09-22 19:38:34Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/list", FALSE, _modinit, _moddeinit,
	"$Id: list.c 6427 2006-09-22 19:38:34Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_list(sourceinfo_t *si, int parc, char *parv[]);

command_t cs_list = { "LIST", "Lists channels registered matching a given pattern.", PRIV_CHAN_AUSPEX, 1, cs_cmd_list };

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
	node_t *n;
	char *chanpattern = parv[0];
	char buf[BUFSIZE];
	uint32_t i;
	uint32_t matches = 0;

	if (!chanpattern)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "LIST");
		command_fail(si, fault_needmoreparams, "Syntax: LIST <channel pattern>");
		return;
	}

	snoop("LIST:CHANNELS: \2%s\2 by \2%s\2", chanpattern, si->su->nick);
	command_success_nodata(si, "Channels matching pattern \2%s\2:", chanpattern);

	for (i = 0; i < HASHSIZE; i++)
	{
		LIST_FOREACH(n, mclist[i].head)
		{
			mc = (mychan_t *)n->data;

			if (!match(chanpattern, mc->name))
			{
				/* in the future we could add a LIMIT parameter */
				*buf = '\0';

				if (metadata_find(mc, METADATA_CHANNEL, "private:mark:setter")) {
					strlcat(buf, "\2[marked]\2", BUFSIZE);
				}
				if (metadata_find(mc, METADATA_CHANNEL, "private:close:closer")) {
					if (*buf)
						strlcat(buf, " ", BUFSIZE);

					strlcat(buf, "\2[closed]\2", BUFSIZE);
				}

				command_success_nodata(si, "- %s (%s) %s", mc->name, mc->founder->name, buf);
				matches++;
			}
		}
	}

	logcommand(chansvs.me, si->su, CMDLOG_ADMIN, "LIST %s (%d matches)", chanpattern, matches);
	if (matches == 0)
		command_success_nodata(si, "No channel matched pattern \2%s\2", chanpattern);
	else
		command_success_nodata(si, "\2%d\2 match%s for pattern \2%s\2", matches, matches != 1 ? "es" : "", chanpattern);
}
