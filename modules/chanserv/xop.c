/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2007 Atheme Project (http://atheme.org/)
 *
 * This file contains code for the CService XOP functions.
 */

#include <atheme.h>
#include "chanserv.h"

static void
cs_xop_do_add(struct sourceinfo *si, struct mychan *mc, struct myentity *mt, char *target, unsigned int level, const char *leveldesc, unsigned int restrictflags)
{
	struct chanacs *ca;
	unsigned int addflags = level, removeflags = ~level;
	bool isnew;
	struct hook_channel_acl_req req;

	if (!mt)
	{
		// we might be adding a hostmask
		if (!validhostmask(target))
		{
			command_fail(si, fault_badparams, _("\2%s\2 is neither a registered account nor a hostmask."), target);
			return;
		}

		target = collapse(target);
		ca = chanacs_open(mc, NULL, target, true, entity(si->smu));
		if (ca->level == level)
		{
			command_fail(si, fault_nochange, _("\2%s\2 is already on the %s list for \2%s\2."), target, leveldesc, mc->name);
			return;
		}
		isnew = ca->level == 0;

		if (isnew && chanacs_is_table_full(ca))
		{
			command_fail(si, fault_toomany, _("Channel \2%s\2 access list is full."), mc->name);
			chanacs_close(ca);
			return;
		}

		req.ca = ca;
		req.oldlevel = ca->level;

		if (!chanacs_modify(ca, &addflags, &removeflags, restrictflags, si->smu))
		{
			command_fail(si, fault_noprivs, STR_NOT_AUTHORIZED);
			chanacs_close(ca);
			return;
		}

		req.newlevel = ca->level;

		hook_call_channel_acl_change(&req);

		chanacs_close(ca);

		if (!isnew)
		{
			// they have access? change it!
			logcommand(si, CMDLOG_SET, "ADD: \2%s\2 \2%s\2 on \2%s\2 (changed access)", mc->name, leveldesc, target);
			command_success_nodata(si, _("\2%s\2's access on \2%s\2 has been changed to \2%s\2."), target, mc->name, leveldesc);
			verbose(mc, "\2%s\2 changed \2%s\2's access to \2%s\2.", get_source_name(si), target, leveldesc);
		}
		else
		{
			logcommand(si, CMDLOG_SET, "ADD: \2%s\2 \2%s\2 on \2%s\2", mc->name, leveldesc, target);
			command_success_nodata(si, _("\2%s\2 has been added to the %s list for \2%s\2."), target, leveldesc, mc->name);
			verbose(mc, "\2%s\2 added \2%s\2 to the %s list.", get_source_name(si), target, leveldesc);
		}

		return;
	}

	ca = chanacs_open(mc, mt, NULL, true, entity(si->smu));

	if (ca->level & CA_FOUNDER)
	{
		command_fail(si, fault_noprivs, _("\2%s\2 is the founder for \2%s\2 and may not be added to the %s list."), mt->name, mc->name, leveldesc);
		return;
	}

	if (ca->level == level)
	{
		command_fail(si, fault_nochange, _("\2%s\2 is already on the %s list for \2%s\2."), mt->name, leveldesc, mc->name);
		return;
	}

	/* NEVEROP logic moved here
	 * Allow changing access level, but not adding
	 * -- jilles */
	if (isuser(mt) && MU_NEVEROP & user(mt)->flags && (ca->level == 0 || ca->level == CA_AKICK))
	{
		command_fail(si, fault_noprivs, _("\2%s\2 does not wish to be added to channel access lists (NEVEROP set)."), mt->name);
		chanacs_close(ca);
		return;
	}

	/*
	 * this is a little more cryptic than it used to be, but much cleaner. Functionally should be
	 * the same, with the exception that if they had access before, now it doesn't tell what it got
	 * changed from (I considered the effort to put an extra lookup in not worth it. --w00t
	 */

	// just assume there's just one entry for that user -- jilles

	isnew = ca->level == 0;
	if (isnew && chanacs_is_table_full(ca))
	{
		command_fail(si, fault_toomany, _("Channel \2%s\2 access list is full."), mc->name);
		chanacs_close(ca);
		return;
	}

	req.ca = ca;
	req.oldlevel = ca->level;

	if (!chanacs_modify(ca, &addflags, &removeflags, restrictflags, si->smu))
	{
		command_fail(si, fault_noprivs, STR_NOT_AUTHORIZED);
		chanacs_close(ca);
		return;
	}

	req.newlevel = ca->level;

	hook_call_channel_acl_change(&req);
	chanacs_close(ca);

	if (!isnew)
	{
		// they have access? change it!
		logcommand(si, CMDLOG_SET, "ADD: \2%s\2 \2%s\2 on \2%s\2 (changed access)", mc->name, leveldesc, mt->name);
		command_success_nodata(si, _("\2%s\2's access on \2%s\2 has been changed to \2%s\2."), mt->name, mc->name, leveldesc);
		verbose(mc, "\2%s\2 changed \2%s\2's access to \2%s\2.", get_source_name(si), mt->name, leveldesc);
	}
	else
	{
		// they have no access, add
		logcommand(si, CMDLOG_SET, "ADD: \2%s\2 \2%s\2 on \2%s\2", mc->name, leveldesc, mt->name);
		command_success_nodata(si, _("\2%s\2 has been added to the %s list for \2%s\2."), mt->name, leveldesc, mc->name);
		verbose(mc, "\2%s\2 added \2%s\2 to the %s list.", get_source_name(si), mt->name, leveldesc);
	}
}

static void
cs_xop_do_del(struct sourceinfo *si, struct mychan *mc, struct myentity *mt, char *target, unsigned int level, const char *leveldesc)
{
	struct chanacs *ca;
	struct hook_channel_acl_req req;

	// let's finally make this sane.. --w00t
	if (!mt)
	{
		// we might be deleting a hostmask
		if (!validhostmask(target))
		{
			command_fail(si, fault_badparams, _("\2%s\2 is neither a nickname nor a hostmask."), target);
			return;
		}

		ca = chanacs_find_host_literal(mc, target, level);
		if (ca == NULL || ca->level != level)
		{
			command_fail(si, fault_nochange, _("\2%s\2 is not on the %s list for \2%s\2."), target, leveldesc, mc->name);
			return;
		}

		req.ca = ca;
		req.oldlevel = ca->level;

		ca->level = 0;

		req.newlevel = ca->level;

		hook_call_channel_acl_change(&req);
		atheme_object_unref(ca);

		verbose(mc, "\2%s\2 removed \2%s\2 from the %s list.", get_source_name(si), target, leveldesc);
		logcommand(si, CMDLOG_SET, "DEL: \2%s\2 \2%s\2 from \2%s\2", mc->name, leveldesc, target);
		command_success_nodata(si, _("\2%s\2 has been removed from the %s list for \2%s\2."), target, leveldesc, mc->name);
		return;
	}

	if (!(ca = chanacs_find_literal(mc, mt, level)) || ca->level != level)
	{
		command_fail(si, fault_nochange, _("\2%s\2 is not on the %s list for \2%s\2."), mt->name, leveldesc, mc->name);
		return;
	}

	req.ca = ca;
	req.oldlevel = ca->level;

	ca->level = 0;

	req.newlevel = ca->level;

	hook_call_channel_acl_change(&req);
	atheme_object_unref(ca);

	command_success_nodata(si, _("\2%s\2 has been removed from the %s list for \2%s\2."), mt->name, leveldesc, mc->name);
	logcommand(si, CMDLOG_SET, "DEL: \2%s\2 \2%s\2 from \2%s\2", mc->name, leveldesc, mt->name);
	verbose(mc, "\2%s\2 removed \2%s\2 from the %s list.", get_source_name(si), mt->name, leveldesc);
}

static void
cs_xop_do_list(struct sourceinfo *si, struct mychan *mc, unsigned int level, const char *leveldesc, bool operoverride)
{
	struct chanacs *ca;
	unsigned int i = 0;
	mowgli_node_t *n;

	command_success_nodata(si, _("%s list for \2%s\2:"), leveldesc ,mc->name);
	MOWGLI_ITER_FOREACH(n, mc->chanacs.head)
	{
		ca = (struct chanacs *)n->data;
		if (ca->level == level)
		{
			if (ca->entity == NULL)
				command_success_nodata(si, "%u: \2%s\2", ++i, ca->host);
			else if (isuser(ca->entity) && MOWGLI_LIST_LENGTH(&user(ca->entity)->logins))
				command_success_nodata(si, _("%u: \2%s\2 (logged in)"), ++i, ca->entity->name);
			else
				command_success_nodata(si, _("%u: \2%s\2 (not logged in)"), ++i, ca->entity->name);
		}
	}

	command_success_nodata(si, ngettext(N_("Total of \2%u\2 entry in %s list of \2%s\2."),
	                                    N_("Total of \2%u\2 entries in %s list of \2%s\2."),
	                                    i), i, leveldesc, mc->name);

	if (operoverride)
		logcommand(si, CMDLOG_ADMIN, "LIST: \2%s\2 \2%s\2 (oper override)", mc->name, leveldesc);
	else
		logcommand(si, CMDLOG_GET, "LIST: \2%s\2 \2%s\2", mc->name, leveldesc);
}

static void
cs_xop(struct sourceinfo *si, int parc, char *parv[], const char *leveldesc)
{
	struct myentity *mt;
	struct mychan *mc;
	bool operoverride = false;
	unsigned int restrictflags;
	const char *chan = parv[0];
	const char *cmd = parv[1];
	char *uname = parv[2];
	unsigned int level;

	if (!cmd || !chan)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "XOP");
		command_fail(si, fault_needmoreparams, _("Syntax: SOP|AOP|HOP|VOP <#channel> ADD|DEL|LIST <nickname>"));
		return;
	}

	if ((strcasecmp("LIST", cmd)) && (!uname))
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "XOP");
		command_fail(si, fault_needmoreparams, _("Syntax: SOP|AOP|HOP|VOP <#channel> ADD|DEL|LIST <nickname>"));
		return;
	}

	/* make sure they're registered, logged in
	 * and the founder of the channel before
	 * we go any further.
	 */
	if (!si->smu)
	{
		// if they're opers and just want to LIST, they don't have to log in
		if (!(has_priv(si, PRIV_CHAN_AUSPEX) && !strcasecmp("LIST", cmd)))
		{
			command_fail(si, fault_noprivs, STR_NOT_LOGGED_IN);
			return;
		}
	}

	mc = mychan_find(chan);
	if (!mc)
	{
		command_fail(si, fault_nosuch_target, STR_IS_NOT_REGISTERED, chan);
		return;
	}

	if (metadata_find(mc, "private:close:closer") && (!has_priv(si, PRIV_CHAN_AUSPEX) || strcasecmp("LIST", cmd)))
	{
		command_fail(si, fault_noprivs, STR_CHANNEL_IS_CLOSED, chan);
		return;
	}

	level = get_template_flags(mc, leveldesc);
	if (level & CA_FOUNDER)
	{
		command_fail(si, fault_noprivs, _("\2%s\2 %s template has founder flag, not allowing XOP command."), chan, leveldesc);
		return;
	}

	// ADD
	if (!strcasecmp("ADD", cmd))
	{
		mt = myentity_find_ext(uname);

		// As in /cs flags, allow founder to do anything
		restrictflags = chanacs_source_flags(mc, si);
		if (restrictflags & CA_FOUNDER)
			restrictflags = ca_all;

		/* The following is a bit complicated, to allow for
		 * possible future denial of granting +f */
		if (!(restrictflags & CA_FLAGS))
		{
			command_fail(si, fault_noprivs, STR_NOT_AUTHORIZED);
			return;
		}
		restrictflags = allow_flags(mc, restrictflags);
		if ((restrictflags & level) != level)
		{
			command_fail(si, fault_noprivs, STR_NOT_AUTHORIZED);
			return;
		}
		cs_xop_do_add(si, mc, mt, uname, level, leveldesc, restrictflags);
	}

	else if (!strcasecmp("DEL", cmd))
	{
		mt = myentity_find_ext(uname);

		// As in /cs flags, allow founder to do anything -- fix for #64: allow self removal.
		restrictflags = chanacs_source_flags(mc, si);
		if (restrictflags & CA_FOUNDER || entity(si->smu) == mt)
			restrictflags = ca_all;

		/* The following is a bit complicated, to allow for
		 * possible future denial of granting +f */
		if (!(restrictflags & CA_FLAGS))
		{
			command_fail(si, fault_noprivs, STR_NOT_AUTHORIZED);
			return;
		}
		restrictflags = allow_flags(mc, restrictflags);
		if ((restrictflags & level) != level)
		{
			command_fail(si, fault_noprivs, STR_NOT_AUTHORIZED);
			return;
		}
		cs_xop_do_del(si, mc, mt, uname, level, leveldesc);
	}

	else if (!strcasecmp("LIST", cmd))
	{
		if (!(mc->flags & MC_PUBACL) && !chanacs_source_has_flag(mc, si, CA_ACLVIEW))
		{
			if (has_priv(si, PRIV_CHAN_AUSPEX))
				operoverride = true;
			else
			{
				command_fail(si, fault_noprivs, STR_NOT_AUTHORIZED);
				return;
			}
		}
		cs_xop_do_list(si, mc, level, leveldesc, operoverride);
	}
}

