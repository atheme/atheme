/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService AKICK functions.
 *
 * $Id: akick.c 8103 2007-04-04 22:51:10Z jilles $
 */

#include "atheme.h"

static void cs_cmd_akick(sourceinfo_t *si, int parc, char *parv[]);

DECLARE_MODULE_V1
(
	"chanserv/akick", FALSE, _modinit, _moddeinit,
	"$Id: akick.c 8103 2007-04-04 22:51:10Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

command_t cs_akick = { "AKICK", N_("Manipulates a channel's AKICK list."),
                        AC_NONE, 3, cs_cmd_akick };

list_t *cs_cmdtree;
list_t *cs_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(cs_cmdtree, "chanserv/main", "cs_cmdtree");
	MODULE_USE_SYMBOL(cs_helptree, "chanserv/main", "cs_helptree");

        command_add(&cs_akick, cs_cmdtree);

	help_addentry(cs_helptree, "AKICK", "help/cservice/akick", NULL);
}

void _moddeinit()
{
	command_delete(&cs_akick, cs_cmdtree);
	help_delentry(cs_helptree, "AKICK");
}

void cs_cmd_akick(sourceinfo_t *si, int parc, char *parv[])
{
	myuser_t *mu;
	mychan_t *mc;
	chanacs_t *ca, *ca2;
	node_t *n;
	int operoverride = 0;
	char *chan = parv[0];
	char *cmd = parv[1];
	char *uname = parv[2];

	if (!cmd || !chan)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "AKICK");
		command_fail(si, fault_needmoreparams, _("Syntax: AKICK <#channel> ADD|DEL|LIST <nickname|hostmask>"));
		return;
	}

	if ((strcasecmp("LIST", cmd)) && (!uname))
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "AKICK");
		command_fail(si, fault_needmoreparams, _("Syntax: AKICK <#channel> ADD|DEL|LIST <nickname|hostmask>"));
		return;
	}

	/* make sure they're registered, logged in
	 * and the founder of the channel before
	 * we go any further.
	 */
	if (!si->smu)
	{
		/* if they're opers and just want to LIST, they don't have to log in */
		if (!(has_priv(si, PRIV_CHAN_AUSPEX) && !strcasecmp("LIST", cmd)))
		{
			command_fail(si, fault_noprivs, _("You are not logged in."));
			return;
		}
	}

	mc = mychan_find(chan);
	if (!mc)
	{
		command_fail(si, fault_nosuch_target, _("\2%s\2 is not registered."), chan);
		return;
	}
	
	if (metadata_find(mc, METADATA_CHANNEL, "private:close:closer"))
	{
		command_fail(si, fault_noprivs, _("\2%s\2 is closed."), chan);
		return;
	}

	/* ADD */
	if (!strcasecmp("ADD", cmd))
	{
		if ((chanacs_source_flags(mc, si) & (CA_FLAGS | CA_REMOVE)) != (CA_FLAGS | CA_REMOVE))
		{
			command_fail(si, fault_noprivs, _("You are not authorized to perform this operation."));
			return;
		}

		mu = myuser_find_ext(uname);
		if (!mu)
		{
			/* we might be adding a hostmask */
			if (!validhostmask(uname))
			{
				command_fail(si, fault_badparams, _("\2%s\2 is neither a nickname nor a hostmask."), uname);
				return;
			}

			uname = collapse(uname);

			ca = chanacs_find_host_literal(mc, uname, 0);
			if (ca != NULL)
			{
				if (ca->level & CA_AKICK)
					command_fail(si, fault_nochange, _("\2%s\2 is already on the AKICK list for \2%s\2"), uname, mc->name);
				else
					command_fail(si, fault_alreadyexists, _("\2%s\2 already has flags \2%s\2 on \2%s\2"), uname, bitmask_to_flags(ca->level, chanacs_flags), mc->name);
				return;
			}

			ca = chanacs_find_host(mc, uname, CA_AKICK);
			if (ca != NULL)
			{
				command_fail(si, fault_nochange, _("The more general mask \2%s\2 is already on the AKICK list for \2%s\2"), ca->host, mc->name);
				return;
			}

			/* ok to use chanacs_add_host() here, already
			 * verified it doesn't exist yet */
			ca2 = chanacs_add_host(mc, uname, CA_AKICK, CURRTIME);

			hook_call_event("channel_akick_add", ca2);

			verbose(mc, "\2%s\2 added \2%s\2 to the AKICK list.", get_source_name(si), uname);
			logcommand(si, CMDLOG_SET, "%s AKICK ADD %s", mc->name, uname);

			command_success_nodata(si, _("\2%s\2 has been added to the AKICK list for \2%s\2."), uname, mc->name);

			return;
		}
		else
		{
			if ((ca = chanacs_find(mc, mu, 0x0)))
			{
				if (ca->level & CA_AKICK)
					command_fail(si, fault_nochange, _("\2%s\2 is already on the AKICK list for \2%s\2"), mu->name, mc->name);
				else
					command_fail(si, fault_alreadyexists, _("\2%s\2 already has flags \2%s\2 on \2%s\2"), mu->name, bitmask_to_flags(ca->level, chanacs_flags), mc->name);
				return;
			}

			ca2 = chanacs_add(mc, mu, CA_AKICK, CURRTIME);

			hook_call_event("channel_akick_add", ca2);

			command_success_nodata(si, _("\2%s\2 has been added to the AKICK list for \2%s\2."), mu->name, mc->name);

			verbose(mc, "\2%s\2 added \2%s\2 to the AKICK list.", get_source_name(si), mu->name);
			logcommand(si, CMDLOG_SET, "%s AKICK ADD %s", mc->name, mu->name);

			return;
		}
	}
	else if (!strcasecmp("DEL", cmd))
	{
		if ((chanacs_source_flags(mc, si) & (CA_FLAGS | CA_REMOVE)) != (CA_FLAGS | CA_REMOVE))
		{
			command_fail(si, fault_noprivs, _("You are not authorized to perform this operation."));
			return;
		}

		mu = myuser_find_ext(uname);
		if (!mu)
		{
			/* we might be deleting a hostmask */
			ca = chanacs_find_host_literal(mc, uname, CA_AKICK);
			if (ca == NULL)
			{
				ca = chanacs_find_host(mc, uname, CA_AKICK);
				if (ca != NULL)
					command_fail(si, fault_nosuch_key, _("\2%s\2 is not on the AKICK list for \2%s\2, however \2%s\2 is."), uname, mc->name, ca->host);
				else
					command_fail(si, fault_nosuch_key, _("\2%s\2 is not on the AKICK list for \2%s\2."), uname, mc->name);
				return;
			}

			chanacs_modify_simple(ca, 0, CA_AKICK);
			chanacs_close(ca);

			verbose(mc, "\2%s\2 removed \2%s\2 from the AKICK list.", get_source_name(si), uname);
			logcommand(si, CMDLOG_SET, "%s AKICK DEL %s", mc->name, uname);

			command_success_nodata(si, _("\2%s\2 has been removed from the AKICK list for \2%s\2."), uname, mc->name);

			return;
		}

		if (!(ca = chanacs_find(mc, mu, CA_AKICK)))
		{
			command_fail(si, fault_nosuch_key, _("\2%s\2 is not on the AKICK list for \2%s\2."), mu->name, mc->name);
			return;
		}

		chanacs_modify_simple(ca, 0, CA_AKICK);
		chanacs_close(ca);

		command_success_nodata(si, _("\2%s\2 has been removed from the AKICK list for \2%s\2."), mu->name, mc->name);
		logcommand(si, CMDLOG_SET, "%s AKICK DEL %s", mc->name, mu->name);

		verbose(mc, "\2%s\2 removed \2%s\2 from the AKICK list.", get_source_name(si), mu->name);

		return;
	}
	else if (!strcasecmp("LIST", cmd))
	{
		int i = 0;

		if (!chanacs_source_has_flag(mc, si, CA_ACLVIEW))
		{
			if (has_priv(si, PRIV_CHAN_AUSPEX))
				operoverride = 1;
			else
			{
				command_fail(si, fault_noprivs, _("You are not authorized to perform this operation."));
				return;
			}
		}
		command_success_nodata(si, _("AKICK list for \2%s\2:"), mc->name);

		LIST_FOREACH(n, mc->chanacs.head)
		{
			ca = (chanacs_t *)n->data;

			if (ca->level == CA_AKICK)
			{
				if (ca->myuser == NULL)
					command_success_nodata(si, _("%d: \2%s\2 [modified: %s ago]"), ++i, ca->host, time_ago(ca->ts));
				else if (LIST_LENGTH(&ca->myuser->logins) > 0)
					command_success_nodata(si, _("%d: \2%s\2 (logged in) [modified: %s ago]"), ++i, ca->myuser->name, time_ago(ca->ts));
				else
					command_success_nodata(si, _("%d: \2%s\2 (not logged in) [modified: %s ago]"), ++i, ca->myuser->name, time_ago(ca->ts));
			}

		}

		command_success_nodata(si, _("Total of \2%d\2 %s in \2%s\2's AKICK list."), i, (i == 1) ? "entry" : "entries", mc->name);
		if (operoverride)
			logcommand(si, CMDLOG_ADMIN, "%s AKICK LIST (oper override)", mc->name);
		else
			logcommand(si, CMDLOG_GET, "%s AKICK LIST", mc->name);
	}
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
