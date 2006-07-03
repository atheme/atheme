/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService FLAGS functions.
 *
 * $Id: flags.c 5686 2006-07-03 16:25:03Z jilles $
 */

#include "atheme.h"
#include "template.h"

DECLARE_MODULE_V1
(
	"chanserv/flags", FALSE, _modinit, _moddeinit,
	"$Id: flags.c 5686 2006-07-03 16:25:03Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_flags(char *origin);
static void cs_fcmd_flags(char *origin, char *channel);

command_t cs_flags = { "FLAGS", "Manipulates specific permissions on a channel.",
                        AC_NONE, cs_cmd_flags };
fcommand_t cs_fantasy_flags = { ".flags", AC_NONE, cs_fcmd_flags };

list_t *cs_cmdtree;
list_t *cs_fcmdtree;
list_t *cs_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(cs_cmdtree, "chanserv/main", "cs_cmdtree");
	MODULE_USE_SYMBOL(cs_fcmdtree, "chanserv/main", "cs_fcmdtree");
	MODULE_USE_SYMBOL(cs_helptree, "chanserv/main", "cs_helptree");

        command_add(&cs_flags, cs_cmdtree);
	fcommand_add(&cs_fantasy_flags, cs_fcmdtree);
	help_addentry(cs_helptree, "FLAGS", "help/cservice/flags", NULL);
}

void _moddeinit()
{
	command_delete(&cs_flags, cs_cmdtree);
	fcommand_delete(&cs_fantasy_flags, cs_fcmdtree);
	help_delentry(cs_helptree, "FLAGS");
}

