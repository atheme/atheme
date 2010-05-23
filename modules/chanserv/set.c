/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains routines to handle the CService SET command.
 *
 * $Id: set.c 8027 2007-04-02 10:47:18Z nenolod $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/set", false, _modinit, _moddeinit,
	"$Id: set.c 8027 2007-04-02 10:47:18Z nenolod $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_set_config_ready(void *unused);

static void cs_help_set(sourceinfo_t *si);
static void cs_cmd_set(sourceinfo_t *si, int parc, char *parv[]);

static void cs_cmd_set_verbose(sourceinfo_t *si, int parc, char *parv[]);
static void cs_cmd_set_fantasy(sourceinfo_t *si, int parc, char *parv[]);
static void cs_cmd_set_restricted(sourceinfo_t *si, int parc, char *parv[]);
static void cs_cmd_set_property(sourceinfo_t *si, int parc, char *parv[]);
static void cs_cmd_set_guard(sourceinfo_t *si, int parc, char *parv[]);

command_t cs_set = { "SET", N_("Sets various control flags."),
                        AC_NONE, 3, cs_cmd_set };

command_t cs_set_verbose   = { "VERBOSE",   N_("Notifies channel about access list modifications."),            AC_NONE, 2, cs_cmd_set_verbose    };
command_t cs_set_property  = { "PROPERTY",  N_("Manipulates channel metadata."),                                AC_NONE, 2, cs_cmd_set_property   };
command_t cs_set_guard     = { "GUARD",     N_("Sets whether or not services will inhabit the channel."),       AC_NONE, 2, cs_cmd_set_guard      };
command_t cs_set_fantasy   = { "FANTASY",   N_("Allows or disallows in-channel commands."),                     AC_NONE, 2, cs_cmd_set_fantasy    };
command_t cs_set_restricted = { "RESTRICTED", N_("Restricts access to the channel to users on the access list. (Other users are kickbanned.)"),   AC_NONE, 2, cs_cmd_set_restricted  };

command_t *cs_set_commands[] = {
	&cs_set_verbose,
	&cs_set_property,
	&cs_set_guard,
	&cs_set_fantasy,
	&cs_set_restricted,
	NULL
};

list_t *cs_cmdtree;
list_t *cs_helptree;
list_t cs_set_cmdtree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(cs_cmdtree, "chanserv/main", "cs_cmdtree");
	MODULE_USE_SYMBOL(cs_helptree, "chanserv/main", "cs_helptree");

        command_add(&cs_set, cs_cmdtree);
	command_add_many(cs_set_commands, &cs_set_cmdtree);

	help_addentry(cs_helptree, "SET", NULL, cs_help_set);
	help_addentry(cs_helptree, "SET VERBOSE", "help/cservice/set_verbose", NULL);
	help_addentry(cs_helptree, "SET PROPERTY", "help/cservice/set_property", NULL);
	help_addentry(cs_helptree, "SET RESTRICTED", "help/cservice/set_restricted", NULL);
	help_addentry(cs_helptree, "SET FANTASY", "help/cservice/set_fantasy", NULL);
	help_addentry(cs_helptree, "SET GUARD", "help/cservice/set_guard", NULL);

	hook_add_event("config_ready");
	hook_add_config_ready(cs_set_config_ready);
}

void _moddeinit()
{
	command_delete(&cs_set, cs_cmdtree);
	command_delete_many(cs_set_commands, &cs_set_cmdtree);

	help_delentry(cs_helptree, "SET");
	help_delentry(cs_helptree, "SET VERBOSE");
	help_delentry(cs_helptree, "SET PROPERTY");
	help_delentry(cs_helptree, "SET RESTRICTED");
	help_delentry(cs_helptree, "SET FANTASY");
	help_delentry(cs_helptree, "SET GUARD");

	hook_del_config_ready(cs_set_config_ready);
}

static void cs_set_config_ready(void *unused)
{
	if (config_options.join_chans)
		cs_set_guard.access = NULL;
	else
		cs_set_guard.access = PRIV_ADMIN;
	if (chansvs.fantasy)
		cs_set_fantasy.access = NULL;
	else
		cs_set_fantasy.access = AC_DISABLED;
}

