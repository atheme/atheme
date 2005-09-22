/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Lists object properties via their metadata table.
 *
 * $Id: taxonomy.c 2297 2005-09-22 22:30:59Z pfish $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/taxonomy", FALSE, _modinit, _moddeinit,
	"$Id: taxonomy.c 2297 2005-09-22 22:30:59Z pfish $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_taxonomy(char *origin);

command_t cs_taxonomy = { "TAXONOMY", "Displays a channel's metadata.",
                        AC_NONE, cs_cmd_taxonomy };
                                                                                   
list_t *cs_cmdtree;

void _modinit(module_t *m)
{
        cs_cmdtree = module_locate_symbol("chanserv/main", "cs_cmdtree");
	command_add(&cs_taxonomy, cs_cmdtree);
}

void _moddeinit()
{
	command_delete(&cs_taxonomy, cs_cmdtree);
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

	snoop("TAXONOMY: \2%s\2", origin);
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