/* FLAGS <channel> [user] [flags] */
static void cs_cmd_flags(char *origin)
{
	user_t *u = user_find_named(origin);
	chanacs_t *ca;
	node_t *n;
	int operoverride = 0;
	char *channel = strtok(NULL, " ");
	char *target = strtok(NULL, " ");
	uint32_t addflags, removeflags, restrictflags;

	if (!channel)
	{
		notice(chansvs.nick, origin, STR_INSUFFICIENT_PARAMS, "FLAGS");
		notice(chansvs.nick, origin, "Syntax: FLAGS <channel> [target] [flags]");
		return;
	}

	if (!target)
	{
		mychan_t *mc = mychan_find(channel);
		uint8_t i = 1;

		if (!mc)
		{
			notice(chansvs.nick, origin, "\2%s\2 is not registered.", channel);
			return;
		}

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
		
		if (metadata_find(mc, METADATA_CHANNEL, "private:close:closer") && !has_priv(u, PRIV_CHAN_AUSPEX))
		{
			notice(chansvs.nick, origin, "\2%s\2 is closed.", channel);
			return;
		}

		notice(chansvs.nick, origin, "Entry Nickname/Host          Flags");
		notice(chansvs.nick, origin, "----- ---------------------- -----");

		LIST_FOREACH(n, mc->chanacs.head)
		{
			ca = n->data;
			notice(chansvs.nick, origin, "%-5d %-22s %s", i, ca->myuser ? ca->myuser->name : ca->host, bitmask_to_flags(ca->level, chanacs_flags));
			i++;
		}

		notice(chansvs.nick, origin, "----- ---------------------- -----");
		notice(chansvs.nick, origin, "End of \2%s\2 FLAGS listing.", channel);
		if (operoverride)
			logcommand(chansvs.me, u, CMDLOG_ADMIN, "%s FLAGS (oper override)", mc->name);
		else
			logcommand(chansvs.me, u, CMDLOG_GET, "%s FLAGS", mc->name);
	}
	else
	{
		mychan_t *mc = mychan_find(channel);
		myuser_t *tmu;
		char *flagstr = strtok(NULL, " ");

		if (!u->myuser)
		{
			notice(chansvs.nick, origin, "You are not logged in.");
			return;
		}

		if (!mc)
		{
			notice(chansvs.nick, origin, "\2%s\2 is not registered.", channel);
			return;
		}
		
		if (metadata_find(mc, METADATA_CHANNEL, "private:close:closer"))
		{
			notice(chansvs.nick, origin, "\2%s\2 is closed.", channel);
			return;
		}

		if (!target)
		{
			notice(chansvs.nick, origin, "Usage: FLAGS %s [target] [flags]", channel);
			return;
		}

		if (!flagstr)
		{
			if (!(chanacs_user_flags(mc, u) & CA_ACLVIEW))
			{
				notice(chansvs.nick, origin, "You are not authorized to execute this command.");
				return;
			}
			if (validhostmask(target))
				ca = chanacs_find_host_literal(mc, target, 0);
			else
			{
				if (!(tmu = myuser_find_ext(target)))
				{
					notice(chansvs.nick, origin, "The nickname \2%s\2 is not registered.", target);
					return;
				}
				ca = chanacs_find(mc, tmu, 0);
			}
			if (ca != NULL)
				notice(chansvs.nick, origin, "Flags for \2%s\2 in \2%s\2 are \2%s\2.",
						target, channel,
						bitmask_to_flags2(ca != NULL ? ca->level : 0, 0, chanacs_flags));
			else
				notice(chansvs.nick, origin, "No flags for \2%s\2 in \2%s\2.",
						target, channel);
			logcommand(chansvs.me, u, CMDLOG_SET, "%s FLAGS %s", mc->name, target);
			return;
		}

		/* founder may always set flags -- jilles */
		if (is_founder(mc, u->myuser))
			restrictflags = CA_ALL;
		else
		{
			restrictflags = chanacs_user_flags(mc, u);
			if (!(restrictflags & CA_FLAGS))
			{
				/* allow a user to remove their own access
				 * even without +f */
				if (restrictflags & CA_AKICK ||
						u->myuser == NULL ||
						irccmp(target, u->myuser->name) ||
						strcmp(flagstr, "-*"))
				{
					notice(chansvs.nick, origin, "You are not authorized to execute this command.");
					return;
				}
			}
			restrictflags = allow_flags(restrictflags);
		}

		if (*flagstr == '+' || *flagstr == '-' || *flagstr == '=')
		{
			flags_make_bitmasks(flagstr, chanacs_flags, &addflags, &removeflags);
			if (addflags == 0 && removeflags == 0)
			{
				notice(chansvs.nick, origin, "No valid flags given, use /%s%s HELP FLAGS for a list", ircd->uses_rcommand ? "" : "msg ", chansvs.disp);
				return;
			}
		}
		else
		{
			addflags = get_template_flags(mc, flagstr);
			if (addflags == 0)
			{
				/* Hack -- jilles */
				if (*target == '+' || *target == '-' || *target == '=')
					notice(chansvs.nick, origin, "Usage: FLAGS %s [target] [flags]", mc->name);
				else
					notice(chansvs.nick, origin, "Invalid template name given, use /%s%s TEMPLATE %s for a list", ircd->uses_rcommand ? "" : "msg ", chansvs.disp, mc->name);
				return;
			}
			removeflags = CA_ALL & ~addflags;
		}

		if (!validhostmask(target))
		{
			if (!(tmu = myuser_find_ext(target)))
			{
				notice(chansvs.nick, origin, "The nickname \2%s\2 is not registered.", target);
				return;
			}

			if (tmu == mc->founder && removeflags & CA_FLAGS)
			{
				notice(chansvs.nick, origin, "You may not remove the founder's +f access.");
				return;
			}

			/* If NEVEROP is set, don't allow adding new entries
			 * except sole +b. Adding flags if the current level
			 * is +b counts as adding an entry.
			 * -- jilles */
			if (MU_NEVEROP & tmu->flags && addflags != CA_AKICK && addflags != 0 && ((ca = chanacs_find(mc, tmu, 0)) == NULL || ca->level == CA_AKICK))
			{
				notice(chansvs.nick, origin, "\2%s\2 does not wish to be added to access lists (NEVEROP set).", tmu->name);
				return;
			}

			if (!chanacs_change(mc, tmu, NULL, &addflags, &removeflags, restrictflags))
			{
		                notice(chansvs.nick, origin, "You are not allowed to set \2%s\2 on \2%s\2 in \2%s\2.", bitmask_to_flags2(addflags, removeflags, chanacs_flags), tmu->name, mc->name);
				return;
			}
		}
		else
		{
			if (!chanacs_change(mc, NULL, target, &addflags, &removeflags, restrictflags))
			{
		                notice(chansvs.nick, origin, "You are not allowed to set \2%s\2 on \2%s\2 in \2%s\2.", bitmask_to_flags2(addflags, removeflags, chanacs_flags), target, mc->name);
				return;
			}
		}

		if ((addflags | removeflags) == 0)
		{
			notice(chansvs.nick, origin, "Channel access to \2%s\2 for \2%s\2 unchanged.", channel, target);
			return;
		}
		flagstr = bitmask_to_flags2(addflags, removeflags, chanacs_flags);
		notice(chansvs.nick, origin, "Flags \2%s\2 were set on \2%s\2 in \2%s\2.", flagstr, target, channel);
		logcommand(chansvs.me, u, CMDLOG_SET, "%s FLAGS %s %s", mc->name, target, flagstr);
		verbose(mc, "\2%s\2 set flags \2%s\2 on \2%s\2 in \2%s\2.", origin, flagstr, target, channel);
	}
}

