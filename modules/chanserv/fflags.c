/*
 * Copyright (c) 2005-2006 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService FFLAGS functions.
 *
 */

#include "atheme.h"
#include "template.h"

DECLARE_MODULE_V1
(
	"chanserv/fflags", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_fflags(sourceinfo_t *si, int parc, char *parv[]);

command_t cs_fflags = { "FFLAGS", N_("Forces a flags change on a channel."),
                        PRIV_CHAN_ADMIN, 3, cs_cmd_fflags, { .path = "cservice/fflags" } };

void _modinit(module_t *m)
{
        service_named_bind_command("chanserv", &cs_fflags);
}

void _moddeinit()
{
	service_named_unbind_command("chanserv", &cs_fflags);
}

/* FFLAGS <channel> <user> <flags> */
static void cs_cmd_fflags(sourceinfo_t *si, int parc, char *parv[])
{
	char *channel = parv[0];
	char *target = parv[1];
	char *flagstr = parv[2];
	mychan_t *mc;
	myentity_t *mt;
	unsigned int addflags, removeflags;

	if (parc < 3)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "FFLAGS");
		command_fail(si, fault_needmoreparams, _("Syntax: FFLAGS <channel> <target> <flags>"));
		return;
	}

	mc = mychan_find(channel);
	if (!mc)
	{
		command_fail(si, fault_nosuch_target, _("Channel \2%s\2 is not registered."), channel);
		return;
	}
	
	if (*flagstr == '+' || *flagstr == '-' || *flagstr == '=')
	{
		flags_make_bitmasks(flagstr, &addflags, &removeflags);
		if (addflags == 0 && removeflags == 0)
		{
			command_fail(si, fault_badparams, _("No valid flags given, use /%s%s HELP FLAGS for a list"), ircd->uses_rcommand ? "" : "msg ", chansvs.me->disp);
			return;
		}
	}
	else
	{
		addflags = get_template_flags(mc, flagstr);
		if (addflags == 0)
		{
			/* Hack -- jilles */
			if (*target == '+' || *target == '-' || *target == '=')
				command_fail(si, fault_badparams, _("Usage: FFLAGS %s <target> <flags>"), mc->name);
			else
				command_fail(si, fault_badparams, _("Invalid template name given, use /%s%s TEMPLATE %s for a list"), ircd->uses_rcommand ? "" : "msg ", chansvs.me->disp, mc->name);
			return;
		}
		removeflags = ca_all & ~addflags;
	}

	if (!validhostmask(target))
	{
		if (!(mt = myentity_find_ext(target)))
		{
			command_fail(si, fault_nosuch_target, _("\2%s\2 is not registered."), target);
			return;
		}
		target = mt->name;

		/* XXX this should be more like flags.c */
		if (removeflags & CA_FLAGS)
			removeflags |= CA_FOUNDER, addflags &= ~CA_FOUNDER;
		else if (addflags & CA_FOUNDER)
			addflags |= CA_FLAGS, removeflags &= ~CA_FLAGS;
		if (is_founder(mc, mt) && removeflags & CA_FOUNDER && mychan_num_founders(mc) == 1)
		{
			command_fail(si, fault_noprivs, _("You may not remove the last founder."));
			return;
		}

		if (!chanacs_change(mc, mt, NULL, &addflags, &removeflags, ca_all))
		{
			/* this shouldn't happen */
			command_fail(si, fault_noprivs, _("You are not allowed to set \2%s\2 on \2%s\2 in \2%s\2."), bitmask_to_flags2(addflags, removeflags), mt->name, mc->name);
			return;
		}
	}
	else
	{
		if (addflags & CA_FOUNDER)
		{
			command_fail(si, fault_badparams, _("You may not set founder status on a hostmask."));
			return;
		}
		if (!chanacs_change(mc, NULL, target, &addflags, &removeflags, ca_all))
		{
			/* this shouldn't happen */
			command_fail(si, fault_noprivs, _("You are not allowed to set \2%s\2 on \2%s\2 in \2%s\2."), bitmask_to_flags2(addflags, removeflags), target, mc->name);
			return;
		}
	}

	if ((addflags | removeflags) == 0)
	{
		command_fail(si, fault_nochange, _("Channel access to \2%s\2 for \2%s\2 unchanged."), channel, target);
		return;
	}
	flagstr = bitmask_to_flags2(addflags, removeflags);
	wallops("\2%s\2 is forcing flags change \2%s\2 on \2%s\2 in \2%s\2.", get_oper_name(si), flagstr, target, mc->name);
	command_success_nodata(si, _("Flags \2%s\2 were set on \2%s\2 in \2%s\2."), flagstr, target, channel);
	logcommand(si, CMDLOG_ADMIN, "FFLAGS: \2%s\2 \2%s\2 \2%s\2", mc->name, target, flagstr);
	verbose(mc, "\2%s\2 forced flags change \2%s\2 on \2%s\2.", get_source_name(si), flagstr, target);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
