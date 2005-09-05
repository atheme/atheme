/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * A simple module inspector.
 *
 * $Id$
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/modinspect", FALSE, _modinit, _moddeinit,
	"$Id$",
	"Atheme Development Group <http://www.atheme.org>"
);

static void os_cmd_modinspect(char *origin);

list_t *os_cmdtree;

command_t os_modinspect = { "MODINSPECT", "Displays information about loaded modules.", AC_IRCOP, os_cmd_modinspect };

void _modinit(module_t *m)
{
	os_cmdtree = module_locate_symbol("operserv/main", "os_cmdtree");
	command_add(&os_modinspect, os_cmdtree);
}

void _moddeinit(void)
{
	command_delete(&os_modinspect, os_cmdtree);
}

static void os_cmd_modinspect(char *origin)
{
	char *mname = strtok(NULL, " ");
	module_t *m;

	if (!mname)
	{
		notice(opersvs.nick, origin, "Insufficient parameters for \2MODINSPECT\2.");
		notice(opersvs.nick, origin, "Syntax: MODINSPECT <module>");
		return;
	}

	m = module_find_published(mname);

	if (!m)
	{
		notice(opersvs.nick, origin, "\2%s\2 is not loaded.");
		return;
	}

	/* Is there a header? */
	if (!m->header)
	{
		notice(opersvs.nick, origin, "\2%s\2 cannot be inspected.");
		return;
	}

	notice(opersvs.nick, origin, "Information on \2%s\2:");
	notice(opersvs.nick, origin, "Name       : %s", m->header->name);
	notice(opersvs.nick, origin, "Address    : %p", m->address);
	notice(opersvs.nick, origin, "Entry point: %p", m->header->modinit);
	notice(opersvs.nick, origin, "Exit point : %p", m->header->deinit);
	notice(opersvs.nick, origin, "Version    : %s", m->header->version);
	notice(opersvs.nick, origin, "Vendor     : %s", m->header->vendor);
	notice(opersvs.nick, origin, "*** \2End of Info\2 ***");
}
