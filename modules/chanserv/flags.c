/*
 * Copyright (c) 2005-2007 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService FLAGS functions.
 *
 * $Id: flags.c 7895 2007-03-06 02:40:03Z pippijn $
 */

#include "atheme.h"
#include "template.h"

DECLARE_MODULE_V1
(
	"chanserv/flags", FALSE, _modinit, _moddeinit,
	"$Id: flags.c 7895 2007-03-06 02:40:03Z pippijn $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_flags(sourceinfo_t *si, int parc, char *parv[]);

command_t cs_flags = { "FLAGS", N_("Manipulates specific permissions on a channel."),
                        AC_NONE, 3, cs_cmd_flags };

list_t *cs_cmdtree;
list_t *cs_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(cs_cmdtree, "chanserv/main", "cs_cmdtree");
	MODULE_USE_SYMBOL(cs_helptree, "chanserv/main", "cs_helptree");

	command_add(&cs_flags, cs_cmdtree);
	help_addentry(cs_helptree, "FLAGS", "help/cservice/flags", NULL);
}

void _moddeinit()
{
	command_delete(&cs_flags, cs_cmdtree);
	help_delentry(cs_helptree, "FLAGS");
}

static const char *get_template_name(mychan_t *mc, uint32_t level)
{
	metadata_t *md;
	const char *p, *q, *r;
	char *s;
	char ss[40];
	static char flagname[400];

	md = metadata_find(mc, METADATA_CHANNEL, "private:templates");
	if (md != NULL)
	{
		p = md->value;
		while (p != NULL)
		{
			while (*p == ' ')
				p++;
			q = strchr(p, '=');
			if (q == NULL)
				break;
			r = strchr(q, ' ');
			if (r != NULL && r < q)
				break;
			strlcpy(ss, q, sizeof ss);
			if (r != NULL && r - q < (int)(sizeof ss - 1))
			{
				ss[r - q] = '\0';
			}
			if (level == flags_to_bitmask(ss, chanacs_flags, 0))
			{
				strlcpy(flagname, p, sizeof flagname);
				s = strchr(flagname, '=');
				if (s != NULL)
					*s = '\0';
				return flagname;
			}
			p = r;
		}
	}
	if (level == chansvs.ca_sop)
		return "SOP";
	if (level == chansvs.ca_aop)
		return "AOP";
	/* if vop==hop, prefer vop */
	if (level == chansvs.ca_vop)
		return "VOP";
	if (level == chansvs.ca_hop)
		return "HOP";
	return NULL;
}

