/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService OP functions.
 *
 * $Id: op.c 6547 2006-09-29 16:39:38Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/op", FALSE, _modinit, _moddeinit,
	"$Id: op.c 6547 2006-09-29 16:39:38Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_op(sourceinfo_t *si, int parc, char *parv[]);
static void cs_cmd_deop(sourceinfo_t *si, int parc, char *parv[]);
static void cs_fcmd_op(char *origin, char *chan);
static void cs_fcmd_deop(char *origin, char *chan);

command_t cs_op = { "OP", "Gives channel ops to a user.",
                        AC_NONE, 2, cs_cmd_op };
command_t cs_deop = { "DEOP", "Removes channel ops from a user.",
                        AC_NONE, 2, cs_cmd_deop };

fcommand_t fc_op = { "!op", AC_NONE, cs_fcmd_op };
fcommand_t fc_deop = { "!deop", AC_NONE, cs_fcmd_deop };
                                                                                   
list_t *cs_cmdtree;
list_t *cs_fcmdtree;
list_t *cs_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(cs_cmdtree, "chanserv/main", "cs_cmdtree");
	MODULE_USE_SYMBOL(cs_fcmdtree, "chanserv/main", "cs_fcmdtree");
	MODULE_USE_SYMBOL(cs_helptree, "chanserv/main", "cs_helptree");

        command_add(&cs_op, cs_cmdtree);
        command_add(&cs_deop, cs_cmdtree);
	fcommand_add(&fc_op, cs_fcmdtree);
	fcommand_add(&fc_deop, cs_fcmdtree);

	help_addentry(cs_helptree, "OP", "help/cservice/op_voice", NULL);
	help_addentry(cs_helptree, "DEOP", "help/cservice/op_voice", NULL);
}

void _moddeinit()
{
	command_delete(&cs_op, cs_cmdtree);
	command_delete(&cs_deop, cs_cmdtree);
	fcommand_delete(&fc_op, cs_fcmdtree);
	fcommand_delete(&fc_deop, cs_fcmdtree);

	help_delentry(cs_helptree, "OP");
	help_delentry(cs_helptree, "DEOP");
}

static void cs_cmd_op(sourceinfo_t *si, int parc, char *parv[])
{
	char *chan = parv[0];
	char *nick = parv[1];
	mychan_t *mc;
	user_t *tu;
	chanuser_t *cu;

	if (!chan)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "OP");
		command_fail(si, fault_needmoreparams, "Syntax: OP <#channel> [nickname]");
		return;
	}

	mc = mychan_find(chan);
	if (!mc)
	{
		command_fail(si, fault_nosuch_target, "\2%s\2 is not registered.", chan);
		return;
	}

	si->su = user_find_named(si->su->nick);
	if (!chanacs_user_has_flag(mc, si->su, CA_OP))
	{
		command_fail(si, fault_noprivs, "You are not authorized to perform this operation.");
		return;
	}
	
	if (metadata_find(mc, METADATA_CHANNEL, "private:close:closer"))
	{
		command_fail(si, fault_noprivs, "\2%s\2 is closed.", chan);
		return;
	}

	/* figure out who we're going to op */
	if (!nick)
		tu = si->su;
	else
	{
		if (!(tu = user_find_named(nick)))
		{
			command_fail(si, fault_nosuch_target, "\2%s\2 is not online.", nick);
			return;
		}
	}

	if (is_internal_client(tu))
		return;

	/* SECURE check; we can skip this if sender == target, because we already verified */
	if ((si->su != tu) && (mc->flags & MC_SECURE) && !chanacs_user_has_flag(mc, tu, CA_OP) && !chanacs_user_has_flag(mc, tu, CA_AUTOOP))
	{
		command_fail(si, fault_noprivs, "You are not authorized to perform this operation.", mc->name);
		command_fail(si, fault_noprivs, "\2%s\2 has the SECURE option enabled, and \2%s\2 does not have appropriate access.", mc->name, tu->nick);
		return;
	}

	cu = chanuser_find(mc->chan, tu);
	if (!cu)
	{
		command_fail(si, fault_nosuch_target, "\2%s\2 is not on \2%s\2.", tu->nick, mc->name);
		return;
	}

	modestack_mode_param(chansvs.nick, chan, MTYPE_ADD, 'o', CLIENT_NAME(tu));
	cu->modes |= CMODE_OP;

	/* TODO: Add which username had access to perform the command */
	if (tu != si->su)
		notice(chansvs.nick, tu->nick, "You have been opped on %s by %s", mc->name, si->su->nick);

	logcommand(si, CMDLOG_SET, "%s OP %s!%s@%s", mc->name, tu->nick, tu->user, tu->vhost);
	if (!chanuser_find(mc->chan, si->su))
		command_success_nodata(si, "\2%s\2 has been opped on \2%s\2.", tu->nick, mc->name);
}

