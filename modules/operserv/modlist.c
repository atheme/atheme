/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Module listing.
 *
 * $Id: modlist.c 6337 2006-09-10 15:54:41Z pippijn $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/modlist", FALSE, _modinit, _moddeinit,
	"$Id: modlist.c 6337 2006-09-10 15:54:41Z pippijn $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void os_cmd_modlist(sourceinfo_t *si, int parc, char *parv[]);

command_t os_modlist = { "MODLIST", "Lists loaded modules.", PRIV_SERVER_AUSPEX, 0, os_cmd_modlist };

list_t *os_cmdtree;
list_t *os_helptree;
extern list_t modules;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(os_cmdtree, "operserv/main", "os_cmdtree");
	MODULE_USE_SYMBOL(os_helptree, "operserv/main", "os_helptree");

	command_add(&os_modlist, os_cmdtree);
	help_addentry(os_helptree, "MODLIST", "help/oservice/modlist", NULL);
}

void _moddeinit()
{
	command_delete(&os_modlist, os_cmdtree);
	help_delentry(os_helptree, "MODLIST");
}

static void os_cmd_modlist(sourceinfo_t *si, int parc, char *parv[])
{
	node_t *n;
	uint16_t i = 0;
	notice(opersvs.nick, si->su->nick, "Loaded modules:");

	LIST_FOREACH(n, modules.head)
	{
		module_t *m = n->data;

		notice(opersvs.nick, si->su->nick, "%2d: %-20s [loaded at 0x%lx]",
			++i, m->header->name, m->address);
	}

	notice(opersvs.nick, si->su->nick, "\2%d\2 modules loaded.", i);
	logcommand(opersvs.me, si->su, CMDLOG_GET, "MODLIST");
}
