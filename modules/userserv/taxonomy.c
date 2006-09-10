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
	"userserv/taxonomy", FALSE, _modinit, _moddeinit,
	"$Id: taxonomy.c 6337 2006-09-10 15:54:41Z pippijn $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void us_cmd_taxonomy(sourceinfo_t *si, int parc, char *parv[]);

command_t us_taxonomy = { "TAXONOMY", "Displays a user's metadata.", AC_NONE, 1, us_cmd_taxonomy };

list_t *us_cmdtree, *us_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(us_cmdtree, "userserv/main", "us_cmdtree");
	MODULE_USE_SYMBOL(us_helptree, "userserv/main", "us_helptree");

	command_add(&us_taxonomy, us_cmdtree);
	help_addentry(us_helptree, "TAXONOMY", "help/userserv/taxonomy", NULL);
}

void _moddeinit()
{
	command_delete(&us_taxonomy, us_cmdtree);
	help_delentry(us_helptree, "TAXONOMY");
}

static void us_cmd_taxonomy(sourceinfo_t *si, int parc, char *parv[])
{
	char *target = parv[0];
	myuser_t *mu;
	node_t *n;
	boolean_t isoper;

	if (!target)
	{
		notice(usersvs.nick, si->su->nick, STR_INSUFFICIENT_PARAMS, "TAXONOMY");
		notice(usersvs.nick, si->su->nick, "Syntax: TAXONOMY <nick>");
		return;
	}

	if (!(mu = myuser_find_ext(target)))
	{
		notice(usersvs.nick, si->su->nick, "\2%s\2 is not a registered account.", target);
		return;
	}

	isoper = has_priv(si->su, PRIV_USER_AUSPEX);
	if (isoper)
		logcommand(usersvs.me, si->su, CMDLOG_ADMIN, "TAXONOMY %s (oper)", target);
	else
		logcommand(usersvs.me, si->su, CMDLOG_GET, "TAXONOMY %s", target);

	notice(usersvs.nick, si->su->nick, "Taxonomy for \2%s\2:", target);

	LIST_FOREACH(n, mu->metadata.head)
	{
		metadata_t *md = n->data;

		if (md->private == TRUE && !isoper)
			continue;

		notice(usersvs.nick, si->su->nick, "%-32s: %s", md->name, md->value);
	}

	notice(usersvs.nick, si->su->nick, "End of \2%s\2 taxonomy.", target);
}