static void cs_cmd_deop(sourceinfo_t *si, int parc, char *parv[])
{
	char *chan = parv[0];
	char *nick = parv[1];
	mychan_t *mc;
	user_t *tu;
	chanuser_t *cu;

	if (!chan)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "DEOP");
		command_fail(si, fault_needmoreparams, "Syntax: DEOP <#channel> [nickname]");
		return;
	}

	mc = mychan_find(chan);
	if (!mc)
	{
		command_fail(si, fault_nosuch_target, "\2%s\2 is not registered.", chan);
		return;
	}

	si->su = user_find_named(si->su->nick);
	if (!chanacs_user_has_flag(mc, si->su, CA_OP))
	{
		command_fail(si, fault_noprivs, "You are not authorized to perform this operation.");
		return;
	}
	
	if (metadata_find(mc, METADATA_CHANNEL, "private:close:closer"))
	{
		command_fail(si, fault_noprivs, "\2%s\2 is closed.", chan);
		return;
	}

	/* figure out who we're going to deop */
	if (!nick)
		tu = si->su;
	else
	{
		if (!(tu = user_find_named(nick)))
		{
			command_fail(si, fault_nosuch_target, "\2%s\2 is not online.", nick);
			return;
		}
	}

	if (is_internal_client(tu))
		return;

	cu = chanuser_find(mc->chan, tu);
	if (!cu)
	{
		command_fail(si, fault_nosuch_target, "\2%s\2 is not on \2%s\2.", tu->nick, mc->name);
		return;
	}

	modestack_mode_param(chansvs.nick, chan, MTYPE_DEL, 'o', CLIENT_NAME(tu));
	cu->modes &= ~CMODE_OP;

	/* TODO: Add which username had access to perform the command */
	if (tu != si->su)
		notice(chansvs.nick, tu->nick, "You have been deopped on %s by %s", mc->name, si->su->nick);

	logcommand(si, CMDLOG_SET, "%s DEOP %s!%s@%s", mc->name, tu->nick, tu->user, tu->vhost);
	if (!chanuser_find(mc->chan, si->su))
		command_success_nodata(si, "\2%s\2 has been deopped on \2%s\2.", tu->nick, mc->name);
}

static void cs_fcmd_op(char *origin, char *chan)
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

	u = user_find_named(origin);
	if (!chanacs_user_has_flag(mc, u, CA_OP))
	{
		notice(chansvs.nick, origin, "You are not authorized to perform this operation.");
		return;
	}
	
	if (metadata_find(mc, METADATA_CHANNEL, "private:close:closer"))
	{
		notice(chansvs.nick, origin, "\2%s\2 is closed.", chan);
		return;
	}

	/* figure out who we're going to op */
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
		if ((u != tu) && (mc->flags & MC_SECURE) && !chanacs_user_has_flag(mc, tu, CA_OP) && !chanacs_user_has_flag(mc, tu, CA_AUTOOP))
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

		modestack_mode_param(chansvs.nick, chan, MTYPE_ADD, 'o', CLIENT_NAME(tu));
		cu->modes |= CMODE_OP;
		logcommand_user(chansvs.me, u, CMDLOG_SET, "%s OP %s!%s@%s", mc->name, tu->nick, tu->user, tu->vhost);
	} while ((nick = strtok(NULL, " ")) != NULL);
}

static void cs_fcmd_deop(char *origin, char *chan)
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

	u = user_find_named(origin);
	if (!chanacs_user_has_flag(mc, u, CA_OP))
	{
		notice(chansvs.nick, origin, "You are not authorized to perform this operation.");
		return;
	}
	
	if (metadata_find(mc, METADATA_CHANNEL, "private:close:closer"))
	{
		notice(chansvs.nick, origin, "\2%s\2 is closed.", chan);
		return;
	}

	/* figure out who we're going to deop */
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

		modestack_mode_param(chansvs.nick, chan, MTYPE_DEL, 'o', CLIENT_NAME(tu));
		cu->modes &= ~CMODE_OP;
		logcommand_user(chansvs.me, u, CMDLOG_SET, "%s DEOP %s!%s@%s", mc->name, tu->nick, tu->user, tu->vhost);
	} while ((nick = strtok(NULL, " ")) != NULL);
}

