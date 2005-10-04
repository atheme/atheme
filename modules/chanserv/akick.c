/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService AKICK functions.
 *
 * $Id: akick.c 2551 2005-10-04 06:14:07Z nenolod $
 */

#include "atheme.h"

static void cs_cmd_akick(char *origin);
static void cs_fcmd_akick(char *origin, char *chan);

DECLARE_MODULE_V1
(
	"chanserv/akick", FALSE, _modinit, _moddeinit,
	"$Id: akick.c 2551 2005-10-04 06:14:07Z nenolod $",
	"Atheme Development Group <http://www.atheme.org>"
);

command_t cs_akick = { "AKICK", "Manipulates a channel's AKICK list.",
                        AC_NONE, cs_cmd_akick };
fcommand_t fc_akick = { "!akick", AC_NONE, cs_fcmd_akick };

list_t *cs_cmdtree;
list_t *cs_fcmdtree;
list_t *cs_helptree;

void _modinit(module_t *m)
{
	cs_cmdtree = module_locate_symbol("chanserv/main", "cs_cmdtree");
	cs_fcmdtree = module_locate_symbol("chanserv/main", "cs_fcmdtree");
	cs_helptree = module_locate_symbol("chanserv/main", "cs_helptree");

        command_add(&cs_akick, cs_cmdtree);
	fcommand_add(&fc_akick, cs_fcmdtree);

	help_addentry(cs_helptree, "AKICK", "help/cservice/akick", NULL);
}

void _moddeinit()
{
	command_delete(&cs_akick, cs_cmdtree);
	fcommand_delete(&fc_akick, cs_fcmdtree);
	help_delentry(cs_helptree, "AKICK");
}

