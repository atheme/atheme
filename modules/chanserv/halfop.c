/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService OP functions.
 *
 * $Id: halfop.c 5686 2006-07-03 16:25:03Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/halfop", FALSE, _modinit, _moddeinit,
	"$Id: halfop.c 5686 2006-07-03 16:25:03Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_halfop(char *origin);
static void cs_cmd_dehalfop(char *origin);
static void cs_fcmd_halfop(char *origin, char *chan);
static void cs_fcmd_dehalfop(char *origin, char *chan);

command_t cs_halfop = { "HALFOP", "Gives channel halfops to a user.",
                        AC_NONE, cs_cmd_halfop };
command_t cs_dehalfop = { "DEHALFOP", "Removes channel halfops from a user.",
                        AC_NONE, cs_cmd_dehalfop };

fcommand_t fc_halfop = { "!halfop", AC_NONE, cs_fcmd_halfop };
fcommand_t fc_hop = { "!hop", AC_NONE, cs_fcmd_halfop };
fcommand_t fc_dehalfop = { "!dehalfop", AC_NONE, cs_fcmd_dehalfop };
fcommand_t fc_dehop = { "!dehop", AC_NONE, cs_fcmd_dehalfop };

list_t *cs_cmdtree;
list_t *cs_fcmdtree;
list_t *cs_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(cs_cmdtree, "chanserv/main", "cs_cmdtree");
	MODULE_USE_SYMBOL(cs_fcmdtree, "chanserv/main", "cs_fcmdtree");
	MODULE_USE_SYMBOL(cs_helptree, "chanserv/main", "cs_helptree");

        command_add(&cs_halfop, cs_cmdtree);
        command_add(&cs_dehalfop, cs_cmdtree);
	fcommand_add(&fc_halfop, cs_fcmdtree);
	fcommand_add(&fc_hop, cs_fcmdtree);
	fcommand_add(&fc_dehalfop, cs_fcmdtree);
	fcommand_add(&fc_dehop, cs_fcmdtree);

	help_addentry(cs_helptree, "HALFOP", "help/cservice/op_voice", NULL);
	help_addentry(cs_helptree, "DEHALFOP", "help/cservice/op_voice", NULL);
}

void _moddeinit()
{
	command_delete(&cs_halfop, cs_cmdtree);
	command_delete(&cs_dehalfop, cs_cmdtree);
	fcommand_delete(&fc_halfop, cs_fcmdtree);
	fcommand_delete(&fc_hop, cs_fcmdtree);
	fcommand_delete(&fc_dehalfop, cs_fcmdtree);
	fcommand_delete(&fc_dehop, cs_fcmdtree);

	help_delentry(cs_helptree, "HALFOP");
	help_delentry(cs_helptree, "DEHALFOP");
}

