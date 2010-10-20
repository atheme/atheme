/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService AKICK functions.
 *
 */

#include "atheme.h"

static void cs_cmd_akick(sourceinfo_t *si, int parc, char *parv[]);

DECLARE_MODULE_V1
(
	"chanserv/akick", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

command_t cs_akick = { "AKICK", N_("Manipulates a channel's AKICK list."),
                        AC_NONE, 4, cs_cmd_akick, { .path = "cservice/akick" } };

void _modinit(module_t *m)
{
        service_named_bind_command("chanserv", &cs_akick);
}

void _moddeinit()
{
	service_named_unbind_command("chanserv", &cs_akick);
}

void cs_cmd_akick(sourceinfo_t *si, int parc, char *parv[])
{
	myentity_t *mt;
	mychan_t *mc;
	chanacs_t *ca, *ca2;
	metadata_t *md;
	mowgli_node_t *n;
	int operoverride = 0;
	char *chan;
	char *cmd;
	char *uname = parv[2];
	char *reason = parv[3];
	const char *ago;

	if (parc < 2)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "AKICK");
		command_fail(si, fault_needmoreparams, _("Syntax: AKICK <#channel> ADD|DEL|LIST <nickname|hostmask> [reason]"));
		return;
	}

	if (parv[0][0] == '#')
		chan = parv[0], cmd = parv[1];
	else if (parv[1][0] == '#')
		cmd = parv[0], chan = parv[1];
	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "AKICK");
		command_fail(si, fault_badparams, _("Syntax: AKICK <#channel> ADD|DEL|LIST <nickname|hostmask> [reason]"));
		return;
	}

	if ((strcasecmp("LIST", cmd)) && (!uname))
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "AKICK");
		command_fail(si, fault_needmoreparams, _("Syntax: AKICK <#channel> ADD|DEL|LIST <nickname|hostmask> [reason]"));
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
		command_fail(si, fault_nosuch_target, _("Channel \2%s\2 is not registered."), chan);
		return;
	}
	
	if (metadata_find(mc, "private:close:closer"))
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

		mt = myentity_find_ext(uname);
		if (!mt)
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
					command_fail(si, fault_alreadyexists, _("\2%s\2 already has flags \2%s\2 on \2%s\2"), uname, bitmask_to_flags(ca->level), mc->name);
				return;
			}

			ca = chanacs_find_host(mc, uname, CA_AKICK);
			if (ca != NULL)
			{
				command_fail(si, fault_nochange, _("The more general mask \2%s\2 is already on the AKICK list for \2%s\2"), ca->host, mc->name);
				return;
			}

			/* new entry */
			ca2 = chanacs_open(mc, NULL, uname, true);
			if (chanacs_is_table_full(ca2))
			{
				command_fail(si, fault_toomany, _("Channel %s access list is full."), mc->name);
				chanacs_close(ca2);
				return;
			}
			chanacs_modify_simple(ca2, CA_AKICK, 0);
			if (reason != NULL)
				metadata_add(ca2, "reason", reason);

			hook_call_channel_akick_add(ca2);
			chanacs_close(ca2);

			verbose(mc, "\2%s\2 added \2%s\2 to the AKICK list.", get_source_name(si), uname);
			logcommand(si, CMDLOG_SET, "AKICK:ADD: \2%s\2 on \2%s\2", uname, mc->name);

			command_success_nodata(si, _("\2%s\2 has been added to the AKICK list for \2%s\2."), uname, mc->name);

			return;
		}
		else
		{
			if ((ca = chanacs_find(mc, mt, 0x0)))
			{
				if (ca->level & CA_AKICK)
					command_fail(si, fault_nochange, _("\2%s\2 is already on the AKICK list for \2%s\2"), mt->name, mc->name);
				else
					command_fail(si, fault_alreadyexists, _("\2%s\2 already has flags \2%s\2 on \2%s\2"), mt->name, bitmask_to_flags(ca->level), mc->name);
				return;
			}

			/* new entry */
			ca2 = chanacs_open(mc, mt, NULL, true);
			if (chanacs_is_table_full(ca2))
			{
				command_fail(si, fault_toomany, _("Channel %s access list is full."), mc->name);
				chanacs_close(ca2);
				return;
			}
			chanacs_modify_simple(ca2, CA_AKICK, 0);
			if (reason != NULL)
				metadata_add(ca2, "reason", reason);

			hook_call_channel_akick_add(ca2);
			chanacs_close(ca2);

			command_success_nodata(si, _("\2%s\2 has been added to the AKICK list for \2%s\2."), mt->name, mc->name);

			verbose(mc, "\2%s\2 added \2%s\2 to the AKICK list.", get_source_name(si), mt->name);
			logcommand(si, CMDLOG_SET, "AKICK:ADD: \2%s\2 on \2%s\2", mt->name, mc->name);

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

		mt = myentity_find_ext(uname);
		if (!mt)
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
			logcommand(si, CMDLOG_SET, "AKICK:DEL: \2%s\2 on \2%s\2", uname, mc->name);

			command_success_nodata(si, _("\2%s\2 has been removed from the AKICK list for \2%s\2."), uname, mc->name);

			return;
		}

		if (!(ca = chanacs_find(mc, mt, CA_AKICK)))
		{
			command_fail(si, fault_nosuch_key, _("\2%s\2 is not on the AKICK list for \2%s\2."), mt->name, mc->name);
			return;
		}

		chanacs_modify_simple(ca, 0, CA_AKICK);
		chanacs_close(ca);

		command_success_nodata(si, _("\2%s\2 has been removed from the AKICK list for \2%s\2."), mt->name, mc->name);
		logcommand(si, CMDLOG_SET, "AKICK:DEL: \2%s\2 on \2%s\2", mt->name, mc->name);

		verbose(mc, "\2%s\2 removed \2%s\2 from the AKICK list.", get_source_name(si), mt->name);

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

		MOWGLI_ITER_FOREACH(n, mc->chanacs.head)
		{
			ca = (chanacs_t *)n->data;

			if (ca->level == CA_AKICK)
			{
				md = metadata_find(ca, "reason");
				ago = ca->tmodified ? time_ago(ca->tmodified) : "?";
				if (ca->entity == NULL)
					command_success_nodata(si, _("%d: \2%s\2  %s [modified: %s ago]"),
							++i, ca->host,
							md ? md->value : "", ago);
				else if (isuser(ca->entity) && MOWGLI_LIST_LENGTH(&user(ca->entity)->logins) > 0)
					command_success_nodata(si, _("%d: \2%s\2 (logged in)  %s [modified: %s ago]"),
							++i, ca->entity->name,
							md ? md->value : "", ago);
				else
					command_success_nodata(si, _("%d: \2%s\2 (not logged in)  %s [modified: %s ago]"),
							++i, ca->entity->name,
							md ? md->value : "", ago);
			}

		}

		command_success_nodata(si, _("Total of \2%d\2 %s in \2%s\2's AKICK list."), i, (i == 1) ? "entry" : "entries", mc->name);
		if (operoverride)
			logcommand(si, CMDLOG_ADMIN, "AKICK:LIST: \2%s\2 (oper override)", mc->name);
		else
			logcommand(si, CMDLOG_GET, "AKICK:LIST: \2%s\2", mc->name);
	}
	else
		command_fail(si, fault_badparams, _("Invalid command. Use \2/%s%s help\2 for a command listing."), (ircd->uses_rcommand == false) ? "msg " : "", chansvs.me->disp);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
