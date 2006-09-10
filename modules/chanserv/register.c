/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService REGISTER function.
 *
 * $Id: register.c 6337 2006-09-10 15:54:41Z pippijn $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/register", FALSE, _modinit, _moddeinit,
	"$Id: register.c 6337 2006-09-10 15:54:41Z pippijn $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_register(sourceinfo_t *si, int parc, char *parv[]);

command_t cs_register = { "REGISTER", "Registers a channel.",
                           AC_NONE, 1, cs_cmd_register };

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

static void cs_cmd_register(sourceinfo_t *si, int parc, char *parv[])
{
	channel_t *c;
	chanuser_t *cu;
	mychan_t *mc, *tmc;
	node_t *n;
	char *name = parv[0];
	uint32_t i, tcnt;
	char str[21];

	if (!name)
	{
		notice(chansvs.nick, si->su->nick, STR_INSUFFICIENT_PARAMS, "REGISTER");
		notice(chansvs.nick, si->su->nick, "To register a channel: REGISTER <#channel>");
		return;
	}

	if (*name != '#')
	{
		notice(chansvs.nick, si->su->nick, STR_INVALID_PARAMS, "REGISTER");
		notice(chansvs.nick, si->su->nick, "Syntax: REGISTER <#channel>");
		return;
	}

	/* make sure they're logged in */
	if (!si->su->myuser)
	{
		notice(chansvs.nick, si->su->nick, "You are not logged in.");
		return;
	}

	if (si->su->myuser->flags & MU_WAITAUTH)
	{
		notice(chansvs.nick, si->su->nick, "You need to verify your email address before you may register channels.");
		return;
	}
	
	/* make sure it isn't already registered */
	if ((mc = mychan_find(name)))
	{
		notice(chansvs.nick, si->su->nick, "\2%s\2 is already registered to \2%s\2.", mc->name, mc->founder->name);
		return;
	}

	/* make sure the channel exists */
	if (!(c = channel_find(name)))
	{
		notice(chansvs.nick, si->su->nick, "The channel \2%s\2 must exist in order to register it.", name);
		return;
	}

	/* make sure they're in it */
	if (!(cu = chanuser_find(c, si->su)))
	{
		notice(chansvs.nick, si->su->nick, "You must be in \2%s\2 in order to register it.", name);
		return;
	}

	/* make sure they're opped */
	if (!(CMODE_OP & cu->modes))
	{
		notice(chansvs.nick, si->su->nick, "You must be a channel operator in \2%s\2 in order to " "register it.", name);
		return;
	}

	/* make sure they're within limits */
	for (i = 0, tcnt = 0; i < HASHSIZE; i++)
	{
		LIST_FOREACH(n, mclist[i].head)
		{
			tmc = (mychan_t *)n->data;

			if (is_founder(tmc, si->su->myuser))
				tcnt++;
		}
	}

	if ((tcnt >= me.maxchans) && !has_priv(si->su, PRIV_REG_NOLIMIT))
	{
		notice(chansvs.nick, si->su->nick, "You have too many channels registered.");
		return;
	}

	logcommand(chansvs.me, si->su, CMDLOG_REGISTER, "%s REGISTER", name);
	snoop("REGISTER: \2%s\2 to \2%s\2 as \2%s\2", name, si->su->nick, si->su->myuser->name);

	mc = mychan_add(name);
	mc->founder = si->su->myuser;
	mc->registered = CURRTIME;
	mc->used = CURRTIME;
	mc->mlock_on |= (CMODE_NOEXT | CMODE_TOPIC);
	mc->mlock_off |= (CMODE_LIMIT | CMODE_KEY);
	mc->flags |= config_options.defcflags;

	chanacs_add(mc, si->su->myuser, CA_INITIAL);

	if (c->ts > 0)
	{
		snprintf(str, sizeof str, "%lu", (unsigned long)c->ts);
		metadata_add(mc, METADATA_CHANNEL, "private:channelts", str);
	}

	notice(chansvs.nick, si->su->nick, "\2%s\2 is now registered to \2%s\2.", mc->name, mc->founder->name);

	hook_call_event("channel_register", mc);
}