static void cs_cmd_halfop(char *origin)
{
	char *chan = strtok(NULL, " ");
	char *nick = strtok(NULL, " ");
	mychan_t *mc;
	user_t *u, *tu;
	chanuser_t *cu;

	if (!ircd->uses_halfops)
	{
		notice(chansvs.nick, origin, "Your IRC server does not support halfops.");
		return;
	}

	if (!chan)
	{
		notice(chansvs.nick, origin, STR_INSUFFICIENT_PARAMS, "HALFOP");
		notice(chansvs.nick, origin, "Syntax: HALFOP <#channel> [nickname]");
		return;
	}

	mc = mychan_find(chan);
	if (!mc)
	{
		notice(chansvs.nick, origin, "\2%s\2 is not registered.", chan);
		return;
	}

	u = user_find_named(origin);
	if (!chanacs_user_has_flag(mc, u, CA_HALFOP))
	{
		notice(chansvs.nick, origin, "You are not authorized to perform this operation.");
		return;
	}
	
	if (metadata_find(mc, METADATA_CHANNEL, "private:close:closer"))
	{
		notice(chansvs.nick, origin, "\2%s\2 is closed.", chan);
		return;
	}

	/* figure out who we're going to halfop */
	if (!nick)
		tu = u;
	else
	{
		if (!(tu = user_find_named(nick)))
		{
			notice(chansvs.nick, origin, "\2%s\2 is not online.", nick);
			return;
		}
	}

	if (is_internal_client(tu))
		return;

	/* SECURE check; we can skip this if sender == target, because we already verified */
	if ((u != tu) && (mc->flags & MC_SECURE) && !chanacs_user_has_flag(mc, tu, CA_HALFOP) && !chanacs_user_has_flag(mc, tu, CA_AUTOHALFOP))
	{
		notice(chansvs.nick, origin, "You are not authorized to perform this operation.", mc->name);
		notice(chansvs.nick, origin, "\2%s\2 has the SECURE option enabled, and \2%s\2 does not have appropriate access.", mc->name, tu->nick);
		return;
	}

	cu = chanuser_find(mc->chan, tu);
	if (!cu)
	{
		notice(chansvs.nick, origin, "\2%s\2 is not on \2%s\2.", tu->nick, mc->name);
		return;
	}

	modestack_mode_param(chansvs.nick, chan, MTYPE_ADD, 'h', CLIENT_NAME(tu));
	cu->modes |= ircd->halfops_mode;

	/* TODO: Add which username had access to perform the command */
	if (tu != u)
		notice(chansvs.nick, tu->nick, "You have been halfopped on %s by %s", mc->name, origin);

	logcommand(chansvs.me, u, CMDLOG_SET, "%s HALFOP %s!%s@%s", mc->name, tu->nick, tu->user, tu->vhost);
	if (!chanuser_find(mc->chan, u))
		notice(chansvs.nick, origin, "\2%s\2 has been halfopped on \2%s\2.", tu->nick, mc->name);
}

static void cs_cmd_dehalfop(char *origin)
{
	char *chan = strtok(NULL, " ");
	char *nick = strtok(NULL, " ");
	mychan_t *mc;
	user_t *u, *tu;
	chanuser_t *cu;

	if (!ircd->uses_halfops)
	{
		notice(chansvs.nick, origin, "Your IRC server does not support halfops.");
		return;
	}

	if (!chan)
	{
		notice(chansvs.nick, origin, STR_INSUFFICIENT_PARAMS, "DEHALFOP");
		notice(chansvs.nick, origin, "Syntax: DEHALFOP <#channel> [nickname]");
		return;
	}

	mc = mychan_find(chan);
	if (!mc)
	{
		notice(chansvs.nick, origin, "\2%s\2 is not registered.", chan);
		return;
	}

	u = user_find_named(origin);
	if (!chanacs_user_has_flag(mc, u, CA_HALFOP))
	{
		notice(chansvs.nick, origin, "You are not authorized to perform this operation.");
		return;
	}
	
	if (metadata_find(mc, METADATA_CHANNEL, "private:close:closer"))
	{
		notice(chansvs.nick, origin, "\2%s\2 is closed.", chan);
		return;
	}

	/* figure out who we're going to dehalfop */
	if (!nick)
		tu = u;
	else
	{
		if (!(tu = user_find_named(nick)))
		{
			notice(chansvs.nick, origin, "\2%s\2 is not online.", nick);
			return;
		}
	}

	if (is_internal_client(tu))
		return;

	cu = chanuser_find(mc->chan, tu);
	if (!cu)
	{
		notice(chansvs.nick, origin, "\2%s\2 is not on \2%s\2.", tu->nick, mc->name);
		return;
	}

	modestack_mode_param(chansvs.nick, chan, MTYPE_DEL, 'h', CLIENT_NAME(tu));
	cu->modes &= ~ircd->halfops_mode;

	/* TODO: Add which username had access to perform the command */
	if (tu != u)
		notice(chansvs.nick, tu->nick, "You have been dehalfopped on %s by %s", mc->name, origin);

	logcommand(chansvs.me, u, CMDLOG_SET, "%s DEHALFOP %s!%s@%s", mc->name, tu->nick, tu->user, tu->vhost);
	if (!chanuser_find(mc->chan, u))
		notice(chansvs.nick, origin, "\2%s\2 has been dehalfopped on \2%s\2.", tu->nick, mc->name);
}

