/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Lists object properties via their metadata table.
 *
 * $Id: taxonomy.c 7771 2007-03-03 12:46:36Z pippijn $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/taxonomy", FALSE, _modinit, _moddeinit,
	"$Id: taxonomy.c 7771 2007-03-03 12:46:36Z pippijn $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_taxonomy(sourceinfo_t *si, int parc, char *parv[]);

command_t cs_taxonomy = { "TAXONOMY", "Displays a channel's metadata.",
                        AC_NONE, 1, cs_cmd_taxonomy };
                                                                                   
list_t *cs_cmdtree;
list_t *cs_helptree;

void _modinit(module_t *m)
{
        MODULE_USE_SYMBOL(cs_cmdtree, "chanserv/main", "cs_cmdtree");
	MODULE_USE_SYMBOL(cs_helptree, "chanserv/main", "cs_helptree");

	command_add(&cs_taxonomy, cs_cmdtree);
	help_addentry(cs_helptree, "TAXONOMY", "help/cservice/taxonomy", NULL);
}

void _moddeinit()
{
	command_delete(&cs_taxonomy, cs_cmdtree);
	help_delentry(cs_helptree, "TAXONOMY");
}

void cs_cmd_taxonomy(sourceinfo_t *si, int parc, char *parv[])
{
	char *target = parv[0];
	mychan_t *mc;
	node_t *n;
	boolean_t isoper;

	if (!target || *target != '#')
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "TAXONOMY");
		command_fail(si, fault_needmoreparams, "Syntax: TAXONOMY <#channel>");
		return;
	}

	if (!(mc = mychan_find(target)))
	{
		command_fail(si, fault_nosuch_target, "\2%s\2 is not a registered channel.", target);
		return;
	}

	isoper = has_priv(si, PRIV_CHAN_AUSPEX);
	if (isoper)
		logcommand(si, CMDLOG_ADMIN, "%s TAXONOMY (oper)", mc->name);
	else
		logcommand(si, CMDLOG_GET, "%s TAXONOMY", mc->name);
	command_success_nodata(si, "Taxonomy for \2%s\2:", target);

	LIST_FOREACH(n, mc->metadata.head)
	{
		metadata_t *md = n->data;

                if (md->private && !isoper)
                        continue;

		command_success_nodata(si, "%-32s: %s", md->name, md->value);
	}

	command_success_nodata(si, "End of \2%s\2 taxonomy.", target);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:noexpandtab
 */
