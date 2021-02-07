/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2006 William Pitcock, et al.
 *
 * This file contains code for the ChanServ WHY function.
 */

#include <atheme.h>

static void
cs_cmd_why(struct sourceinfo *si, int parc, char *parv[])
{
	const char *chan = parv[0];
	const char *targ = parv[1];
	struct mychan *mc;
	struct user *u;
	struct myuser *mu;
	mowgli_node_t *n;
	struct chanacs *ca;
	const struct entity_vtable *vt;
	struct metadata *md;
	bool operoverride = false;
	int fl = 0;

	if (!chan || (!targ && si->su == NULL))
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "WHY");
		command_fail(si, fault_needmoreparams, _("Syntax: WHY <channel> [user]"));
		return;
	}

	if (!targ)
		targ = si->su->nick;

	mc = mychan_find(chan);
	u = user_find_named(targ);

	if (u == NULL)
	{
		command_fail(si, fault_nosuch_target, _("\2%s\2 is not online."),
			targ);
		return;
	}

	mu = u->myuser;

	if (mc == NULL)
	{
		command_fail(si, fault_nosuch_target, STR_IS_NOT_REGISTERED,
			chan);
		return;
	}

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

	if (metadata_find(mc, "private:close:closer"))
	{
		command_fail(si, fault_noprivs, STR_CHANNEL_IS_CLOSED, chan);
		return;
	}

	if (operoverride)
		logcommand(si, CMDLOG_ADMIN, "WHY: \2%s!%s@%s\2 on \2%s\2 (oper override)", u->nick, u->user, u->vhost, mc->name);
	else
		logcommand(si, CMDLOG_GET, "WHY: \2%s!%s@%s\2 on \2%s\2", u->nick, u->user, u->vhost, mc->name);

	bool show_akicks = ( chanacs_source_has_flag(mc, si, CA_ACLVIEW) || has_priv(si, PRIV_CHAN_AUSPEX) );

	MOWGLI_ITER_FOREACH(n, mc->chanacs.head)
	{
		ca = (struct chanacs *)n->data;

		if (ca->entity == NULL)
			continue;
		vt = myentity_get_vtable(ca->entity);
		if (ca->entity == entity(u->myuser) ||
				(vt->match_user != NULL ?
				 vt->match_user(ca->entity, u) :
				 u->myuser != NULL &&
				 vt->match_entity(ca->entity, entity(u->myuser)) ))
		{
			fl |= ca->level;
			if (ca->entity == entity(u->myuser))
				command_success_nodata(si,
					_("\2%s\2 has flags \2%s\2 in \2%s\2 because they are logged in as \2%s\2."),
					u->nick, bitmask_to_flags2(ca->level, 0), mc->name, ca->entity->name);
			else if (isgroup(ca->entity))
				command_success_nodata(si,
					_("\2%s\2 has flags \2%s\2 in \2%s\2 because they are a member of \2%s\2."),
					u->nick, bitmask_to_flags2(ca->level, 0), mc->name, ca->entity->name);
			else
				command_success_nodata(si,
					_("\2%s\2 has flags \2%s\2 in \2%s\2 because they match \2%s\2."),
					u->nick, bitmask_to_flags2(ca->level, 0), mc->name, ca->entity->name);
			if (ca->level & CA_AKICK && show_akicks)
			{
				md = metadata_find(ca, "reason");
				if (md != NULL)
					command_success_nodata(si, _("Ban reason: %s"), md->value);
			}
		}
	}
	for (n = next_matching_host_chanacs(mc, u, mc->chanacs.head); n != NULL; n = next_matching_host_chanacs(mc, u, n->next))
	{
		ca = n->data;
		fl |= ca->level;
		command_success_nodata(si,
				_("\2%s\2 has flags \2%s\2 in \2%s\2 because they match the mask \2%s\2."),
				u->nick, bitmask_to_flags2(ca->level, 0), mc->name, ca->host);
		if (ca->level & CA_AKICK && show_akicks)
		{
			md = metadata_find(ca, "reason");
			if (md != NULL)
				command_success_nodata(si, _("Ban reason: %s"), md->value);
		}
	}

	if (fl & (CA_AUTOOP | CA_AUTOHALFOP | CA_AUTOVOICE))
	{
		if (mc->flags & MC_NOOP)
			command_success_nodata(si, _("The \2%s\2 flag is set for \2%s\2, therefore no status will be given."),
					"NOOP",
					mc->name);
		else if (mu != NULL && mu->flags & MU_NOOP)
			command_success_nodata(si, _("The \2%s\2 flag is set for \2%s\2, therefore no status will be given."),
					"NOOP",
					entity(mu)->name);
	}
	if ((fl & (CA_AKICK | CA_EXEMPT)) == (CA_AKICK | CA_EXEMPT))
		command_success_nodata(si, _("\2+e\2 exempts from \2+b\2."));
	else if (fl == 0)
		command_success_nodata(si, _("\2%s\2 has no special access to \2%s\2."),
				u->nick, mc->name);
}

static struct command cs_why = {
	.name           = "WHY",
	.desc           = N_("Explains channel access logic."),
	.access         = AC_NONE,
	.maxparc        = 2,
	.cmd            = &cs_cmd_why,
	.help           = { .path = "cservice/why" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "chanserv/main")

	service_named_bind_command("chanserv", &cs_why);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("chanserv", &cs_why);
}

SIMPLE_DECLARE_MODULE_V1("chanserv/why", MODULE_UNLOAD_CAPABILITY_OK)
