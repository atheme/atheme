/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService REGISTER function.
 *
 * $Id: register.c 5686 2006-07-03 16:25:03Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/register", FALSE, _modinit, _moddeinit,
	"$Id: register.c 5686 2006-07-03 16:25:03Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_register(char *origin);

command_t cs_register = { "REGISTER", "Registers a channel.",
                           AC_NONE, cs_cmd_register };
                                                                                   
list_t *cs_cmdtree;
list_t *cs_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(cs_cmdtree, "chanserv/main", "cs_cmdtree");
	MODULE_USE_SYMBOL(cs_helptree, "chanserv/main", "cs_helptree");

        command_add(&cs_register, cs_cmdtree);
	help_addentry(cs_helptree, "REGISTER", "help/cservice/register", NULL);
}

void _moddeinit()
{
	command_delete(&cs_register, cs_cmdtree);
	help_delentry(cs_helptree, "REGISTER");
}

static void cs_cmd_register(char *origin)
{
	user_t *u = user_find_named(origin);
	channel_t *c;
	chanuser_t *cu;
	mychan_t *mc, *tmc;
	node_t *n;
	char *name = strtok(NULL, " ");
	uint32_t i, tcnt;

	if (!name)
	{
		notice(chansvs.nick, origin, STR_INSUFFICIENT_PARAMS, "REGISTER");
		notice(chansvs.nick, origin, "To register a channel: REGISTER <#channel>");
		return;
	}

	if (*name != '#')
	{
		notice(chansvs.nick, origin, STR_INVALID_PARAMS, "REGISTER");
		notice(chansvs.nick, origin, "Syntax: REGISTER <#channel>");
		return;
	}

	/* make sure they're logged in */
	if (!u->myuser)
	{
		notice(chansvs.nick, origin, "You are not logged in.");
		return;
	}

	if (u->myuser->flags & MU_WAITAUTH)
	{
		notice(memosvs.nick, origin, "You need to verify your email address before you may register channels.");
		return;
	}
	
	/* make sure it isn't already registered */
	if ((mc = mychan_find(name)))
	{
		notice(chansvs.nick, origin, "\2%s\2 is already registered to \2%s\2.", mc->name, mc->founder->name);
		return;
	}

	/* make sure the channel exists */
	if (!(c = channel_find(name)))
	{
		notice(chansvs.nick, origin, "The channel \2%s\2 must exist in order to register it.", name);
		return;
	}

	/* make sure they're in it */
	if (!(cu = chanuser_find(c, u)))
	{
		notice(chansvs.nick, origin, "You must be in \2%s\2 in order to register it.", name);
		return;
	}

	/* make sure they're opped */
	if (!(CMODE_OP & cu->modes))
	{
		notice(chansvs.nick, origin, "You must be a channel operator in \2%s\2 in order to " "register it.", name);
		return;
	}

	/* make sure they're within limits */
	for (i = 0, tcnt = 0; i < HASHSIZE; i++)
	{
		LIST_FOREACH(n, mclist[i].head)
		{
			tmc = (mychan_t *)n->data;

			if (is_founder(tmc, u->myuser))
				tcnt++;
		}
	}

	if ((tcnt >= me.maxchans) && !has_priv(u, PRIV_REG_NOLIMIT))
	{
		notice(chansvs.nick, origin, "You have too many channels registered.");
		return;
	}

	logcommand(chansvs.me, u, CMDLOG_REGISTER, "%s REGISTER", name);
	snoop("REGISTER: \2%s\2 to \2%s\2 as \2%s\2", name, u->nick, u->myuser->name);

	mc = mychan_add(name);
	mc->founder = u->myuser;
	mc->registered = CURRTIME;
	mc->used = CURRTIME;
	mc->mlock_on |= (CMODE_NOEXT | CMODE_TOPIC);
	mc->mlock_off |= (CMODE_LIMIT | CMODE_KEY);
	mc->flags |= config_options.defcflags;

	chanacs_add(mc, u->myuser, CA_INITIAL);

	notice(chansvs.nick, origin, "\2%s\2 is now registered to \2%s\2.", mc->name, mc->founder->name);

	hook_call_event("channel_register", mc);
}
