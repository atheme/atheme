/*
 * Copyright (c) 2005-2006 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the ChanServ WHY function.
 *
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/why", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_why(sourceinfo_t *si, int parc, char *parv[]);

command_t cs_why = { "WHY", N_("Explains channel access logic."),
		     AC_NONE, 2, cs_cmd_why, { .path = "cservice/why" } };

void _modinit(module_t *m)
{
	service_named_bind_command("chanserv", &cs_why);
}

void _moddeinit()
{
	service_named_unbind_command("chanserv", &cs_why);
}

static void cs_cmd_why(sourceinfo_t *si, int parc, char *parv[])
{
	char *chan = parv[0];
	char *targ = parv[1];
	mychan_t *mc;
	user_t *u;
	myuser_t *mu;
	mowgli_node_t *n;
	chanacs_t *ca;
	metadata_t *md;
	int operoverride = 0;
	int fl = 0;

	if (!chan)
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
		command_fail(si, fault_nosuch_target, _("Channel \2%s\2 is not registered."),
			chan);
		return;
	}
	
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

	if (metadata_find(mc, "private:close:closer"))
	{
		command_fail(si, fault_noprivs, _("\2%s\2 is closed."), chan);
		return;
	}

	if (operoverride)
		logcommand(si, CMDLOG_ADMIN, "WHY: \2%s!%s@%s\2 on \2%s\2 (oper override)", u->nick, u->user, u->vhost, mc->name);
	else
		logcommand(si, CMDLOG_GET, "WHY: \2%s!%s@%s\2 on \2%s\2", u->nick, u->user, u->vhost, mc->name);

	if (u->myuser != NULL)
	{
		MOWGLI_ITER_FOREACH(n, mc->chanacs.head)
		{
       	        	ca = (chanacs_t *)n->data;

			if (ca->entity == entity(u->myuser))
			{
				fl |= ca->level;
				command_success_nodata(si,
					"\2%s\2 has flags \2%s\2 in \2%s\2 because they are logged in as \2%s\2.",
					u->nick, bitmask_to_flags2(ca->level, 0), mc->name, ca->entity->name);
				if (ca->level & CA_AKICK)
				{
					md = metadata_find(ca, "reason");
					if (md != NULL)
						command_success_nodata(si, "Ban reason: %s", md->value);
				}
			}
			else if (isgroup(ca->entity))
			{
				entity_chanacs_validation_vtable_t *vt;

				vt = myentity_get_chanacs_validator(ca->entity);
				if (vt->match_entity(ca, entity(u->myuser)) != NULL)
				{
					fl |= ca->level;
					command_success_nodata(si,
						"\2%s\2 has flags \2%s\2 in \2%s\2 because they are a member of \2%s\2.",
						u->nick, bitmask_to_flags2(ca->level, 0), mc->name, ca->entity->name);
					if (ca->level & CA_AKICK)
					{
						md = metadata_find(ca, "reason");
						if (md != NULL)
							command_success_nodata(si, "Ban reason: %s", md->value);
					}
				}
			}
		}
	}
	for (n = next_matching_host_chanacs(mc, u, mc->chanacs.head); n != NULL; n = next_matching_host_chanacs(mc, u, n->next))
	{
		ca = n->data;
		fl |= ca->level;
		command_success_nodata(si,
				"\2%s\2 has flags \2%s\2 in \2%s\2 because they match the mask \2%s\2.",
				u->nick, bitmask_to_flags2(ca->level, 0), mc->name, ca->host);
		if (ca->level & CA_AKICK)
		{
			md = metadata_find(ca, "reason");
			if (md != NULL)
				command_success_nodata(si, "Ban reason: %s", md->value);
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
	if ((fl & (CA_AKICK | CA_REMOVE)) == (CA_AKICK | CA_REMOVE))
		command_success_nodata(si, _("+r exempts from +b."));
	else if (fl == 0)
		command_success_nodata(si, _("\2%s\2 has no special access to \2%s\2."),
				u->nick, mc->name);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
