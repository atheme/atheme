/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService VOICE functions.
 *
 * $Id: voice.c 2129 2005-09-05 00:59:19Z nenolod $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/voice", FALSE, _modinit, _moddeinit,
	"$Id: voice.c 2129 2005-09-05 00:59:19Z nenolod $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_voice(char *origin);
static void cs_cmd_devoice(char *origin);
static void cs_fcmd_voice(char *origin, char *chan);
static void cs_fcmd_devoice(char *origin, char *chan);

command_t cs_voice = { "VOICE", "Gives channel voice to a user.",
                         AC_NONE, cs_cmd_voice };
command_t cs_devoice = { "DEVOICE", "Removes channel voice from a user.",
                         AC_NONE, cs_cmd_devoice };

fcommand_t fc_voice = { "!voice", AC_NONE, cs_fcmd_voice };
fcommand_t fc_devoice = { "!devoice", AC_NONE, cs_fcmd_devoice };

list_t *cs_cmdtree;
list_t *cs_fcmdtree;

void _modinit(module_t *m)
{
	cs_cmdtree = module_locate_symbol("chanserv/main", "cs_cmdtree");
	cs_fcmdtree = module_locate_symbol("chanserv/main", "cs_fcmdtree");

        command_add(&cs_voice, cs_cmdtree);
        command_add(&cs_devoice, cs_cmdtree);
	fcommand_add(&fc_voice, cs_fcmdtree);
	fcommand_add(&fc_devoice, cs_fcmdtree);
}

void _moddeinit()
{
	command_delete(&cs_voice, cs_cmdtree);
	command_delete(&cs_devoice, cs_cmdtree);
	fcommand_delete(&fc_voice, cs_fcmdtree);
	fcommand_delete(&fc_devoice, cs_fcmdtree);
}

static void cs_cmd_voice(char *origin)
{
	char *chan = strtok(NULL, " ");
	char *nick = strtok(NULL, " ");
	mychan_t *mc;
	user_t *u, *tu;
	chanuser_t *cu;

	if (!chan)
	{
		notice(chansvs.nick, origin, "Insufficient parameters specified for \2VOICE\2.");
		notice(chansvs.nick, origin, "Syntax: VOICE <#channel> [nickname]");
		return;
	}

	mc = mychan_find(chan);
	if (!mc)
	{
		notice(chansvs.nick, origin, "\2%s\2 is not registered.", chan);
		return;
	}

	u = user_find(origin);
	if (!chanacs_user_has_flag(mc, u, CA_VOICE))
	{
		notice(chansvs.nick, origin, "You are not authorized to perform this operation.");
		return;
	}

	/* figure out who we're going to voice */
	if (!nick)
		tu = u;
	else
	{
		if (!(tu = user_find(nick)))
		{
			notice(chansvs.nick, origin, "\2%s\2 is not online.", nick);
			return;
		}
	}
	
	if (tu->server == me.me)
		return;

	cu = chanuser_find(mc->chan, tu);
	if (!cu)
	{
		notice(chansvs.nick, origin, "\2%s\2 is not on \2%s\2.", tu->nick, mc->name);
		return;
	}

	if (CMODE_VOICE & cu->modes)
	{
		notice(chansvs.nick, origin, "\2%s\2 is already voiced on \2%s\2.", tu->nick, mc->name);
		return;
	}

	cmode(chansvs.nick, chan, "+v", CLIENT_NAME(tu));
	cu->modes |= CMODE_VOICE;
	notice(chansvs.nick, origin, "\2%s\2 has been voiced on \2%s\2.", tu->nick, mc->name);
}

