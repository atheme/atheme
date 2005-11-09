/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Lists object properties via their metadata table.
 *
 * $Id: taxonomy.c 3741 2005-11-09 13:02:50Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/taxonomy", FALSE, _modinit, _moddeinit,
	"$Id: taxonomy.c 3741 2005-11-09 13:02:50Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_taxonomy(char *origin);

command_t cs_taxonomy = { "TAXONOMY", "Displays a channel's metadata.",
                        AC_NONE, cs_cmd_taxonomy };
                                                                                   
list_t *cs_cmdtree;
list_t *cs_helptree;

void _modinit(module_t *m)
{
        cs_cmdtree = module_locate_symbol("chanserv/main", "cs_cmdtree");
	cs_helptree = module_locate_symbol("chanserv/main", "cs_helptree");

	command_add(&cs_taxonomy, cs_cmdtree);
	help_addentry(cs_helptree, "TAXONOMY", "help/cservice/taxonomy", NULL);
}

void _moddeinit()
{
	command_delete(&cs_taxonomy, cs_cmdtree);
	help_delentry(cs_helptree, "TAXONOMY");
}

void cs_cmd_taxonomy(char *origin)
{
	char *target = strtok(NULL, " ");
	user_t *u = user_find(origin);
	myuser_t *mu;
	mychan_t *mc;
	node_t *n;

	if (!target || *target != '#')
	{
		notice(chansvs.nick, origin, "Insufficient parameters for TAXONOMY.");
		notice(chansvs.nick, origin, "Syntax: TAXONOMY <#channel>");
		return;
	}

	if (!(mc = mychan_find(target)))
	{
		notice(chansvs.nick, origin, "\2%s\2 is not a registered channel.", target);
		return;
	}

	/*snoop("TAXONOMY: \2%s\2", origin);*/
	logcommand(chansvs.me, u, CMDLOG_GET, "%s TAXONOMY", mc->name);
	notice(chansvs.nick, origin, "Taxonomy for \2%s\2:", target);

	LIST_FOREACH(n, mc->metadata.head)
	{
		metadata_t *md = n->data;

                if (md->private == TRUE && !is_ircop(u) && !is_sra(u->myuser))
                        continue;

		notice(chansvs.nick, origin, "%-32s: %s", md->name, md->value);
	}

	notice(chansvs.nick, origin, "End of \2%s\2 taxonomy.", target);
}