void cs_cmd_akick(char *origin)
{
	user_t *u = user_find(origin);
	myuser_t *mu;
	mychan_t *mc;
	chanacs_t *ca, *ca2;
	node_t *n;
	char *chan = strtok(NULL, " ");
	char *cmd = strtok(NULL, " ");
	char *uname = strtok(NULL, " ");

	if (!cmd || !chan)
	{
		notice(chansvs.nick, origin, "Insufficient parameters specified for \2AKICK\2.");
		notice(chansvs.nick, origin, "Syntax: AKICK <#channel> ADD|DEL|LIST <nickname|hostmask>");
		return;
	}

	if ((strcasecmp("LIST", cmd)) && (!uname))
	{
		notice(chansvs.nick, origin, "Insufficient parameters specified for \2AKICK\2.");
		notice(chansvs.nick, origin, "Syntax: AKICK <#channel> ADD|DEL|LIST <nickname|hostmask>");
		return;
	}

	/* make sure they're registered, logged in
	 * and the founder of the channel before
	 * we go any further.
	 */
	if (!u->myuser)
	{
		notice(chansvs.nick, origin, "You are not logged in.");
		return;
	}

	mc = mychan_find(chan);
	if (!mc)
	{
		notice(chansvs.nick, origin, "\2%s\2 is not registered.", chan);
		return;
	}

	/* ADD */
	if (!strcasecmp("ADD", cmd))
	{
		if (!is_xop(mc, u->myuser, CA_FLAGS))
		{
			notice(chansvs.nick, origin, "You are not authorized to perform this operation.");
			return;
		}

		mu = myuser_find(uname);
		if (!mu)
		{
			/* we might be adding a hostmask */
			if (!validhostmask(uname))
			{
				notice(chansvs.nick, origin, "\2%s\2 is neither a nickname nor a hostmask.", uname);
				return;
			}

			if (chanacs_find_host(mc, uname, CA_AKICK))
			{
				notice(chansvs.nick, origin, "\2%s\2 is already on the AKICK list for \2%s\2", uname, mc->name);
				return;
			}

			uname = collapse(uname);

			ca2 = chanacs_add_host(mc, uname, CA_AKICK);

			hook_call_event("channel_akick_add", ca2);

			verbose(mc, "\2%s\2 added \2%s\2 to the AKICK list.", u->nick, uname);

			notice(chansvs.nick, origin, "\2%s\2 has been added to the AKICK list for \2%s\2.", uname, mc->name);

			return;
		}
		else
		{
			if ((ca = chanacs_find(mc, mu, 0x0)))
			{
				ca->level = CA_AKICK;
				notice(chansvs.nick, origin, "\2%s\2 has been added to the AKICK list for \2%s\2.", mu->name, mc->name);
				verbose(mc, "\2%s\2 added \2%s\2 to the AKICK list.", u->nick, mu->name);
				return;
			}

			ca2 = chanacs_add(mc, mu, CA_AKICK);

			hook_call_event("channel_akick_add", ca2);

			notice(chansvs.nick, origin, "\2%s\2 has been added to the AKICK list for \2%s\2.", mu->name, mc->name);

			verbose(mc, "\2%s\2 added \2%s\2 to the AKICK list.", u->nick, mu->name);

			return;
		}
	}
	else if (!strcasecmp("DEL", cmd))
	{
		if (!is_xop(mc, u->myuser, CA_FLAGS))
		{
			notice(chansvs.nick, origin, "You are not authorized to perform this operation.");
			return;
		}

		mu = myuser_find(uname);
		if (!mu)
		{
			/* we might be deleting a hostmask */
			if (!validhostmask(uname))
			{
				notice(chansvs.nick, origin, "\2%s\2 is neither a nickname nor a hostmask.", uname);
				return;
			}

			if (!chanacs_find_host(mc, uname, CA_AKICK))
			{
				notice(chansvs.nick, origin, "\2%s\2 is not on the AKICK list for \2%s\2.", uname, mc->name);
				return;
			}

			chanacs_delete_host(mc, uname, CA_AKICK);

			verbose(mc, "\2%s\2 removed \2%s\2 from the AKICK list.", u->nick, uname);

			notice(chansvs.nick, origin, "\2%s\2 has been removed from the AKICK list for \2%s\2.", uname, mc->name);

			return;
		}

		if (!(ca = chanacs_find(mc, mu, CA_AKICK)))
		{
			notice(chansvs.nick, origin, "\2%s\2 is not on the AKICK list for \2%s\2.", mu->name, mc->name);
			return;
		}

		chanacs_delete(mc, mu, CA_AKICK);

		notice(chansvs.nick, origin, "\2%s\2 has been removed from the AKICK list for \2%s\2.", mu->name, mc->name);

		verbose(mc, "\2%s\2 removed \2%s\2 from the AKICK list.", u->nick, mu->name);

		return;
	}
	else if (!strcasecmp("LIST", cmd))
	{
		uint8_t i = 0;

		if (!is_xop(mc, u->myuser, CA_ACLVIEW))
		{
			notice(chansvs.nick, origin, "You are not authorized to perform this operation.");
			return;
		}
		notice(chansvs.nick, origin, "AKICK list for \2%s\2:", mc->name);

		LIST_FOREACH(n, mc->chanacs.head)
		{
			ca = (chanacs_t *)n->data;

			if (ca->level == CA_AKICK)
			{
				if (ca->myuser == NULL)
					notice(chansvs.nick, origin, "%d: \2%s\2", ++i, ca->host);

				else if (LIST_LENGTH(&ca->myuser->logins) > 0)
					notice(chansvs.nick, origin, "%d: \2%s\2 (logged in)", ++i, ca->myuser->name);
				else
					notice(chansvs.nick, origin, "%d: \2%s\2 (not logged in)", ++i, ca->myuser->name);
			}

		}

		notice(chansvs.nick, origin, "Total of \2%d\2 %s in \2%s\2's AKICK list.", i, (i == 1) ? "entry" : "entries", mc->name);
	}
}

