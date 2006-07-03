/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the UserServ LISTCHANS function.
 *   -- Contains an alias "MYACCESS" for legacy users
 *
 * $Id: listchans.c 5686 2006-07-03 16:25:03Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"userserv/listchans", FALSE, _modinit, _moddeinit,
	"$Id: listchans.c 5686 2006-07-03 16:25:03Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void us_cmd_listchans(char *origin);

command_t us_myaccess = { "MYACCESS", "Alias for LISTCHANS", AC_NONE, us_cmd_listchans };
command_t us_listchans = { "LISTCHANS", "Lists channels that you have access to.", AC_NONE, us_cmd_listchans };

list_t *us_cmdtree, *us_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(us_cmdtree, "userserv/main", "us_cmdtree");
	MODULE_USE_SYMBOL(us_helptree, "userserv/main", "us_helptree");
	
	command_add(&us_myaccess, us_cmdtree);
	help_addentry(us_helptree, "MYACCESS", "help/userserv/listchans", NULL);
	
	command_add(&us_listchans, us_cmdtree);
	help_addentry(us_helptree, "LISTCHANS", "help/userserv/listchans", NULL);
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
	user_t *u = user_find_named(origin);
	myuser_t *mu;
	node_t *n;
	chanacs_t *ca;
	uint32_t akicks = 0, i;

	/* Optional target */
	char *target = strtok(NULL, " ");

	if (target)
	{
		if (!has_priv(u, PRIV_USER_AUSPEX))
		{
			notice(usersvs.nick, origin, "You are not authorized to use the target argument.");
			return;
		}

		mu = myuser_find_ext(target);

		if (mu == NULL)
		{
			notice(usersvs.nick, origin, "The account \2%s\2 is not registered.", target);
			return;
		}
	}
	else
	{
		mu = u->myuser;
		if (mu == NULL)
		{
			notice(usersvs.nick, origin, "You are not logged in.");
			return;
		}
	}


	if (mu != u->myuser)
	{	/* must have been an oper */
		snoop("LISTCHANS: \2%s\2 on \2%s\2", u->nick, mu->name);
		logcommand(usersvs.me, u, CMDLOG_ADMIN, "LISTCHANS %s", mu->name);
	}
	else
	{	/* just a user, or oper is listing himself */
		logcommand(usersvs.me, u, CMDLOG_GET, "LISTCHANS");
	}

	if (mu->chanacs.count == 0)
	{
		notice(usersvs.nick, origin, "No channel access was found for the account \2%s\2.", mu->name);
		return;
	}
  
	LIST_FOREACH(n, mu->chanacs.head)
	{
		ca = (chanacs_t *)n->data;

		if (is_founder(ca->mychan, mu))
			notice(usersvs.nick, origin, "Founder of %s", ca->mychan->name);

		switch (ca->level)
		{
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
