/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService KICK functions.
 *
 * $Id: clear_bans.c 7771 2007-03-03 12:46:36Z pippijn $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/clear_bans", FALSE, _modinit, _moddeinit,
	"$Id: clear_bans.c 7771 2007-03-03 12:46:36Z pippijn $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_clear_bans(sourceinfo_t *si, int parc, char *parv[]);

command_t cs_clear_bans = { "BANS", "Clears bans or other lists of a channel.",
	AC_NONE, 2, cs_cmd_clear_bans };

list_t *cs_clear_cmds;
list_t *cs_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(cs_clear_cmds, "chanserv/clear", "cs_clear_cmds");
	MODULE_USE_SYMBOL(cs_helptree, "chanserv/main", "cs_helptree");

	command_add(&cs_clear_bans, cs_clear_cmds);
	help_addentry(cs_helptree, "CLEAR BANS", "help/cservice/clear_bans", NULL);	
}

void _moddeinit()
{
	command_delete(&cs_clear_bans, cs_clear_cmds);

	help_delentry(cs_helptree, "CLEAR BANS");
}

static void cs_cmd_clear_bans(sourceinfo_t *si, int parc, char *parv[])
{
	channel_t *c;
	mychan_t *mc = mychan_find(parv[0]);
	chanban_t *cb;
	node_t *n, *tn;
	char *item = parv[1], *p;
	int hits;

	if (item == NULL)
		item = "b";
	if (*item == '+' || *item == '-')
		item++;
	if (!strcmp(item, "*"))
		item = ircd->ban_like_modes;
	for (p = item; *p != '\0'; p++)
	{
		if (!strchr(ircd->ban_like_modes, *p))
		{
			command_fail(si, fault_badparams, "Invalid mode; valid ones are %s.", ircd->ban_like_modes);
			return;
		}
	}
	if (*item == '\0')
	{
		command_fail(si, fault_badparams, "Invalid mode; valid ones are %s.", ircd->ban_like_modes);
		return;
	}

	if (!(c = channel_find(parv[0])))
	{
		command_fail(si, fault_nosuch_target, "\2%s\2 does not exist.", parv[0]);
		return;
	}

	if (!mc)
	{
		command_fail(si, fault_nosuch_target, "\2%s\2 is not registered.", c->name);
		return;
	}

	if (!chanacs_source_has_flag(mc, si, CA_RECOVER))
	{
		command_fail(si, fault_noprivs, "You are not authorized to perform this operation.");
		return;
	}
	
	if (metadata_find(mc, METADATA_CHANNEL, "private:close:closer"))
	{
		command_fail(si, fault_noprivs, "\2%s\2 is closed.", parv[0]);
		return;
	}

	hits = 0;
	LIST_FOREACH_SAFE(n, tn, c->bans.head)
	{
		cb = n->data;
		if (!strchr(item, cb->type))
			continue;
		modestack_mode_param(chansvs.nick, c->name, MTYPE_DEL, cb->type, cb->mask);
		chanban_delete(cb);
		hits++;
	}

	logcommand(si, CMDLOG_DO, "%s CLEAR BANS %s",
			mc->name, item);

	command_success_nodata(si, "Cleared %s modes on \2%s\2 (%d removed).",
			item, parv[0], hits);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:noexpandtab
 */
