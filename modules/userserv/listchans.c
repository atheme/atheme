/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the UserServ LISTCHANS function.
 *   -- Contains an alias "MYACCESS" for legacy users
 *
 * $Id: myaccess.c 2575 2005-10-05 02:46:11Z alambert $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"userserv/listchans", FALSE, _modinit, _moddeinit,
	"$Id: myaccess.c 2575 2005-10-05 02:46:11Z alambert $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void us_cmd_listchans(char *origin);

command_t us_myaccess = { "MYACCESS", "Alias for LISTCHANS", AC_NONE, us_cmd_listchans };
command_t us_listchans = { "LISTCHANS", "Lists channels that you have access to.", AC_NONE, us_cmd_listchans };

list_t *us_cmdtree, *us_helptree;

void _modinit(module_t *m)
{
	us_cmdtree = module_locate_symbol("userserv/main", "us_cmdtree");
	us_helptree = module_locate_symbol("userserv/main", "us_helptree");
	
	command_add(&us_myaccess, us_cmdtree);
	help_addentry(us_helptree, "MYACCESS", "help/userserv/myaccess", NULL);
	
	command_add(&us_listchans, us_cmdtree);
	help_addentry(us_helptree, "LISTCHANS", "help/userserv/myaccess", NULL);
}

void _moddeinit()
{
	command_delete(&us_myaccess, us_cmdtree);
	help_delentry(us_helptree, "MYACCESS");
	
  command_delete(&us_listchans, us_cmdtree);
	help_delentry(us_helptree, "LISTCHANS");
}

static void us_cmd_listchans(char *origin)
{
	user_t *u = user_find(origin);
	myuser_t *mu = u->myuser;
	node_t *n;
	chanacs_t *ca;
	uint32_t akicks = 0, i;

	/* Optional target */
	char *target = strtok(NULL, " ");

	if (mu == NULL)
	{
		notice(usersvs.nick, origin, "You are not logged in.");
		return;
	}

	if (target != NULL && is_sra(mu))
	{
		if (!(mu = myuser_find(target)))
		{
  	              notice(usersvs.nick, origin, "\2Account %s is not registerd\2.", target);
  	              return;
		}

                /* snoop if not calling on themselves */
                if (strcasecmp(mu->name,target) != 0)
                        snoop("LISTCHANS: \2%s\2 on  \2%s\2", u->myuser->name, target);
	}
	
  if (mu->chanacs.count == 0)
  {
  		notice(usersvs.nick, origin, "No channel access was found for the account \2%s\2.", mu->name);
  		return;
  }  
  
	LIST_FOREACH(n, mu->chanacs.head)
	{
    		ca = (chanacs_t *)n->data;

		switch (ca->level)
		{
			case CA_VOP:
				notice(usersvs.nick, origin, "VOP in %s", ca->mychan->name);
				break;
			case CA_HOP:
				notice(usersvs.nick, origin, "HOP in %s", ca->mychan->name);
				break;
			case CA_AOP:
				notice(usersvs.nick, origin, "AOP in %s", ca->mychan->name);
				break;
			case CA_SOP:
				notice(usersvs.nick, origin, "SOP in %s", ca->mychan->name);
				break;
			case CA_SUCCESSOR:
				notice(usersvs.nick, origin, "Successor of %s", ca->mychan->name);
				break;
			case CA_FOUNDER:	/* equiv to is_founder() */
				notice(usersvs.nick, origin, "Founder of %s", ca->mychan->name);
				break;
			default:
				/* don't tell users they're akicked (flag +b) */
				if (!(ca->level & CA_AKICK))
					notice(usersvs.nick, origin, "Access flag(s) %s in %s", bitmask_to_flags(ca->level, chanacs_flags), ca->mychan->name);
				else
					akicks++;	
		}
	}

    i = mu->chanacs.count - akicks;

    if (i == 0)
        notice(usersvs.nick, origin, "No channel access was found for the account \2%s\2.", mu->name);
    else
	notice(usersvs.nick, origin, "\2%d\2 channel access match%s for the account \2%s\2",
							i, (akicks > 1) ? "es" : "", mu->name);
}
