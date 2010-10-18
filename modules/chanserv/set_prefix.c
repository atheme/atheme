/*
 * Copyright (c) 2006-2010 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains routines to handle the CService SET PREFIX command.
 *
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/set_prefix", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_set_prefix_config_ready(void *unused); 
static void cs_cmd_set_prefix(sourceinfo_t *si, int parc, char *parv[]);

command_t cs_set_prefix = { "PREFIX", N_("Sets the channel PREFIX."), AC_NONE, 2, cs_cmd_set_prefix, { .path = "cservice/set_prefix" } };

mowgli_patricia_t **cs_set_cmdtree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(cs_set_cmdtree, "chanserv/set_core", "cs_set_cmdtree");

	command_add(&cs_set_prefix, *cs_set_cmdtree);

	hook_add_event("config_ready");
	hook_add_config_ready(cs_set_prefix_config_ready); 
}

void _moddeinit()
{
	command_delete(&cs_set_prefix, *cs_set_cmdtree);

	hook_del_config_ready(cs_set_prefix_config_ready);
}

static void cs_set_prefix_config_ready(void *unused)
{
	if (chansvs.fantasy)
		cs_set_prefix.access = NULL;
	else
		cs_set_prefix.access = AC_DISABLED;
}

static int goodprefix(const char *p)
{
	int i;
	int haschar = 0;
	int hasnonprint = 0;

	for (i = 0; p[i]; i++) {
		if (!isspace(p[i])) { haschar = 1; }
		if (!isprint(p[i])) { hasnonprint = 1; }
	}

	return haschar && !hasnonprint;
}


static void cs_cmd_set_prefix(sourceinfo_t *si, int parc, char *parv[])
{
	mychan_t *mc;
	char *prefix = parv[1];

	if (!(mc = mychan_find(parv[0])))
	{
		command_fail(si, fault_nosuch_target, _("Channel \2%s\2 is not registered."), parv[0]);
		return;
	}

	if (!chanacs_source_has_flag(mc, si, CA_SET))
	{
		command_fail(si, fault_noprivs, _("You are not authorized to execute this command."));
		return;
	}
	
	if (!prefix || !strcasecmp(prefix, "DEFAULT")) 
	{
		metadata_delete(mc, "private:prefix");
		logcommand(si, CMDLOG_SET, "SET:PREFIX: \2%s\2 reset", mc->name);
		command_success_nodata(si, _("The fantasy prefix for channel \2%s\2 has been reset."), parv[0]);
		return;
	}

	if (!goodprefix(prefix))
	{
		command_fail(si, fault_badparams, _("Prefix '%s' is invalid. The prefix may "
		             "contain only printable characters, and must contain at least "
		             "one non-space character."), prefix);
		return;
	}

	metadata_add(mc, "private:prefix", prefix);
	logcommand(si, CMDLOG_SET, "SET:PREFIX: \2%s\2 \2%s\2", mc->name, prefix);
	command_success_nodata(si, _("The fantasy prefix for channel \2%s\2 has been set to \2%s\2."),
                               parv[0], prefix);

}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
