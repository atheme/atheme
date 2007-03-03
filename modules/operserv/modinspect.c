/*
 * Copyright (c) 2005-2006 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * A simple module inspector.
 *
 * $Id: modinspect.c 7771 2007-03-03 12:46:36Z pippijn $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/modinspect", FALSE, _modinit, _moddeinit,
	"$Id: modinspect.c 7771 2007-03-03 12:46:36Z pippijn $",
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
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "MODINSPECT");
		command_fail(si, fault_needmoreparams, "Syntax: MODINSPECT <module>");
		return;
	}

	logcommand(si, CMDLOG_GET, "MODINSPECT %s", mname);

	m = module_find_published(mname);

	if (!m)
	{
		command_fail(si, fault_nosuch_target, "\2%s\2 is not loaded.", mname);
		return;
	}

	/* Is there a header? */
	if (!m->header)
	{
		command_fail(si, fault_unimplemented, "\2%s\2 cannot be inspected.", mname);
		return;
	}

	command_success_nodata(si, "Information on \2%s\2:", mname);
	command_success_nodata(si, "Name       : %s", m->header->name);
	command_success_nodata(si, "Address    : %p", m->address);
	command_success_nodata(si, "Entry point: %p", m->header->modinit);
	command_success_nodata(si, "Exit point : %p", m->header->deinit);
	command_success_nodata(si, "Version    : %s", m->header->version);
	command_success_nodata(si, "Vendor     : %s", m->header->vendor);
	command_success_nodata(si, "*** \2End of Info\2 ***");
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:noexpandtab
 */
