/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Lists object properties via their metadata table.
 *
 * $Id: taxonomy.c 2557 2005-10-04 06:44:30Z pfish $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"nickserv/taxonomy", FALSE, _modinit, _moddeinit,
	"$Id: taxonomy.c 2557 2005-10-04 06:44:30Z pfish $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void ns_cmd_taxonomy(char *origin);

command_t ns_taxonomy = { "TAXONOMY", "Displays a user's metadata.", AC_NONE, ns_cmd_taxonomy };

list_t *ns_cmdtree, *ns_helptree;

void _modinit(module_t *m)
{
	ns_cmdtree = module_locate_symbol("nickserv/main", "ns_cmdtree");
	command_add(&ns_taxonomy, ns_cmdtree);
}

void _moddeinit()
{
	command_delete(&ns_taxonomy, ns_cmdtree);
}

static void ns_cmd_taxonomy(char *origin)
{
	char *target = strtok(NULL, " ");
	user_t *u = user_find(origin);
	myuser_t *mu;
	node_t *n;

	if (!target)
	{
		notice(nicksvs.nick, origin, "Insufficient parameters for TAXONOMY.");
		notice(nicksvs.nick, origin, "Syntax: TAXONOMY <nick>");
		return;
	}

	if (!(mu = myuser_find(target)))
	{
		notice(nicksvs.nick, origin, "\2%s\2 is not a registered nickname.", target);
		return;
	}

	snoop("TAXONOMY:\2%s\2", origin);

	notice(nicksvs.nick, origin, "Taxonomy for \2%s\2:", target);

	LIST_FOREACH(n, mu->metadata.head)
	{
		metadata_t *md = n->data;

		if (md->private == TRUE && !is_ircop(u) && !is_sra(u->myuser))
			continue;

		notice(nicksvs.nick, origin, "%-32s: %s", md->name, md->value);
	}

	notice(nicksvs.nick, origin, "End of \2%s\2 taxonomy.", target);
}
