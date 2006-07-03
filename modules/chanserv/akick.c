/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService AKICK functions.
 *
 * $Id: akick.c 5686 2006-07-03 16:25:03Z jilles $
 */

#include "atheme.h"

static void cs_cmd_akick(char *origin);
static void cs_fcmd_akick(char *origin, char *chan);

DECLARE_MODULE_V1
(
	"chanserv/akick", FALSE, _modinit, _moddeinit,
	"$Id: akick.c 5686 2006-07-03 16:25:03Z jilles $",
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
	MODULE_USE_SYMBOL(cs_cmdtree, "chanserv/main", "cs_cmdtree");
	MODULE_USE_SYMBOL(cs_fcmdtree, "chanserv/main", "cs_fcmdtree");
	MODULE_USE_SYMBOL(cs_helptree, "chanserv/main", "cs_helptree");

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
	user_t *u = user_find_named(origin);
	myuser_t *mu;
	mychan_t *mc;
	chanacs_t *ca, *ca2;
	node_t *n;
	int operoverride = 0;
	char *chan = strtok(NULL, " ");
	char *cmd = strtok(NULL, " ");
	char *uname = strtok(NULL, " ");

	if (!cmd || !chan)
	{
		notice(chansvs.nick, origin, STR_INSUFFICIENT_PARAMS, "AKICK");
		notice(chansvs.nick, origin, "Syntax: AKICK <#channel> ADD|DEL|LIST <nickname|hostmask>");
		return;
	}

	if ((strcasecmp("LIST", cmd)) && (!uname))
	{
		notice(chansvs.nick, origin, STR_INSUFFICIENT_PARAMS, "AKICK");
		notice(chansvs.nick, origin, "Syntax: AKICK <#channel> ADD|DEL|LIST <nickname|hostmask>");
		return;
	}

	/* make sure they're registered, logged in
	 * and the founder of the channel before
	 * we go any further.
	 */
	if (!u->myuser)
	{
		/* if they're opers and just want to LIST, they don't have to log in */
		if (!(has_priv(u, PRIV_CHAN_AUSPEX) && !strcasecmp("LIST", cmd)))
		{
			notice(chansvs.nick, origin, "You are not logged in.");
			return;
		}
	}

	mc = mychan_find(chan);
	if (!mc)
	{
		notice(chansvs.nick, origin, "\2%s\2 is not registered.", chan);
		return;
	}
	
	if (metadata_find(mc, METADATA_CHANNEL, "private:close:closer"))
	{
		notice(chansvs.nick, origin, "\2%s\2 is closed.", chan);
		return;
	}

	/* ADD */
	if (!strcasecmp("ADD", cmd))
	{
		if ((chanacs_user_flags(mc, u) & (CA_FLAGS | CA_REMOVE)) != (CA_FLAGS | CA_REMOVE))
		{
			notice(chansvs.nick, origin, "You are not authorized to perform this operation.");
			return;
		}

		mu = myuser_find_ext(uname);
		if (!mu)
		{
			/* we might be adding a hostmask */
			if (!validhostmask(uname))
			{
				notice(chansvs.nick, origin, "\2%s\2 is neither a nickname nor a hostmask.", uname);
				return;
			}

			uname = collapse(uname);

			ca = chanacs_find_host_literal(mc, uname, 0);
			if (ca != NULL)
			{
				if (ca->level & CA_AKICK)
					notice(chansvs.nick, origin, "\2%s\2 is already on the AKICK list for \2%s\2", uname, mc->name);
				else
					notice(chansvs.nick, origin, "\2%s\2 already has flags \2%s\2 on \2%s\2", uname, bitmask_to_flags(ca->level, chanacs_flags), mc->name);
				return;
			}

			ca = chanacs_find_host(mc, uname, CA_AKICK);
			if (ca != NULL)
			{
				notice(chansvs.nick, origin, "The more general mask \2%s\2 is already on the AKICK list for \2%s\2", ca->host, mc->name);
				return;
			}

			/* ok to use chanacs_add_host() here, already
			 * verified it doesn't exist yet */
			ca2 = chanacs_add_host(mc, uname, CA_AKICK);

			hook_call_event("channel_akick_add", ca2);

			verbose(mc, "\2%s\2 added \2%s\2 to the AKICK list.", u->nick, uname);
			logcommand(chansvs.me, u, CMDLOG_SET, "%s AKICK ADD %s", mc->name, uname);

			notice(chansvs.nick, origin, "\2%s\2 has been added to the AKICK list for \2%s\2.", uname, mc->name);

			return;
		}
		else
		{
			if ((ca = chanacs_find(mc, mu, 0x0)))
			{
				if (ca->level & CA_AKICK)
					notice(chansvs.nick, origin, "\2%s\2 is already on the AKICK list for \2%s\2", mu->name, mc->name);
				else
					notice(chansvs.nick, origin, "\2%s\2 already has flags \2%s\2 on \2%s\2", mu->name, bitmask_to_flags(ca->level, chanacs_flags), mc->name);
				return;
			}

			ca2 = chanacs_add(mc, mu, CA_AKICK);

			hook_call_event("channel_akick_add", ca2);

			notice(chansvs.nick, origin, "\2%s\2 has been added to the AKICK list for \2%s\2.", mu->name, mc->name);

			verbose(mc, "\2%s\2 added \2%s\2 to the AKICK list.", u->nick, mu->name);
			logcommand(chansvs.me, u, CMDLOG_SET, "%s AKICK ADD %s", mc->name, mu->name);

			return;
		}
	}
	else if (!strcasecmp("DEL", cmd))
	{
		if ((chanacs_user_flags(mc, u) & (CA_FLAGS | CA_REMOVE)) != (CA_FLAGS | CA_REMOVE))
		{
			notice(chansvs.nick, origin, "You are not authorized to perform this operation.");
			return;
		}

		mu = myuser_find_ext(uname);
		if (!mu)
		{
			/* we might be deleting a hostmask */
			if (!validhostmask(uname))
			{
				notice(chansvs.nick, origin, "\2%s\2 is neither a nickname nor a hostmask.", uname);
				return;
			}

			if (!chanacs_find_host_literal(mc, uname, CA_AKICK))
			{
				ca = chanacs_find_host(mc, uname, CA_AKICK);
				if (ca != NULL)
					notice(chansvs.nick, origin, "\2%s\2 is not on the AKICK list for \2%s\2, however \2%s\2 is.", uname, mc->name, ca->host);
				else
					notice(chansvs.nick, origin, "\2%s\2 is not on the AKICK list for \2%s\2.", uname, mc->name);
				return;
			}

			chanacs_delete_host(mc, uname, CA_AKICK);

			verbose(mc, "\2%s\2 removed \2%s\2 from the AKICK list.", u->nick, uname);
			logcommand(chansvs.me, u, CMDLOG_SET, "%s AKICK DEL %s", mc->name, uname);

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
		logcommand(chansvs.me, u, CMDLOG_SET, "%s AKICK DEL %s", mc->name, mu->name);

		verbose(mc, "\2%s\2 removed \2%s\2 from the AKICK list.", u->nick, mu->name);

		return;
	}
	else if (!strcasecmp("LIST", cmd))
	{
		uint8_t i = 0;

		if (!chanacs_user_has_flag(mc, u, CA_ACLVIEW))
		{
			if (has_priv(u, PRIV_CHAN_AUSPEX))
				operoverride = 1;
			else
			{
				notice(chansvs.nick, origin, "You are not authorized to perform this operation.");
				return;
			}
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
		if (operoverride)
			logcommand(chansvs.me, u, CMDLOG_ADMIN, "%s AKICK LIST (oper override)", mc->name);
		else
			logcommand(chansvs.me, u, CMDLOG_GET, "%s AKICK LIST", mc->name);
	}
}

/* !akick add *!*@*.aol.com */
void cs_fcmd_akick(char *origin, char *chan)
{
	user_t *u = user_find_named(origin);
	myuser_t *mu;
	mychan_t *mc;
	chanacs_t *ca, *ca2;
	node_t *n;
	char *cmd = strtok(NULL, " ");
	char *uname = strtok(NULL, " ");

	if (!cmd || !chan)
	{
		notice(chansvs.nick, origin, STR_INSUFFICIENT_PARAMS, "AKICK");
		notice(chansvs.nick, origin, "Syntax: AKICK <#channel> ADD|DEL|LIST <nickname|hostmask>");
		return;
	}

	if ((strcasecmp("LIST", cmd)) && (!uname))
	{
		notice(chansvs.nick, origin, STR_INSUFFICIENT_PARAMS, "!AKICK");
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
		if ((chanacs_user_flags(mc, u) & (CA_FLAGS | CA_REMOVE)) != (CA_FLAGS | CA_REMOVE))
		{
			notice(chansvs.nick, origin, "You are not authorized to perform this operation.");
			return;
		}

		mu = myuser_find_ext(uname);
		if (!mu)
		{
			/* we might be adding a hostmask */
			if (!validhostmask(uname))
			{
				notice(chansvs.nick, origin, "\2%s\2 is neither a nickname nor a hostmask.", uname);
				return;
			}

			uname = collapse(uname);

			ca = chanacs_find_host_literal(mc, uname, 0);
			if (ca != NULL)
			{
				if (ca->level & CA_AKICK)
					notice(chansvs.nick, origin, "\2%s\2 is already on the AKICK list for \2%s\2", uname, mc->name);
				else
					notice(chansvs.nick, origin, "\2%s\2 already has flags \2%s\2 on \2%s\2", uname, bitmask_to_flags(ca->level, chanacs_flags), mc->name);
				return;
			}

			ca = chanacs_find_host(mc, uname, CA_AKICK);
			if (ca != NULL)
			{
				notice(chansvs.nick, origin, "The more general mask \2%s\2 is already on the AKICK list for \2%s\2", ca->host, mc->name);
				return;
			}

			/* ok to use chanacs_add_host() here, already
			 * verified it doesn't exist yet */
			ca2 = chanacs_add_host(mc, uname, CA_AKICK);

			hook_call_event("channel_akick_add", ca2);

			notice(chansvs.nick, mc->name, "\2%s\2 added \2%s\2 to the AKICK list.", u->nick, uname);
			logcommand(chansvs.me, u, CMDLOG_SET, "%s AKICK ADD %s", mc->name, uname);

			notice(chansvs.nick, origin, "\2%s\2 has been added to the AKICK list for \2%s\2.", uname, mc->name);

			return;
		}
		else
		{
			if ((ca = chanacs_find(mc, mu, 0x0)))
			{
				if (ca->level & CA_AKICK)
					notice(chansvs.nick, origin, "\2%s\2 is already on the AKICK list for \2%s\2", mu->name, mc->name);
				else
					notice(chansvs.nick, origin, "\2%s\2 already has flags \2%s\2 on \2%s\2", mu->name, bitmask_to_flags(ca->level, chanacs_flags), mc->name);
				return;
			}

			ca2 = chanacs_add(mc, mu, CA_AKICK);

			hook_call_event("channel_akick_add", ca2);

			notice(chansvs.nick, origin, "\2%s\2 has been added to the AKICK list for \2%s\2.", mu->name, mc->name);

			notice(chansvs.nick, mc->name, "\2%s\2 added \2%s\2 to the AKICK list.", u->nick, mu->name);
			logcommand(chansvs.me, u, CMDLOG_SET, "%s AKICK ADD %s", mc->name, mu->name);

			return;
		}
	}
	else if (!strcasecmp("DEL", cmd))
	{
		if ((chanacs_user_flags(mc, u) & (CA_FLAGS | CA_REMOVE)) != (CA_FLAGS | CA_REMOVE))
		{
			notice(chansvs.nick, origin, "You are not authorized to perform this operation.");
			return;
		}

		mu = myuser_find_ext(uname);
		if (!mu)
		{
			/* we might be deleting a hostmask */
			if (!validhostmask(uname))
			{
				notice(chansvs.nick, origin, "\2%s\2 is neither a nickname nor a hostmask.", uname);
				return;
			}

			if (!chanacs_find_host_literal(mc, uname, CA_AKICK))
			{
				ca = chanacs_find_host(mc, uname, CA_AKICK);
				if (ca != NULL)
					notice(chansvs.nick, origin, "\2%s\2 is not on the AKICK list for \2%s\2, however \2%s\2 is.", uname, mc->name, ca->host);
				else
					notice(chansvs.nick, origin, "\2%s\2 is not on the AKICK list for \2%s\2.", uname, mc->name);
				return;
			}

			chanacs_delete_host(mc, uname, CA_AKICK);

			notice(chansvs.nick, mc->name, "\2%s\2 removed \2%s\2 from the AKICK list.", u->nick, uname);
			logcommand(chansvs.me, u, CMDLOG_SET, "%s AKICK DEL %s", mc->name, uname);

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
		logcommand(chansvs.me, u, CMDLOG_SET, "%s AKICK DEL %s", mc->name, mu->name);

		notice(chansvs.nick, mc->name, "\2%s\2 removed \2%s\2 from the AKICK list.", u->nick, mu->name);

		return;
	}
	else if (!strcasecmp("LIST", cmd))
	{
		uint8_t i = 0;

		if (!chanacs_user_has_flag(mc, u, CA_ACLVIEW))
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
		logcommand(chansvs.me, u, CMDLOG_GET, "%s AKICK LIST", mc->name);
	}
}
