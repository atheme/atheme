/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService OP functions.
 *
 * $Id: halfop.c 7035 2006-11-02 20:13:35Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/halfop", FALSE, _modinit, _moddeinit,
	"$Id: halfop.c 7035 2006-11-02 20:13:35Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_halfop(sourceinfo_t *si, int parc, char *parv[]);
static void cs_cmd_dehalfop(sourceinfo_t *si, int parc, char *parv[]);

command_t cs_halfop = { "HALFOP", "Gives channel halfops to a user.",
                        AC_NONE, 2, cs_cmd_halfop };
command_t cs_dehalfop = { "DEHALFOP", "Removes channel halfops from a user.",
                        AC_NONE, 2, cs_cmd_dehalfop };

list_t *cs_cmdtree;
list_t *cs_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(cs_cmdtree, "chanserv/main", "cs_cmdtree");
	MODULE_USE_SYMBOL(cs_helptree, "chanserv/main", "cs_helptree");

        command_add(&cs_halfop, cs_cmdtree);
        command_add(&cs_dehalfop, cs_cmdtree);

	help_addentry(cs_helptree, "HALFOP", "help/cservice/op_voice", NULL);
	help_addentry(cs_helptree, "DEHALFOP", "help/cservice/op_voice", NULL);
}

void _moddeinit()
{
	command_delete(&cs_halfop, cs_cmdtree);
	command_delete(&cs_dehalfop, cs_cmdtree);

	help_delentry(cs_helptree, "HALFOP");
	help_delentry(cs_helptree, "DEHALFOP");
}

static void cs_cmd_halfop(sourceinfo_t *si, int parc, char *parv[])
{
	char *chan = parv[0];
	char *nick = parv[1];
	mychan_t *mc;
	user_t *tu;
	chanuser_t *cu;

	if (!ircd->uses_halfops)
	{
		command_fail(si, fault_noprivs, "Your IRC server does not support halfops.");
		return;
	}

	if (!chan)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "HALFOP");
		command_fail(si, fault_needmoreparams, "Syntax: HALFOP <#channel> [nickname]");
		return;
	}

	mc = mychan_find(chan);
	if (!mc)
	{
		command_fail(si, fault_nosuch_target, "\2%s\2 is not registered.", chan);
		return;
	}

	if (!chanacs_source_has_flag(mc, si, CA_HALFOP))
	{
		command_fail(si, fault_noprivs, "You are not authorized to perform this operation.");
		return;
	}
	
	if (metadata_find(mc, METADATA_CHANNEL, "private:close:closer"))
	{
		command_fail(si, fault_noprivs, "\2%s\2 is closed.", chan);
		return;
	}

	/* figure out who we're going to halfop */
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
	if ((si->su != tu) && (mc->flags & MC_SECURE) && !chanacs_user_has_flag(mc, tu, CA_HALFOP) && !chanacs_user_has_flag(mc, tu, CA_AUTOHALFOP))
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

	modestack_mode_param(chansvs.nick, chan, MTYPE_ADD, 'h', CLIENT_NAME(tu));
	cu->modes |= ircd->halfops_mode;

	if (si->c == NULL && tu != si->su)
		notice(chansvs.nick, tu->nick, "You have been halfopped on %s by %s", mc->name, get_source_name(si));

	logcommand(si, CMDLOG_SET, "%s HALFOP %s!%s@%s", mc->name, tu->nick, tu->user, tu->vhost);
	if (!chanuser_find(mc->chan, si->su))
		command_success_nodata(si, "\2%s\2 has been halfopped on \2%s\2.", tu->nick, mc->name);
}

static void cs_cmd_dehalfop(sourceinfo_t *si, int parc, char *parv[])
{
	char *chan = parv[0];
	char *nick = parv[1];
	mychan_t *mc;
	user_t *tu;
	chanuser_t *cu;

	if (!ircd->uses_halfops)
	{
		command_fail(si, fault_noprivs, "Your IRC server does not support halfops.");
		return;
	}

	if (!chan)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "DEHALFOP");
		command_fail(si, fault_needmoreparams, "Syntax: DEHALFOP <#channel> [nickname]");
		return;
	}

	mc = mychan_find(chan);
	if (!mc)
	{
		command_fail(si, fault_nosuch_target, "\2%s\2 is not registered.", chan);
		return;
	}

	if (!chanacs_source_has_flag(mc, si, CA_HALFOP))
	{
		command_fail(si, fault_noprivs, "You are not authorized to perform this operation.");
		return;
	}
	
	if (metadata_find(mc, METADATA_CHANNEL, "private:close:closer"))
	{
		command_fail(si, fault_noprivs, "\2%s\2 is closed.", chan);
		return;
	}

	/* figure out who we're going to dehalfop */
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

	modestack_mode_param(chansvs.nick, chan, MTYPE_DEL, 'h', CLIENT_NAME(tu));
	cu->modes &= ~ircd->halfops_mode;

	if (si->c == NULL && tu != si->su)
		notice(chansvs.nick, tu->nick, "You have been dehalfopped on %s by %s", mc->name, get_source_name(si));

	logcommand(si, CMDLOG_SET, "%s DEHALFOP %s!%s@%s", mc->name, tu->nick, tu->user, tu->vhost);
	if (!chanuser_find(mc->chan, si->su))
		command_success_nodata(si, "\2%s\2 has been dehalfopped on \2%s\2.", tu->nick, mc->name);
}