static void cs_fcmd_halfop(char *origin, char *chan)
{
	char *nick;
	mychan_t *mc;
	user_t *u, *tu;
	chanuser_t *cu;

	if (!ircd->uses_halfops)
	{
		notice(chansvs.nick, origin, "Your IRC server does not support halfops.");
		return;
	}

	mc = mychan_find(chan);
	if (!mc)
	{
		notice(chansvs.nick, origin, "\2%s\2 is not registered.", chan);
		return;
	}

	u = user_find_named(origin);
	if (!chanacs_user_has_flag(mc, u, CA_HALFOP))
	{
		notice(chansvs.nick, origin, "You are not authorized to perform this operation.");
		return;
	}
	
	if (metadata_find(mc, METADATA_CHANNEL, "private:close:closer"))
	{
		notice(chansvs.nick, origin, "\2%s\2 is closed.", chan);
		return;
	}

	/* figure out who we're going to halfop, do so, repeat. */
	nick = strtok(NULL, " ");
	do
	{
		if (!nick)
			tu = u;
		else
		{
			if (!(tu = user_find_named(nick)))
			{
				notice(chansvs.nick, origin, "\2%s\2 is not online.", nick);
				continue;
			}
		}

		if (is_internal_client(tu))
			continue;

		/* SECURE check; we can skip this if sender == target, because we already verified */
		if ((u != tu) && (mc->flags & MC_SECURE) && !chanacs_user_has_flag(mc, tu, CA_HALFOP) && !chanacs_user_has_flag(mc, tu, CA_AUTOHALFOP))
		{
			notice(chansvs.nick, origin, "You are not authorized to perform this operation.", mc->name);
			notice(chansvs.nick, origin, "\2%s\2 has the SECURE option enabled, and \2%s\2 does not have appropriate access.", mc->name, tu->nick);
			continue;
		}

		cu = chanuser_find(mc->chan, tu);
		if (!cu)
		{
			notice(chansvs.nick, origin, "\2%s\2 is not on \2%s\2.", tu->nick, mc->name);
			continue;
		}

		modestack_mode_param(chansvs.nick, chan, MTYPE_ADD, 'h', CLIENT_NAME(tu));
		cu->modes |= ircd->halfops_mode;
		logcommand(chansvs.me, u, CMDLOG_SET, "%s HALFOP %s!%s@%s", mc->name, tu->nick, tu->user, tu->vhost);
	} while ((nick = strtok(NULL, " ")) != NULL);
}

static void cs_fcmd_dehalfop(char *origin, char *chan)
{
	char *nick;
	mychan_t *mc;
	user_t *u, *tu;
	chanuser_t *cu;

	if (!ircd->uses_halfops)
	{
		notice(chansvs.nick, origin, "Your IRC server does not support halfops.");
		return;
	}

	mc = mychan_find(chan);
	if (!mc)
	{
		notice(chansvs.nick, origin, "\2%s\2 is not registered.", chan);
		return;
	}

	u = user_find_named(origin);
	if (!chanacs_user_has_flag(mc, u, CA_HALFOP))
	{
		notice(chansvs.nick, origin, "You are not authorized to perform this operation.");
		return;
	}
	
	if (metadata_find(mc, METADATA_CHANNEL, "private:close:closer"))
	{
		notice(chansvs.nick, origin, "\2%s\2 is closed.", chan);
		return;
	}

	/* figure out who we're going to dehalfop */
	nick = strtok(NULL, " ");
	do
	{
		if (!nick)
			tu = u;
		else
		{
			if (!(tu = user_find_named(nick)))
			{
				notice(chansvs.nick, origin, "\2%s\2 is not online.", nick);
				continue;
			}
		}

		if (is_internal_client(tu))
			continue;

		cu = chanuser_find(mc->chan, tu);
		if (!cu)
		{
			notice(chansvs.nick, origin, "\2%s\2 is not on \2%s\2.", tu->nick, mc->name);
			continue;
		}

		modestack_mode_param(chansvs.nick, chan, MTYPE_DEL, 'h', CLIENT_NAME(tu));
		cu->modes &= ~ircd->halfops_mode;
		logcommand(chansvs.me, u, CMDLOG_SET, "%s DEHALFOP %s!%s@%s", mc->name, tu->nick, tu->user, tu->vhost);
	} while ((nick = strtok(NULL, " ")) != NULL);
}
