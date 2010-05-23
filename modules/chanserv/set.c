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

static void cs_help_set(sourceinfo_t *si);
static void cs_cmd_set(sourceinfo_t *si, int parc, char *parv[]);

static void cs_cmd_set_property(sourceinfo_t *si, int parc, char *parv[]);

command_t cs_set = { "SET", N_("Sets various control flags."),
                        AC_NONE, 3, cs_cmd_set };

command_t cs_set_property  = { "PROPERTY",  N_("Manipulates channel metadata."),                                AC_NONE, 2, cs_cmd_set_property   };

command_t *cs_set_commands[] = {
	&cs_set_property,
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
	help_addentry(cs_helptree, "SET PROPERTY", "help/cservice/set_property", NULL);
}

void _moddeinit()
{
	command_delete(&cs_set, cs_cmdtree);
	command_delete_many(cs_set_commands, &cs_set_cmdtree);

	help_delentry(cs_helptree, "SET");
	help_delentry(cs_helptree, "SET PROPERTY");
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