static void cs_help_set(sourceinfo_t *si)
{
	command_success_nodata(si, _("Help for \2SET\2:"));
	command_success_nodata(si, " ");
	command_success_nodata(si, _("SET allows you to set various control flags\n"
				"for channels that change the way certain\n"
				"operations are performed on them."));
	command_success_nodata(si, " ");
	command_help(si, &cs_set_cmdtree);
	command_success_nodata(si, " ");
	command_success_nodata(si, _("For more specific help use \2/msg %s HELP SET \37command\37\2."), si->service->disp);
}

/* SET <#channel> <setting> <parameters> */
static void cs_cmd_set(sourceinfo_t *si, int parc, char *parv[])
{
	char *chan;
	char *cmd;
	command_t *c;

	if (parc < 3)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SET");
		command_fail(si, fault_needmoreparams, _("Syntax: SET <#channel> <setting> <parameters>"));
		return;
	}

	if (parv[0][0] == '#')
		chan = parv[0], cmd = parv[1];
	else if (parv[1][0] == '#')
		cmd = parv[0], chan = parv[1];
	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "SET");
		command_fail(si, fault_badparams, _("Syntax: SET <#channel> <setting> <parameters>"));
		return;
	}

	c = command_find(&cs_set_cmdtree, cmd);
	if (c == NULL)
	{
		command_fail(si, fault_badparams, _("Invalid command. Use \2/%s%s help\2 for a command listing."), (ircd->uses_rcommand == false) ? "msg " : "", si->service->disp);
		return;
	}

	parv[1] = chan;
	command_exec(si->service, si, c, parc - 1, parv + 1);
}

static void cs_cmd_set_verbose(sourceinfo_t *si, int parc, char *parv[])
{
	mychan_t *mc;

	if (!(mc = mychan_find(parv[0])))
	{
		command_fail(si, fault_nosuch_target, _("Channel \2%s\2 is not registered."), parv[0]);
		return;
	}

	if (!chanacs_source_has_flag(mc, si, CA_SET))
	{
		command_fail(si, fault_noprivs, _("You are not authorized to perform this command."));
		return;
	}

	if (!strcasecmp("ON", parv[1]) || !strcasecmp("ALL", parv[1]))
	{
		if (MC_VERBOSE & mc->flags)
		{
			command_fail(si, fault_nochange, _("The \2%s\2 flag is already set for channel \2%s\2."), "VERBOSE", mc->name);
			return;
		}

		logcommand(si, CMDLOG_SET, "SET:VERBOSE:ON: \2%s\2", mc->name);

 		mc->flags &= ~MC_VERBOSE_OPS;
 		mc->flags |= MC_VERBOSE;

		verbose(mc, "\2%s\2 enabled the VERBOSE flag", get_source_name(si));
		command_success_nodata(si, _("The \2%s\2 flag has been set for channel \2%s\2."), "VERBOSE", mc->name);
		return;
	}

	else if (!strcasecmp("OPS", parv[1]))
	{
		if (MC_VERBOSE_OPS & mc->flags)
		{
			command_fail(si, fault_nochange, _("The \2%s\2 flag is already set for channel \2%s\2."), "VERBOSE_OPS", mc->name);
			return;
		}

		logcommand(si, CMDLOG_SET, "SET:VERBOSE:OPS: \2%s\2", mc->name);

		if (mc->flags & MC_VERBOSE)
		{
			verbose(mc, "\2%s\2 restricted VERBOSE to chanops", get_source_name(si));
 			mc->flags &= ~MC_VERBOSE;
 			mc->flags |= MC_VERBOSE_OPS;
		}
		else
		{
 			mc->flags |= MC_VERBOSE_OPS;
			verbose(mc, "\2%s\2 enabled the VERBOSE_OPS flag", get_source_name(si));
		}

		command_success_nodata(si, _("The \2%s\2 flag has been set for channel \2%s\2."), "VERBOSE_OPS", mc->name);
		return;
	}
	else if (!strcasecmp("OFF", parv[1]))
	{
		if (!((MC_VERBOSE | MC_VERBOSE_OPS) & mc->flags))
		{
			command_fail(si, fault_nochange, _("The \2%s\2 flag is not set for channel \2%s\2."), "VERBOSE", mc->name);
			return;
		}

		logcommand(si, CMDLOG_SET, "SET:VERBOSE:OFF: \2%s\2", mc->name);

		if (mc->flags & MC_VERBOSE)
			verbose(mc, "\2%s\2 disabled the VERBOSE flag", get_source_name(si));
		else
			verbose(mc, "\2%s\2 disabled the VERBOSE_OPS flag", get_source_name(si));
		mc->flags &= ~(MC_VERBOSE | MC_VERBOSE_OPS);

		command_success_nodata(si, _("The \2%s\2 flag has been removed for channel \2%s\2."), "VERBOSE", mc->name);
		return;
	}
	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "VERBOSE");
		return;
	}
}