static void
cs_cmd_sop(struct sourceinfo *si, int parc, char *parv[])
{
	cs_xop(si, parc, parv, "SOP");
}

static void
cs_cmd_aop(struct sourceinfo *si, int parc, char *parv[])
{
	cs_xop(si, parc, parv, "AOP");
}

static void
cs_cmd_vop(struct sourceinfo *si, int parc, char *parv[])
{
	cs_xop(si, parc, parv, "VOP");
}

static void
cs_cmd_hop(struct sourceinfo *si, int parc, char *parv[])
{
	/* Don't reject the command. This helps the rare case where
	 * a network switches to a non-halfop ircd: users can still
	 * remove pre-transition HOP entries.
	 */
	if (!ircd->uses_halfops && si->su != NULL)
		notice(chansvs.nick, si->su->nick, "Warning: Your IRC server does not support halfops.");

	cs_xop(si, parc, parv, "HOP");
}

static void
cs_cmd_forcexop(struct sourceinfo *si, int parc, char *parv[])
{
	char *chan = parv[0];
	struct chanacs *ca;
	struct mychan *mc = mychan_find(chan);
	mowgli_node_t *n;
	unsigned int changes;
	unsigned int newlevel;
	const char *desc;
	unsigned int ca_sop, ca_aop, ca_hop, ca_vop;

	if (!chan)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "FORCEXOP");
		command_fail(si, fault_needmoreparams, _("Syntax: FORCEXOP <#channel>"));
		return;
	}

	if (!mc)
	{
		command_fail(si, fault_nosuch_target, STR_IS_NOT_REGISTERED, chan);
		return;
	}

	if (!is_founder(mc, entity(si->smu)))
	{
		command_fail(si, fault_noprivs, STR_NOT_AUTHORIZED);
		return;
	}

	if (metadata_find(mc, "private:close:closer"))
	{
		command_fail(si, fault_noprivs, STR_CHANNEL_IS_CLOSED, chan);
		return;
	}

	ca_sop = get_template_flags(mc, "SOP");
	ca_aop = get_template_flags(mc, "AOP");
	ca_hop = get_template_flags(mc, "HOP");
	ca_vop = get_template_flags(mc, "VOP");

	changes = 0;
	MOWGLI_ITER_FOREACH(n, mc->chanacs.head)
	{
		ca = (struct chanacs *)n->data;

		if (ca->level & CA_AKICK)
			continue;

		if (ca->level & CA_FOUNDER)
		{
			newlevel = custom_founder_check();
			desc = "Founder";
		}
		else if (!(~ca->level & ca_sop))
		{
			newlevel = ca_sop;
			desc = "SOP";
		}
		else if (ca->level == ca_aop)
		{
			newlevel = ca_aop;
			desc = "AOP";
		}
		else if (ca->level == ca_hop)
		{
			newlevel = ca_hop;
			desc = "HOP";
		}
		else if (ca->level == ca_vop)
		{
			newlevel = ca_vop;
			desc = "VOP";
		}
		else if (ca->level & (CA_SET | CA_RECOVER | CA_FLAGS))
		{
			newlevel = ca_sop;
			desc = "SOP";
		}
		else if (ca->level & (CA_OP | CA_AUTOOP | CA_REMOVE))
		{
			newlevel = ca_aop;
			desc = "AOP";
		}
		else if (ca->level & (CA_HALFOP | CA_AUTOHALFOP | CA_TOPIC))
		{
			if (ca_hop == ca_vop)
			{
				newlevel = ca_aop;
				desc = "AOP";
			}
			else
			{
				newlevel = ca_hop;
				desc = "HOP";
			}
		}
		else
		{
			newlevel = ca_vop;
			desc = "VOP";
		}
#if 0
		else
			newlevel = 0;
#endif

		if (newlevel == ca->level)
			continue;

		changes++;
		command_success_nodata(si, "%s: %s -> %s", ca->entity != NULL ? ca->entity->name : ca->host, bitmask_to_flags(ca->level), desc);
		chanacs_modify_simple(ca, newlevel, ~newlevel, si->smu);
	}
	command_success_nodata(si, ngettext(N_("FORCEXOP \2%s\2 done (\2%u\2 change)"),
	                                    N_("FORCEXOP \2%s\2 done (\2%u\2 changes)"),
	                                    changes), mc->name, changes);
	if (changes > 0)
		verbose(mc, "\2%s\2 reset access levels to XOP (\2%u\2 changes)", get_source_name(si), changes);
	logcommand(si, CMDLOG_SET, "FORCEXOP: \2%s\2 (\2%u\2 changes)", mc->name, changes);
}

