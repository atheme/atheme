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
	"userserv/taxonomy", FALSE, _modinit, _moddeinit,
	"$Id: taxonomy.c 3741 2005-11-09 13:02:50Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void us_cmd_taxonomy(char *origin);

command_t us_taxonomy = { "TAXONOMY", "Displays a user's metadata.", AC_NONE, us_cmd_taxonomy };

list_t *us_cmdtree, *us_helptree;

void _modinit(module_t *m)
{
	us_cmdtree = module_locate_symbol("userserv/main", "us_cmdtree");
	us_helptree = module_locate_symbol("userserv/main", "us_helptree");
	command_add(&us_taxonomy, us_cmdtree);
	help_addentry(us_helptree, "TAXONOMY", "help/userserv/taxonomy", NULL);
}

void _moddeinit()
{
	command_delete(&us_taxonomy, us_cmdtree);
	help_delentry(us_helptree, "TAXONOMY");
}

static void us_cmd_taxonomy(char *origin)
{
	char *target = strtok(NULL, " ");
	user_t *u = user_find(origin);
	myuser_t *mu;
	node_t *n;

	if (!target)
	{
		notice(usersvs.nick, origin, "Insufficient parameters for TAXONOMY.");
		notice(usersvs.nick, origin, "Syntax: TAXONOMY <nick>");
		return;
	}

	if (!(mu = myuser_find(target)))
	{
		notice(usersvs.nick, origin, "\2%s\2 is not a registered account.", target);
		return;
	}

	/*snoop("TAXONOMY:\2%s\2 by \2%s\2", target, origin);*/
	logcommand(usersvs.me, u, CMDLOG_GET, "TAXONOMY %s", target);

	notice(usersvs.nick, origin, "Taxonomy for \2%s\2:", target);

	LIST_FOREACH(n, mu->metadata.head)
	{
		metadata_t *md = n->data;

		if (md->private == TRUE && !is_ircop(u) && !is_sra(u->myuser))
			continue;

		notice(usersvs.nick, origin, "%-32s: %s", md->name, md->value);
	}

	notice(usersvs.nick, origin, "End of \2%s\2 taxonomy.", target);
}