/* !akick add *!*@*.aol.com */
void cs_fcmd_akick(char *origin, char *chan)
{
	user_t *u = user_find(origin);
	myuser_t *mu;
	mychan_t *mc;
	chanacs_t *ca, *ca2;
	node_t *n;
	char *cmd = strtok(NULL, " ");
	char *uname = strtok(NULL, " ");

	if (!cmd || !chan)
	{
		notice(chansvs.nick, origin, "Insufficient parameters specified for \2AKICK\2.");
		notice(chansvs.nick, origin, "Syntax: AKICK <#channel> ADD|DEL|LIST <nickname|hostmask>");
		return;
	}

	if ((strcasecmp("LIST", cmd)) && (!uname))
	{
		notice(chansvs.nick, origin, "Insufficient parameters specified for \2!AKICK\2.");
		notice(chansvs.nick, origin, "Syntax: !AKICK ADD|DEL|LIST <nickname|hostmask>");
		return;
	}

	/* make sure they're registered, logged in
	 * and the founder of the channel before
	 * we go any further.
	 */
	if (!u->myuser)
	{
		notice(chansvs.nick, origin, "You are not logged in.");
		return;
	}

	mc = mychan_find(chan);
	if (!mc)
	{
		notice(chansvs.nick, origin, "\2%s\2 is not registered.", chan);
		return;
	}

	/* ADD */
	if (!strcasecmp("ADD", cmd))
	{
		if (!is_xop(mc, u->myuser, CA_FLAGS))
		{
			notice(chansvs.nick, origin, "You are not authorized to perform this operation.");
			return;
		}

		mu = myuser_find(uname);
		if (!mu)
		{
			/* we might be adding a hostmask */
			if (!validhostmask(uname))
			{
				notice(chansvs.nick, origin, "\2%s\2 is neither a nickname nor a hostmask.", uname);
				return;
			}

			if (chanacs_find_host(mc, uname, CA_AKICK))
			{
				notice(chansvs.nick, origin, "\2%s\2 is already on the AKICK list for \2%s\2", uname, mc->name);
				return;
			}

			uname = collapse(uname);

			ca2 = chanacs_add_host(mc, uname, CA_AKICK);

			hook_call_event("channel_akick_add", ca2);

			verbose(mc, "\2%s\2 added \2%s\2 to the AKICK list.", u->nick, uname);

			notice(chansvs.nick, origin, "\2%s\2 has been added to the AKICK list for \2%s\2.", uname, mc->name);

			return;
		}
		else
		{
			if ((ca = chanacs_find(mc, mu, 0x0)))
			{
				ca->level = CA_AKICK;
				notice(chansvs.nick, origin, "\2%s\2 has been added to the AKICK list for \2%s\2.", mu->name, mc->name);
				verbose(mc, "\2%s\2 added \2%s\2 to the AKICK list.", u->nick, mu->name);
				return;
			}

			ca2 = chanacs_add(mc, mu, CA_AKICK);

			hook_call_event("channel_akick_add", ca2);

			notice(chansvs.nick, origin, "\2%s\2 has been added to the AKICK list for \2%s\2.", mu->name, mc->name);

			verbose(mc, "\2%s\2 added \2%s\2 to the AKICK list.", u->nick, mu->name);

			return;
		}
	}
	else if (!strcasecmp("DEL", cmd))
	{
		if (!is_xop(mc, u->myuser, CA_FLAGS))
		{
			notice(chansvs.nick, origin, "You are not authorized to perform this operation.");
			return;
		}

		mu = myuser_find(uname);
		if (!mu)
		{
			/* we might be deleting a hostmask */
			if (!validhostmask(uname))
			{
				notice(chansvs.nick, origin, "\2%s\2 is neither a nickname nor a hostmask.", uname);
				return;
			}

			if (!chanacs_find_host(mc, uname, CA_AKICK))
			{
				notice(chansvs.nick, origin, "\2%s\2 is not on the AKICK list for \2%s\2.", uname, mc->name);
				return;
			}

			chanacs_delete_host(mc, uname, CA_AKICK);

			verbose(mc, "\2%s\2 removed \2%s\2 from the AKICK list.", u->nick, uname);

			notice(chansvs.nick, origin, "\2%s\2 has been removed from the AKICK list for \2%s\2.", uname, mc->name);

			return;
		}

		if (!(ca = chanacs_find(mc, mu, CA_AKICK)))
		{
			notice(chansvs.nick, origin, "\2%s\2 is not on the AKICK list for \2%s\2.", mu->name, mc->name);
			return;
		}

		chanacs_delete(mc, mu, CA_AKICK);

		notice(chansvs.nick, origin, "\2%s\2 has been removed from the AKICK list for \2%s\2.", mu->name, mc->name);

		verbose(mc, "\2%s\2 removed \2%s\2 from the AKICK list.", u->nick, mu->name);

		return;
	}
	else if (!strcasecmp("LIST", cmd))
	{
		uint8_t i = 0;

		if (!is_xop(mc, u->myuser, CA_ACLVIEW))
		{
			notice(chansvs.nick, origin, "You are not authorized to perform this operation.");
			return;
		}
		notice(chansvs.nick, origin, "AKICK list for \2%s\2:", mc->name);

		LIST_FOREACH(n, mc->chanacs.head)
		{
			ca = (chanacs_t *)n->data;

			if (ca->level == CA_AKICK)
			{
				if (ca->myuser == NULL)
					notice(chansvs.nick, origin, "%d: \2%s\2", ++i, ca->host);

				else if (LIST_LENGTH(&ca->myuser->logins) > 0)
					notice(chansvs.nick, origin, "%d: \2%s\2 (logged in)", ++i, ca->myuser->name);
				else
					notice(chansvs.nick, origin, "%d: \2%s\2 (not logged in)", ++i, ca->myuser->name);
			}

		}

		notice(chansvs.nick, origin, "Total of \2%d\2 %s in \2%s\2's AKICK list.", i, (i == 1) ? "entry" : "entries", mc->name);
	}
}
