/*
 * Copyright (c) 2005 Alex Lambert
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the NickServ MYACCESS function.
 *
 * $Id: myaccess.c 2563 2005-10-04 07:09:30Z pfish $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"nickserv/myaccess", FALSE, _modinit, _moddeinit,
	"$Id: myaccess.c 2563 2005-10-04 07:09:30Z pfish $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void ns_cmd_myaccess(char *origin);

command_t ns_myaccess = { "MYACCESS", "Lists channels that you have access to.", AC_NONE, ns_cmd_myaccess };

list_t *ns_cmdtree, *ns_helptree;

void _modinit(module_t *m)
{
	ns_cmdtree = module_locate_symbol("nickserv/main", "ns_cmdtree");
	ns_helptree = module_locate_symbol("nickserv/main", "ns_helptree");
	command_add(&ns_myaccess, ns_cmdtree);
	help_addentry(ns_helptree, "MYACCESS", "help/nickserv/myaccess", NULL);
}

void _moddeinit()
{
	command_delete(&ns_myaccess, ns_cmdtree);
	help_delentry(ns_helptree, "MYACCESS");
}

static void ns_cmd_myaccess(char *origin)
{
	user_t *u = user_find(origin);
	myuser_t *mu = u->myuser;
	node_t *n;
	chanacs_t *ca;
	uint32_t i, matches = 0;

	if (mu == NULL)
	{
		notice(nicksvs.nick, origin, "You are not logged in.");
		return;
	}

	LIST_FOREACH(n, mu->chanacs.head)
	{
       	        ca = (chanacs_t *)n->data;

		switch (ca->level)
		{
			case CA_VOP:
				notice(nicksvs.nick, origin, "VOP in %s", ca->mychan->name);
				matches++;
				break;
			case CA_HOP:
				notice(nicksvs.nick, origin, "HOP in %s", ca->mychan->name);
				matches++;
				break;
			case CA_AOP:
				notice(nicksvs.nick, origin, "AOP in %s", ca->mychan->name);
				matches++;
				break;
			case CA_SOP:
				notice(nicksvs.nick, origin, "SOP in %s", ca->mychan->name);
				matches++;
				break;
			case CA_SUCCESSOR:
				notice(nicksvs.nick, origin, "Successor of %s", ca->mychan->name);
				matches++;
				break;
			case CA_FOUNDER:	/* equiv to is_founder() */
				notice(nicksvs.nick, origin, "Founder of %s", ca->mychan->name);
				matches++;
				break;
			default:
				/* a user may be AKICKed from a channel he doesn't know about */
				if (!(ca->level & CA_AKICK))
				{
					notice(nicksvs.nick, origin, "%s in %s", bitmask_to_flags(ca->level, chanacs_flags), ca->mychan->name);
					matches++;
				}
		}
	}

	if (matches == 0)
		notice(nicksvs.nick, origin, "No channel access was found for the nickname \2%s\2.", mu->name);
	else
		notice(nicksvs.nick, origin, "\2%d\2 channel access match%s for the nickname \2%s\2",
							matches, (matches != 1) ? "es" : "", mu->name);
}