/* FLAGS <channel> [user] [flags] */
static void cs_cmd_flags(sourceinfo_t *si, int parc, char *parv[])
{
	chanacs_t *ca;
	node_t *n;
	int operoverride = 0;
	char *channel = parv[0];
	char *target = parv[1];
	const char *str1;
	uint32_t addflags, removeflags, restrictflags;

	if (parc < 1)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "FLAGS");
		command_fail(si, fault_needmoreparams, _("Syntax: FLAGS <channel> [target] [flags]"));
		return;
	}

	if (!target)
	{
		mychan_t *mc = mychan_find(channel);
		unsigned int i = 1;

		if (!mc)
		{
			command_fail(si, fault_nosuch_target, _("\2%s\2 is not registered."), channel);
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
		
		if (metadata_find(mc, METADATA_CHANNEL, "private:close:closer") && !has_priv(si, PRIV_CHAN_AUSPEX))
		{
			command_fail(si, fault_noprivs, _("\2%s\2 is closed."), channel);
			return;
		}

		command_success_nodata(si, _("Entry Nickname/Host          Flags"));
		command_success_nodata(si, "----- ---------------------- -----");

		LIST_FOREACH(n, mc->chanacs.head)
		{
			ca = n->data;
			str1 = get_template_name(mc, ca->level);
			if (str1 != NULL)
				command_success_nodata(si, "%-5d %-22s %s (%s)", i, ca->myuser ? ca->myuser->name : ca->host, bitmask_to_flags(ca->level, chanacs_flags), str1);
			else
				command_success_nodata(si, "%-5d %-22s %s", i, ca->myuser ? ca->myuser->name : ca->host, bitmask_to_flags(ca->level, chanacs_flags));
			i++;
		}

		command_success_nodata(si, "----- ---------------------- -----");
		command_success_nodata(si, _("End of \2%s\2 FLAGS listing."), channel);
		if (operoverride)
			logcommand(si, CMDLOG_ADMIN, "%s FLAGS (oper override)", mc->name);
		else
			logcommand(si, CMDLOG_GET, "%s FLAGS", mc->name);
	}
	else
	{
		mychan_t *mc = mychan_find(channel);
		myuser_t *tmu;
		char *flagstr = parv[2];

		if (!si->smu)
		{
			command_fail(si, fault_noprivs, _("You are not logged in."));
			return;
		}

		if (!mc)
		{
			command_fail(si, fault_nosuch_target, _("\2%s\2 is not registered."), channel);
			return;
		}
		
		if (metadata_find(mc, METADATA_CHANNEL, "private:close:closer"))
		{
			command_fail(si, fault_noprivs, _("\2%s\2 is closed."), channel);
			return;
		}

		if (!target)
		{
			command_fail(si, fault_needmoreparams, _("Usage: FLAGS %s [target] [flags]"), channel);
			return;
		}

		if (!flagstr)
		{
			if (!chanacs_source_has_flag(mc, si, CA_ACLVIEW))
			{
				command_fail(si, fault_noprivs, _("You are not authorized to execute this command."));
				return;
			}
			if (validhostmask(target))
				ca = chanacs_find_host_literal(mc, target, 0);
			else
			{
				if (!(tmu = myuser_find_ext(target)))
				{
					command_fail(si, fault_nosuch_target, _("The nickname \2%s\2 is not registered."), target);
					return;
				}
				target = tmu->name;
				ca = chanacs_find(mc, tmu, 0);
			}
			if (ca != NULL)
			{
				str1 = bitmask_to_flags2(ca->level, 0, chanacs_flags);
				command_success_string(si, str1, _("Flags for \2%s\2 in \2%s\2 are \2%s\2."),
						target, channel,
						str1);
			}
			else
				command_success_string(si, "", _("No flags for \2%s\2 in \2%s\2."),
						target, channel);
			logcommand(si, CMDLOG_GET, "%s FLAGS %s", mc->name, target);
			return;
		}

		/* founder may always set flags -- jilles */
		if (is_founder(mc, si->smu))
			restrictflags = ca_all;
		else
		{
			restrictflags = chanacs_source_flags(mc, si);
			if (!(restrictflags & CA_FLAGS))
			{
				/* allow a user to remove their own access
				 * even without +f */
				if (restrictflags & CA_AKICK ||
						si->smu == NULL ||
						irccasecmp(target, si->smu->name) ||
						strcmp(flagstr, "-*"))
				{
					command_fail(si, fault_noprivs, _("You are not authorized to execute this command."));
					return;
				}
			}
			restrictflags = allow_flags(restrictflags);
		}

		if (*flagstr == '+' || *flagstr == '-' || *flagstr == '=')
		{
			flags_make_bitmasks(flagstr, chanacs_flags, &addflags, &removeflags);
			if (addflags == 0 && removeflags == 0)
			{
				command_fail(si, fault_badparams, _("No valid flags given, use /%s%s HELP FLAGS for a list"), ircd->uses_rcommand ? "" : "msg ", chansvs.disp);
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
					command_fail(si, fault_badparams, _("Usage: FLAGS %s [target] [flags]"), mc->name);
				else
					command_fail(si, fault_badparams, _("Invalid template name given, use /%s%s TEMPLATE %s for a list"), ircd->uses_rcommand ? "" : "msg ", chansvs.disp, mc->name);
				return;
			}
			removeflags = ca_all & ~addflags;
		}

		if (!validhostmask(target))
		{
			if (!(tmu = myuser_find_ext(target)))
			{
				command_fail(si, fault_nosuch_target, _("The nickname \2%s\2 is not registered."), target);
				return;
			}
			target = tmu->name;

			if (tmu == mc->founder && removeflags & CA_FLAGS)
			{
				command_fail(si, fault_noprivs, _("You may not remove the founder's +f access."));
				return;
			}

			/* If NEVEROP is set, don't allow adding new entries
			 * except sole +b. Adding flags if the current level
			 * is +b counts as adding an entry.
			 * -- jilles */
			if (MU_NEVEROP & tmu->flags && addflags != CA_AKICK && addflags != 0 && ((ca = chanacs_find(mc, tmu, 0)) == NULL || ca->level == CA_AKICK))
			{
				command_fail(si, fault_noprivs, _("\2%s\2 does not wish to be added to access lists (NEVEROP set)."), tmu->name);
				return;
			}

			if (!chanacs_change(mc, tmu, NULL, &addflags, &removeflags, restrictflags))
			{
				command_fail(si, fault_noprivs, _("You are not allowed to set \2%s\2 on \2%s\2 in \2%s\2."), bitmask_to_flags2(addflags, removeflags, chanacs_flags), tmu->name, mc->name);
				return;
			}
		}
		else
		{
			if (!chanacs_change(mc, NULL, target, &addflags, &removeflags, restrictflags))
			{
		                command_fail(si, fault_noprivs, _("You are not allowed to set \2%s\2 on \2%s\2 in \2%s\2."), bitmask_to_flags2(addflags, removeflags, chanacs_flags), target, mc->name);
				return;
			}
		}

		if ((addflags | removeflags) == 0)
		{
			command_fail(si, fault_nochange, _("Channel access to \2%s\2 for \2%s\2 unchanged."), channel, target);
			return;
		}
		flagstr = bitmask_to_flags2(addflags, removeflags, chanacs_flags);
		command_success_nodata(si, _("Flags \2%s\2 were set on \2%s\2 in \2%s\2."), flagstr, target, channel);
		logcommand(si, CMDLOG_SET, "%s FLAGS %s %s", mc->name, target, flagstr);
		verbose(mc, "\2%s\2 set flags \2%s\2 on \2%s\2.", get_source_name(si), flagstr, target);
	}
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