static void cs_cmd_set_fantasy(sourceinfo_t *si, int parc, char *parv[])
{
	mychan_t *mc;

	if (!(mc = mychan_find(parv[0])))
	{
		command_fail(si, fault_nosuch_target, _("Channel \2%s\2 is not registered."), parv[0]);
		return;
	}

	if (!chanacs_source_has_flag(mc, si, CA_SET))
	{
		command_fail(si, fault_noprivs, _("You are not authorized to perform this command."));
		return;
	}

	if (!strcasecmp("ON", parv[1]))
	{
		metadata_t *md = metadata_find(mc, "disable_fantasy");

		if (!md)
		{
			command_fail(si, fault_nochange, _("The \2%s\2 flag is already set for channel \2%s\2."), "FANTASY", mc->name);
			return;
		}

		metadata_delete(mc, "disable_fantasy");

		logcommand(si, CMDLOG_SET, "SET:FANTASY:ON: \2%s\2", mc->name);
		command_success_nodata(si, _("The \2%s\2 flag has been set for channel \2%s\2."), "FANTASY", mc->name);
		return;
	}
	else if (!strcasecmp("OFF", parv[1]))
	{
		metadata_t *md = metadata_find(mc, "disable_fantasy");

		if (md)
		{
			command_fail(si, fault_nochange, _("The \2%s\2 flag is not set for channel \2%s\2."), "FANTASY", mc->name);
			return;
		}

		metadata_add(mc, "disable_fantasy", "on");

		logcommand(si, CMDLOG_SET, "SET:FANTASY:OFF: \2%s\2", mc->name);
		command_success_nodata(si, _("The \2%s\2 flag has been removed for channel \2%s\2."), "FANTASY", mc->name);
		return;
	}
	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "FANTASY");
		return;
	}
}

static void cs_cmd_set_guard(sourceinfo_t *si, int parc, char *parv[])
{
	mychan_t *mc;

	if (!(mc = mychan_find(parv[0])))
	{
		command_fail(si, fault_nosuch_target, _("Channel \2%s\2 is not registered."), parv[0]);
		return;
	}

	if (!chanacs_source_has_flag(mc, si, CA_SET))
	{
		command_fail(si, fault_noprivs, _("You are not authorized to perform this command."));
		return;
	}

	if (!strcasecmp("ON", parv[1]))
	{
		if (MC_GUARD & mc->flags)
		{
			command_fail(si, fault_nochange, _("The \2%s\2 flag is already set for channel \2%s\2."), "GUARD", mc->name);
			return;
		}
		if (metadata_find(mc, "private:botserv:bot-assigned") &&
				module_find_published("botserv/main"))
		{
			command_fail(si, fault_noprivs, _("Channel \2%s\2 already has a BotServ bot assigned to it.  You need to unassign it first."), mc->name);
			return;
		}

		logcommand(si, CMDLOG_SET, "SET:GUARD:ON: \2%s\2", mc->name);

		mc->flags |= MC_GUARD;

		if (!(mc->flags & MC_INHABIT))
			join(mc->name, chansvs.nick);

		command_success_nodata(si, _("The \2%s\2 flag has been set for channel \2%s\2."), "GUARD", mc->name);
		return;
	}
	else if (!strcasecmp("OFF", parv[1]))
	{
		if (!(MC_GUARD & mc->flags))
		{
			command_fail(si, fault_nochange, _("The \2%s\2 flag is not set for channel \2%s\2."), "GUARD", mc->name);
			return;
		}

		logcommand(si, CMDLOG_SET, "SET:GUARD:OFF: \2%s\2", mc->name);

		mc->flags &= ~MC_GUARD;

		if (mc->chan != NULL && !(mc->flags & MC_INHABIT) && (!config_options.chan || irccasecmp(config_options.chan, mc->name)) && !(mc->chan->flags & CHAN_LOG))
			part(mc->name, chansvs.nick);

		command_success_nodata(si, _("The \2%s\2 flag has been removed for channel \2%s\2."), "GUARD", mc->name);
		return;
	}
	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "GUARD");
		return;
	}
}

