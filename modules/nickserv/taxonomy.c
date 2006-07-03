/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Lists object properties via their metadata table.
 *
 * $Id: taxonomy.c 5686 2006-07-03 16:25:03Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"nickserv/taxonomy", FALSE, _modinit, _moddeinit,
	"$Id: taxonomy.c 5686 2006-07-03 16:25:03Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void ns_cmd_taxonomy(char *origin);

command_t ns_taxonomy = { "TAXONOMY", "Displays a user's metadata.", AC_NONE, ns_cmd_taxonomy };

list_t *ns_cmdtree, *ns_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(ns_cmdtree, "nickserv/main", "ns_cmdtree");
	MODULE_USE_SYMBOL(ns_helptree, "nickserv/main", "ns_helptree");

	command_add(&ns_taxonomy, ns_cmdtree);
	help_addentry(ns_helptree, "TAXONOMY", "help/nickserv/taxonomy", NULL);
}

void _moddeinit()
{
	command_delete(&ns_taxonomy, ns_cmdtree);
	help_delentry(ns_helptree, "TAXONOMY");
}

static void ns_cmd_taxonomy(char *origin)
{
	char *target = strtok(NULL, " ");
	user_t *u = user_find_named(origin);
	myuser_t *mu;
	node_t *n;
	boolean_t isoper;

	if (!target)
	{
		notice(nicksvs.nick, origin, STR_INSUFFICIENT_PARAMS, "TAXONOMY");
		notice(nicksvs.nick, origin, "Syntax: TAXONOMY <nick>");
		return;
	}

	if (!(mu = myuser_find_ext(target)))
	{
		notice(nicksvs.nick, origin, "\2%s\2 is not a registered nickname.", target);
		return;
	}

	isoper = has_priv(u, PRIV_USER_AUSPEX);
	if (isoper)
		logcommand(nicksvs.me, u, CMDLOG_ADMIN, "TAXONOMY %s (oper)", target);
	else
		logcommand(nicksvs.me, u, CMDLOG_GET, "TAXONOMY %s", target);

	notice(nicksvs.nick, origin, "Taxonomy for \2%s\2:", target);

	LIST_FOREACH(n, mu->metadata.head)
	{
		metadata_t *md = n->data;

		if (md->private && !isoper)
			continue;

		notice(nicksvs.nick, origin, "%-32s: %s", md->name, md->value);
	}

	notice(nicksvs.nick, origin, "End of \2%s\2 taxonomy.", target);
}
