/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * A simple module inspector.
 *
 * $Id: modinspect.c 6337 2006-09-10 15:54:41Z pippijn $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/modinspect", FALSE, _modinit, _moddeinit,
	"$Id: modinspect.c 6337 2006-09-10 15:54:41Z pippijn $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void os_cmd_modinspect(sourceinfo_t *si, int parc, char *parv[]);

list_t *os_cmdtree;
list_t *os_helptree;

command_t os_modinspect = { "MODINSPECT", "Displays information about loaded modules.", PRIV_SERVER_AUSPEX, 1, os_cmd_modinspect };

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(os_cmdtree, "operserv/main", "os_cmdtree");
	MODULE_USE_SYMBOL(os_helptree, "operserv/main", "os_helptree");

	command_add(&os_modinspect, os_cmdtree);
	help_addentry(os_helptree, "MODINSPECT", "help/oservice/modinspect", NULL);
}

void _moddeinit(void)
{
	command_delete(&os_modinspect, os_cmdtree);
	help_delentry(os_helptree, "MODINSPECT");
}

static void os_cmd_modinspect(sourceinfo_t *si, int parc, char *parv[])
{
	char *mname = parv[0];
	module_t *m;

	if (!mname)
	{
		notice(opersvs.nick, si->su->nick, STR_INSUFFICIENT_PARAMS, "MODINSPECT");
		notice(opersvs.nick, si->su->nick, "Syntax: MODINSPECT <module>");
		return;
	}

	logcommand(opersvs.me, si->su, CMDLOG_GET, "MODINSPECT %s", mname);

	m = module_find_published(mname);

	if (!m)
	{
		notice(opersvs.nick, si->su->nick, "\2%s\2 is not loaded.", mname);
		return;
	}

	/* Is there a header? */
	if (!m->header)
	{
		notice(opersvs.nick, si->su->nick, "\2%s\2 cannot be inspected.", mname);
		return;
	}

	notice(opersvs.nick, si->su->nick, "Information on \2%s\2:", mname);
	notice(opersvs.nick, si->su->nick, "Name       : %s", m->header->name);
	notice(opersvs.nick, si->su->nick, "Address    : %p", m->address);
	notice(opersvs.nick, si->su->nick, "Entry point: %p", m->header->modinit);
	notice(opersvs.nick, si->su->nick, "Exit point : %p", m->header->deinit);
	notice(opersvs.nick, si->su->nick, "Version    : %s", m->header->version);
	notice(opersvs.nick, si->su->nick, "Vendor     : %s", m->header->vendor);
	notice(opersvs.nick, si->su->nick, "*** \2End of Info\2 ***");
}
