/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService DROP function.
 *
 * $Id: drop.c 8321 2007-05-24 20:10:59Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/drop", FALSE, _modinit, _moddeinit,
	"$Id: drop.c 8321 2007-05-24 20:10:59Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_drop(sourceinfo_t *si, int parc, char *parv[]);

command_t cs_drop = { "DROP", N_("Drops a channel registration."),
                        AC_NONE, 1, cs_cmd_drop };

list_t *cs_cmdtree;
list_t *cs_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(cs_cmdtree, "chanserv/main", "cs_cmdtree");
	MODULE_USE_SYMBOL(cs_helptree, "chanserv/main", "cs_helptree");

        command_add(&cs_drop, cs_cmdtree);
	help_addentry(cs_helptree, "DROP", "help/cservice/drop", NULL);
}

void _moddeinit()
{
	command_delete(&cs_drop, cs_cmdtree);
	help_delentry(cs_helptree, "DROP");
}

static void cs_cmd_drop(sourceinfo_t *si, int parc, char *parv[])
{
	mychan_t *mc;
	char *name = parv[0];

	if (!name)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "DROP");
		command_fail(si, fault_needmoreparams, _("Syntax: DROP <#channel>"));
		return;
	}

	if (*name != '#')
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "DROP");
		command_fail(si, fault_badparams, _("Syntax: DROP <#channel>"));
		return;
	}

	if (!(mc = mychan_find(name)))
	{
		command_fail(si, fault_nosuch_target, _("\2%s\2 is not registered."), name);
		return;
	}

	if (si->c != NULL)
	{
		command_fail(si, fault_noprivs, _("For security reasons, you may not drop a channel registration with a fantasy command."));
		return;
	}

	if (!is_founder(mc, si->smu) && !has_priv(si, PRIV_CHAN_ADMIN))
	{
		command_fail(si, fault_noprivs, _("You are not authorized to perform this operation."));
		return;
	}

	if (metadata_find(mc, METADATA_CHANNEL, "private:close:closer") && !has_priv(si, PRIV_CHAN_ADMIN))
	{
		logcommand(si, CMDLOG_REGISTER, "%s failed DROP (closed)", mc->name);
		command_fail(si, fault_noprivs, _("The channel \2%s\2 is closed; it cannot be dropped."), mc->name);
		return;
	}

	if (mc->flags & MC_HOLD)
	{
		command_fail(si, fault_noprivs, _("The channel \2%s\2 is held; it cannot be dropped."), mc->name);
		return;
	}

	if (!is_founder(mc, si->smu))
	{
		logcommand(si, CMDLOG_ADMIN, "%s DROP", mc->name);
		wallops("%s dropped the channel \2%s\2", get_oper_name(si), name);
	}
	else
		logcommand(si, CMDLOG_REGISTER, "%s DROP", mc->name);

	snoop("DROP: \2%s\2 by \2%s\2 as \2%s\2", mc->name, get_oper_name(si), si->smu->name);

	hook_call_event("channel_drop", mc);
	if ((config_options.chan && irccasecmp(mc->name, config_options.chan)) || !config_options.chan)
		part(mc->name, chansvs.nick);
	object_unref(mc);
	command_success_nodata(si, _("The channel \2%s\2 has been dropped."), name);
	return;
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
