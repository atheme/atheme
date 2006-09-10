/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Loads a new module in.
 *
 * $Id: modload.c 6337 2006-09-10 15:54:41Z pippijn $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/modload", FALSE, _modinit, _moddeinit,
	"$Id: modload.c 6337 2006-09-10 15:54:41Z pippijn $",
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
	unsigned int i;

	if (parc < 1)
	{
		notice(opersvs.nick, si->su->nick, STR_INSUFFICIENT_PARAMS, "MODLOAD");
		notice(opersvs.nick, si->su->nick, "Syntax: MODLOAD <module...>");
		return;
	}
	i = 0;
	while (i < parc)
	{
		module = parv[i++];
		if (module_find_published(module))
		{
			notice(opersvs.nick, si->su->nick, "\2%s\2 is already loaded.", module);
			continue;
		}

		logcommand(opersvs.me, si->su, CMDLOG_ADMIN, "MODLOAD %s", module);
		if (*module != '/')
		{
			snprintf(pbuf, BUFSIZE, "%s/%s", MODDIR "/modules",
					module);
			m = module_load(pbuf);
		}
		else
			m = module_load(module);

		if (m != NULL)
			notice(opersvs.nick, si->su->nick, "Module \2%s\2 loaded.", module);
		else
			notice(opersvs.nick, si->su->nick, "Module \2%s\2 failed to load.", module);
	}
}
