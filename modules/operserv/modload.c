/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Loads a new module in.
 *
 * $Id: modload.c 5686 2006-07-03 16:25:03Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/modload", FALSE, _modinit, _moddeinit,
	"$Id: modload.c 5686 2006-07-03 16:25:03Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void os_cmd_modload(char *origin);

command_t os_modload = { "MODLOAD", "Loads a module.",
			 PRIV_ADMIN, os_cmd_modload };

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

static void os_cmd_modload(char *origin)
{
	char *module;
	module_t *m;
	char pbuf[BUFSIZE + 1];

	while((module = strtok(NULL, " ")))
	{

	        if (module_find_published(module))
       		{
                notice(opersvs.nick, origin, "\2%s\2 is already loaded.", module);
                return;
       		}

		logcommand(opersvs.me, user_find_named(origin), CMDLOG_ADMIN, "MODLOAD %s", module);
		if (*module != '/')
		{
			snprintf(pbuf, BUFSIZE, "%s/%s", MODDIR "/modules",
				 module);
			m = module_load(pbuf);
		}
		else
			m = module_load(module);

		if (m != NULL)
			notice(opersvs.nick, origin, "Module \2%s\2 loaded.", module);
		else
			notice(opersvs.nick, origin, "Module \2%s\2 failed to load.", module);
	}
}