static void cs_cmd_devoice(char *origin)
{
	char *chan = strtok(NULL, " ");
	char *nick = strtok(NULL, " ");
	mychan_t *mc;
	user_t *u, *tu;
	chanuser_t *cu;

	if (!chan)
	{
		notice(chansvs.nick, origin, "Insufficient parameters specified for \2DEVOICE\2.");
		notice(chansvs.nick, origin, "Syntax: DEVOICE <#channel> [nickname]");
		return;
	}

	mc = mychan_find(chan);
	if (!mc)
	{
		notice(chansvs.nick, origin, "\2%s\2 is not registered.", chan);
		return;
	}

	u = user_find(origin);
	if (!chanacs_user_has_flag(mc, u, CA_VOICE))
	{
		notice(chansvs.nick, origin, "You are not authorized to perform this operation.");
		return;
	}

	/* figure out who we're going to devoice */
	if (!nick)
		tu = u;
	else
	{
		if (!(tu = user_find(nick)))
		{
			notice(chansvs.nick, origin, "\2%s\2 is not online.", nick);
			return;
		}
	}

	if (u->server == me.me)
		return;

	cu = chanuser_find(mc->chan, tu);
	if (!cu)
	{
		notice(chansvs.nick, origin, "\2%s\2 is not on \2%s\2.", tu->nick, mc->name);
		return;
	}

	if (!(CMODE_VOICE & cu->modes))
	{
		notice(chansvs.nick, origin, "\2%s\2 is not voiced on \2%s\2.", tu->nick, mc->name);
		return;
	}

	cmode(chansvs.nick, chan, "-v", CLIENT_NAME(tu));
	cu->modes &= ~CMODE_VOICE;
	notice(chansvs.nick, origin, "\2%s\2 has been devoiced on \2%s\2.", tu->nick, mc->name);
}

static void cs_fcmd_voice(char *origin, char *chan)
{
	char *nick;
	mychan_t *mc;
	user_t *u, *tu;
	chanuser_t *cu;

	mc = mychan_find(chan);
	if (!mc)
	{
		notice(chansvs.nick, origin, "\2%s\2 is not registered.", chan);
		return;
	}

	u = user_find(origin);
	if (!chanacs_user_has_flag(mc, u, CA_VOICE))
	{
		notice(chansvs.nick, origin, "You are not authorized to perform this operation.");
		return;
	}
	
	/* figure out who we're going to voice */
	while ((nick = strtok(NULL, " ")))
	{
		if (!nick)
			tu = u;
		else
		{
			if (!(tu = user_find(nick)))
			{
				notice(chansvs.nick, origin, "\2%s\2 is not online.", nick);
				continue;
			}
		}

		if (tu->server == me.me)
			continue;

		cu = chanuser_find(mc->chan, tu);
		if (!cu)
		{
			notice(chansvs.nick, origin, "\2%s\2 is not on \2%s\2.", tu->nick, mc->name);
			continue;
		}

		if (CMODE_VOICE & cu->modes)
		{
			notice(chansvs.nick, origin, "\2%s\2 is already voiced on \2%s\2.", tu->nick, mc->name);
			continue;
		}

		cmode(chansvs.nick, chan, "+v", CLIENT_NAME(tu));
		cu->modes |= CMODE_VOICE;
	}
}

static void cs_fcmd_devoice(char *origin, char *chan)
{
	char *nick;
	mychan_t *mc;
	user_t *u, *tu;
	chanuser_t *cu;

	mc = mychan_find(chan);
	if (!mc)
	{
		notice(chansvs.nick, origin, "\2%s\2 is not registered.", chan);
		return;
	}

	u = user_find(origin);
	if (!chanacs_user_has_flag(mc, u, CA_VOICE))
	{
		notice(chansvs.nick, origin, "You are not authorized to perform this operation.");
		return;
	}

	/* figure out who we're going to devoice */
	while ((nick = strtok(NULL, " ")))
	{
		if (!nick)
			tu = u;
		else
		{
			if (!(tu = user_find(nick)))
			{
				notice(chansvs.nick, origin, "\2%s\2 is not online.", nick);
				continue;
			}
		}

		if (tu->server == me.me)
			continue;

		cu = chanuser_find(mc->chan, tu);
		if (!cu)
		{
			notice(chansvs.nick, origin, "\2%s\2 is not on \2%s\2.", tu->nick, mc->name);
			continue;
		}

		if (!(CMODE_VOICE & cu->modes))
		{
			notice(chansvs.nick, origin, "\2%s\2 is not voiced on \2%s\2.", tu->nick, mc->name);
			continue;
		}

		cmode(chansvs.nick, chan, "-v", CLIENT_NAME(tu));
		cu->modes &= ~CMODE_VOICE;
	}
}