/* FLAGS <channel> [user] [flags] */
static void cs_fcmd_flags(char *origin, char *channel)
{
	user_t *u = user_find_named(origin);
	chanacs_t *ca;
	mychan_t *mc = mychan_find(channel);
	myuser_t *tmu;
	char *target = strtok(NULL, " ");
	char *flagstr = strtok(NULL, " ");
	uint32_t addflags, removeflags, restrictflags;

	if (!target || !flagstr)
	{
		notice(chansvs.nick, origin, STR_INSUFFICIENT_PARAMS, "FLAGS");
		notice(chansvs.nick, origin, "Syntax: .flags <target> <flags>");
		return;
	}

	if (!u->myuser)
	{
		notice(chansvs.nick, origin, "You are not logged in.");
		return;
	}

	/* founder may always set flags -- jilles */
	if (is_founder(mc, u->myuser))
		restrictflags = CA_ALL;
	else
	{
		restrictflags = chanacs_user_flags(mc, u);
		if (!(restrictflags & CA_FLAGS))
		{
			/* allow a user to remove their own access
			 * even without +f */
			if (restrictflags & CA_AKICK ||
					u->myuser == NULL ||
					irccmp(target, u->myuser->name) ||
					strcmp(flagstr, "-*"))
			{
				notice(chansvs.nick, origin, "You are not authorized to execute this command.");
				return;
			}
		}
		restrictflags = allow_flags(restrictflags);
	}
	
	if (metadata_find(mc, METADATA_CHANNEL, "private:close:closer"))
	{
		notice(chansvs.nick, origin, "\2%s\2 is closed.", channel);
		return;
	}

	if (*flagstr == '+' || *flagstr == '-' || *flagstr == '=')
	{
		flags_make_bitmasks(flagstr, chanacs_flags, &addflags, &removeflags);
		if (addflags == 0 && removeflags == 0)
		{
			notice(chansvs.nick, origin, "No valid flags given, use /%s%s HELP FLAGS for a list", ircd->uses_rcommand ? "" : "msg ", chansvs.disp);
			return;
		}
	}
	else
	{
		addflags = get_template_flags(mc, flagstr);
		if (addflags == 0)
		{
			/* Hack -- jilles */
			if (*target == '+' || *target == '-' || *target == '=')
				notice(chansvs.nick, origin, "Syntax: .flags <target> <flags>");
			else
				notice(chansvs.nick, origin, "Invalid template name given, use /%s%s TEMPLATE %s for a list", ircd->uses_rcommand ? "" : "msg ", chansvs.disp, mc->name);
			return;
		}
		removeflags = CA_ALL & ~addflags;
	}

	if (!validhostmask(target))
	{
		if (!(tmu = myuser_find_ext(target)))
		{
			notice(chansvs.nick, origin, "The nickname \2%s\2 is not registered.", target);
			return;
		}

		if (tmu == mc->founder && removeflags & CA_FLAGS)
		{
			notice(chansvs.nick, origin, "You may not remove the founder's +f access.");
			return;
		}

		/* If NEVEROP is set, don't allow adding new entries
		 * except sole +b. Adding flags if the current level
		 * is +b counts as adding an entry.
		 * -- jilles */
		if (MU_NEVEROP & tmu->flags && addflags != CA_AKICK && addflags != 0 && ((ca = chanacs_find(mc, tmu, 0)) == NULL || ca->level == CA_AKICK))
		{
			notice(chansvs.nick, origin, "\2%s\2 does not wish to be added to access lists (NEVEROP set).", tmu->name);
			return;
		}

		if (!chanacs_change(mc, tmu, NULL, &addflags, &removeflags, restrictflags))
		{
			notice(chansvs.nick, origin, "You are not allowed to set \2%s\2 on \2%s\2 in \2%s\2.", bitmask_to_flags2(addflags, removeflags, chanacs_flags), tmu->name, mc->name);
			return;
		}
	}
	else
	{
		if (!chanacs_change(mc, NULL, target, &addflags, &removeflags, restrictflags))
		{
			notice(chansvs.nick, origin, "You are not allowed to set \2%s\2 on \2%s\2 in \2%s\2.", bitmask_to_flags2(addflags, removeflags, chanacs_flags), target, mc->name);
			return;
		}
	}

	if ((addflags | removeflags) == 0)
	{
		notice(chansvs.nick, origin, "Channel access to \2%s\2 for \2%s\2 unchanged.", channel, target);
		return;
	}
	flagstr = bitmask_to_flags2(addflags, removeflags, chanacs_flags);
	logcommand(chansvs.me, u, CMDLOG_SET, "%s FLAGS %s %s", mc->name, target, flagstr);
	notice(chansvs.nick, channel, "\2%s\2 set flags \2%s\2 on \2%s\2 in \2%s\2.", origin, flagstr, target, channel);
}
