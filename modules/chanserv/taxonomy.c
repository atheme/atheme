/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Lists object properties via their metadata table.
 *
 * $Id: taxonomy.c 6337 2006-09-10 15:54:41Z pippijn $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/taxonomy", FALSE, _modinit, _moddeinit,
	"$Id: taxonomy.c 6337 2006-09-10 15:54:41Z pippijn $",
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
		notice(chansvs.nick, si->su->nick, STR_INSUFFICIENT_PARAMS, "TAXONOMY");
		notice(chansvs.nick, si->su->nick, "Syntax: TAXONOMY <#channel>");
		return;
	}

	if (!(mc = mychan_find(target)))
	{
		notice(chansvs.nick, si->su->nick, "\2%s\2 is not a registered channel.", target);
		return;
	}

	isoper = has_priv(si->su, PRIV_CHAN_AUSPEX);
	if (isoper)
		logcommand(chansvs.me, si->su, CMDLOG_ADMIN, "%s TAXONOMY (oper)", mc->name);
	else
		logcommand(chansvs.me, si->su, CMDLOG_GET, "%s TAXONOMY", mc->name);
	notice(chansvs.nick, si->su->nick, "Taxonomy for \2%s\2:", target);

	LIST_FOREACH(n, mc->metadata.head)
	{
		metadata_t *md = n->data;

                if (md->private && !isoper)
                        continue;

		notice(chansvs.nick, si->su->nick, "%-32s: %s", md->name, md->value);
	}

	notice(chansvs.nick, si->su->nick, "End of \2%s\2 taxonomy.", target);
}