static void cs_cmd_set_restricted(sourceinfo_t *si, int parc, char *parv[])
{
	mychan_t *mc;

	if (!(mc = mychan_find(parv[0])))
	{
		command_fail(si, fault_nosuch_target, _("Channel \2%s\2 is not registered."), parv[0]);
		return;
	}

	if (!chanacs_source_has_flag(mc, si, CA_SET))
	{
		command_fail(si, fault_noprivs, _("You are not authorized to perform this command."));
		return;
	}

	if (!strcasecmp("ON", parv[1]))
	{
		if (MC_RESTRICTED & mc->flags)
		{
			command_fail(si, fault_nochange, _("The \2%s\2 flag is already set for channel \2%s\2."), "RESTRICTED", mc->name);
			return;
		}

		logcommand(si, CMDLOG_SET, "SET:RESTRICTED:ON: \2%s\2", mc->name);

		mc->flags |= MC_RESTRICTED;

		command_success_nodata(si, _("The \2%s\2 flag has been set for channel \2%s\2."), "RESTRICTED", mc->name);
		return;
	}
	else if (!strcasecmp("OFF", parv[1]))
	{
		if (!(MC_RESTRICTED & mc->flags))
		{
			command_fail(si, fault_nochange, _("The \2%s\2 flag is not set for channel \2%s\2."), "RESTRICTED", mc->name);
			return;
		}

		logcommand(si, CMDLOG_SET, "SET:RESTRICTED:OFF: \2%s\2", mc->name);

		mc->flags &= ~MC_RESTRICTED;

		command_success_nodata(si, _("The \2%s\2 flag has been removed for channel \2%s\2."), "RESTRICTED", mc->name);
		return;
	}
	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "RESTRICTED");
		return;
	}
}

static void cs_cmd_set_property(sourceinfo_t *si, int parc, char *parv[])
{
	mychan_t *mc;
	char *property = strtok(parv[1], " ");
	char *value = strtok(NULL, "");
	unsigned int count;
	node_t *n;
	metadata_t *md;

	if (!property)
	{
		command_fail(si, fault_needmoreparams, _("Syntax: SET <#channel> PROPERTY <property> [value]"));
		return;
	}

	/* do we really need to allow this? -- jilles */
	if (strchr(property, ':') && !has_priv(si, PRIV_METADATA))
	{
		command_fail(si, fault_badparams, _("Invalid property name."));
		return;
	}

	if (!(mc = mychan_find(parv[0])))
	{
		command_fail(si, fault_nosuch_target, _("Channel \2%s\2 is not registered."), parv[0]);
		return;
	}

	if (!is_founder(mc, si->smu))
	{
		command_fail(si, fault_noprivs, _("You are not authorized to perform this command."));
		return;
	}

	if (strchr(property, ':'))
		logcommand(si, CMDLOG_SET, "SET:PROPERTY: \2%s\2: \2%s\2/\2%s\2", mc->name, property, value);

	if (!value)
	{
		md = metadata_find(mc, property);

		if (!md)
		{
			command_fail(si, fault_nochange, _("Metadata entry \2%s\2 was not set."), property);
			return;
		}

		metadata_delete(mc, property);
		logcommand(si, CMDLOG_SET, "SET:PROPERTY: \2%s\2 \2%s\2 (deleted)", mc->name, property);
		command_success_nodata(si, _("Metadata entry \2%s\2 has been deleted."), property);
		return;
	}

	count = 0;
	LIST_FOREACH(n, object(mc)->metadata.head)
	{
		md = n->data;
		if (strchr(property, ':') ? md->private : !md->private)
			count++;
	}
	if (count >= me.mdlimit)
	{
		command_fail(si, fault_toomany, _("Cannot add \2%s\2 to \2%s\2 metadata table, it is full."),
						property, parv[0]);
		return;
	}

	if (strlen(property) > 32 || strlen(value) > 300 || has_ctrl_chars(property))
	{
		command_fail(si, fault_badparams, _("Parameters are too long. Aborting."));
		return;
	}

	metadata_add(mc, property, value);
	logcommand(si, CMDLOG_SET, "SET:PROPERTY: \2%s\2 on \2%s\2 to \2%s\2", property, mc->name, value);
	command_success_nodata(si, _("Metadata entry \2%s\2 added."), property);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
