/*
 * Copyright (c) 2005 Alex Lambert
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the NickServ MYACCESS function.
 *
 * $Id: myaccess.c 2575 2005-10-05 02:46:11Z alambert $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"userserv/myaccess", FALSE, _modinit, _moddeinit,
	"$Id: myaccess.c 2575 2005-10-05 02:46:11Z alambert $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void us_cmd_myaccess(char *origin);

command_t us_myaccess = { "MYACCESS", "Lists channels that you have access to.", AC_NONE, us_cmd_myaccess };

list_t *us_cmdtree, *us_helptree;

void _modinit(module_t *m)
{
	us_cmdtree = module_locate_symbol("userserv/main", "us_cmdtree");
	us_helptree = module_locate_symbol("userserv/main", "us_helptree");
	command_add(&us_myaccess, us_cmdtree);
	help_addentry(us_helptree, "MYACCESS", "help/userserv/myaccess", NULL);
}

void _moddeinit()
{
	command_delete(&us_myaccess, us_cmdtree);
	help_delentry(us_helptree, "MYACCESS");
}

static void us_cmd_myaccess(char *origin)
{
	user_t *u = user_find(origin);
	myuser_t *mu = u->myuser;
	node_t *n;
	chanacs_t *ca;
	uint32_t i, matches = 0;

	if (mu == NULL)
	{
		notice(usersvs.nick, origin, "You are not logged in.");
		return;
	}

	LIST_FOREACH(n, mu->chanacs.head)
	{
       	        ca = (chanacs_t *)n->data;

		switch (ca->level)
		{
			case CA_VOP:
				notice(usersvs.nick, origin, "VOP in %s", ca->mychan->name);
				matches++;
				break;
			case CA_HOP:
				notice(usersvs.nick, origin, "HOP in %s", ca->mychan->name);
				matches++;
				break;
			case CA_AOP:
				notice(usersvs.nick, origin, "AOP in %s", ca->mychan->name);
				matches++;
				break;
			case CA_SOP:
				notice(usersvs.nick, origin, "SOP in %s", ca->mychan->name);
				matches++;
				break;
			case CA_SUCCESSOR:
				notice(usersvs.nick, origin, "Successor of %s", ca->mychan->name);
				matches++;
				break;
			case CA_FOUNDER:	/* equiv to is_founder() */
				notice(usersvs.nick, origin, "Founder of %s", ca->mychan->name);
				matches++;
				break;
			default:
				/* a user may be AKICKed from a channel he doesn't know about */
				if (!(ca->level & CA_AKICK))
				{
					notice(usersvs.nick, origin, "%s in %s", bitmask_to_flags(ca->level, chanacs_flags), ca->mychan->name);
					matches++;
				}
		}
	}

	if (matches == 0)
		notice(usersvs.nick, origin, "No channel access was found for the account \2%s\2.", mu->name);
	else
		notice(usersvs.nick, origin, "\2%d\2 channel access match%s for the account \2%s\2",
							matches, (matches != 1) ? "es" : "", mu->name);
}
