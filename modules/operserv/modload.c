/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Loads a new module in.
 *
 * $Id: modload.c 6547 2006-09-29 16:39:38Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/modload", FALSE, _modinit, _moddeinit,
	"$Id: modload.c 6547 2006-09-29 16:39:38Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void os_cmd_modload(sourceinfo_t *si, int parc, char *parv[]);

command_t os_modload = { "MODLOAD", "Loads a module.", PRIV_ADMIN, 20, os_cmd_modload };

list_t *os_cmdtree;
list_t *os_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(os_cmdtree, "operserv/main", "os_cmdtree");	
	MODULE_USE_SYMBOL(os_helptree, "operserv/main", "os_helptree");	

	command_add(&os_modload, os_cmdtree);
	help_addentry(os_helptree, "MODLOAD", "help/oservice/modload", NULL);
}

void _moddeinit()
{
	command_delete(&os_modload, os_cmdtree);
	help_delentry(os_helptree, "MODLOAD");
}

static void os_cmd_modload(sourceinfo_t *si, int parc, char *parv[])
{
	char *module;
	module_t *m;
	char pbuf[BUFSIZE + 1];
	int i;

	if (parc < 1)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "MODLOAD");
		command_fail(si, fault_needmoreparams, "Syntax: MODLOAD <module...>");
		return;
	}
	i = 0;
	while (i < parc)
	{
		module = parv[i++];
		if (module_find_published(module))
		{
			command_fail(si, fault_nochange, "\2%s\2 is already loaded.", module);
			continue;
		}

		logcommand(si, CMDLOG_ADMIN, "MODLOAD %s", module);
		if (*module != '/')
		{
			snprintf(pbuf, BUFSIZE, "%s/%s", MODDIR "/modules",
					module);
			m = module_load(pbuf);
		}
		else
			m = module_load(module);

		if (m != NULL)
			command_success_nodata(si, "Module \2%s\2 loaded.", module);
		else
			command_fail(si, fault_nosuch_target, "Module \2%s\2 failed to load.", module);
	}
}