static struct command cs_sop = {
	.name           = "SOP",
	.desc           = N_("Manipulates a channel SOP list."),
	.access         = AC_NONE,
	.maxparc        = 3,
	.cmd            = &cs_cmd_sop,
	.help           = { .path = "cservice/xop" },
};

static struct command cs_aop = {
	.name           = "AOP",
	.desc           = N_("Manipulates a channel AOP list."),
	.access         = AC_NONE,
	.maxparc        = 3,
	.cmd            = &cs_cmd_aop,
	.help           = { .path = "cservice/xop" },
};

static struct command cs_hop = {
	.name           = "HOP",
	.desc           = N_("Manipulates a channel HOP list."),
	.access         = AC_NONE,
	.maxparc        = 3,
	.cmd            = &cs_cmd_hop,
	.help           = { .path = "cservice/xop" },
};

static struct command cs_vop = {
	.name           = "VOP",
	.desc           = N_("Manipulates a channel VOP list."),
	.access         = AC_NONE,
	.maxparc        = 3,
	.cmd            = &cs_cmd_vop,
	.help           = { .path = "cservice/xop" },
};

static struct command cs_forcexop = {
	.name           = "FORCEXOP",
	.desc           = N_("Forces access levels to XOP levels."),
	.access         = AC_NONE,
	.maxparc        = 1,
	.cmd            = &cs_cmd_forcexop,
	.help           = { .path = "cservice/forcexop" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "chanserv/main")

	service_named_bind_command("chanserv", &cs_aop);
	service_named_bind_command("chanserv", &cs_sop);
	if (ircd != NULL && ircd->uses_halfops)
		service_named_bind_command("chanserv", &cs_hop);
	service_named_bind_command("chanserv", &cs_vop);
	service_named_bind_command("chanserv", &cs_forcexop);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("chanserv", &cs_aop);
	service_named_unbind_command("chanserv", &cs_sop);
	service_named_unbind_command("chanserv", &cs_hop);
	service_named_unbind_command("chanserv", &cs_vop);
	service_named_unbind_command("chanserv", &cs_forcexop);
}

SIMPLE_DECLARE_MODULE_V1("chanserv/xop", MODULE_UNLOAD_CAPABILITY_OK)
