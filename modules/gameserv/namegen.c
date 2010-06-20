/*
 * Copyright (c) 2005-2006 William Pitcock <nenolod@nenolod.net> et al
 * Rights to this code are documented in doc/LICENSE.
 *
 * Name generator.
 *
 */

#include "atheme.h"
#include "namegen_tab.h"

DECLARE_MODULE_V1
(
	"gameserv/namegen", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void command_namegen(sourceinfo_t *si, int parc, char *parv[]);

command_t cmd_namegen = { "NAMEGEN", N_("Generates some names to ponder."), AC_NONE, 0, command_namegen };

list_t *gs_cmdtree;
list_t *cs_cmdtree;

list_t *gs_helptree;
list_t *cs_helptree;

void _modinit(module_t * m)
{
	MODULE_USE_SYMBOL(gs_cmdtree, "gameserv/main", "gs_cmdtree");
	MODULE_USE_SYMBOL(cs_cmdtree, "chanserv/main", "cs_cmdtree");	/* fantasy commands */
	MODULE_USE_SYMBOL(gs_helptree, "gameserv/main", "gs_helptree");
	MODULE_USE_SYMBOL(cs_helptree, "chanserv/main", "cs_helptree");	/* fantasy commands */

	command_add(&cmd_namegen, gs_cmdtree);
	help_addentry(gs_helptree, "NAMEGEN", "help/gameserv/namegen", NULL);

	command_add(&cmd_namegen, cs_cmdtree);
	help_addentry(cs_helptree, "NAMEGEN", "help/gameserv/namegen", NULL);
}

void _moddeinit()
{
	command_delete(&cmd_namegen, gs_cmdtree);
	help_delentry(gs_helptree, "NAMEGEN");

	command_delete(&cmd_namegen, cs_cmdtree);
	help_delentry(cs_helptree, "NAMEGEN");
}

/*
 * Handle reporting for both fantasy commands and normal commands in GameServ
 * quickly and easily. Of course, sourceinfo has a vtable that can be manipulated,
 * but this is quicker and easier...                                  -- nenolod
 */
static void gs_command_report(sourceinfo_t *si, const char *fmt, ...)
{
	va_list args;
	char buf[BUFSIZE];

	va_start(args, fmt);
	vsnprintf(buf, BUFSIZE, fmt, args);
	va_end(args);

	if (si->c != NULL)
		msg(chansvs.nick, si->c->name, "%s", buf);
	else
		command_success_nodata(si, "%s", buf);
}

static void command_namegen(sourceinfo_t *si, int parc, char *parv[])
{
	unsigned int iter;
	unsigned int amt = 20;
	char buf[BUFSIZE];

	*buf = '\0';

	for (iter = 0; iter < amt; iter++)
	{
		char namebuf[BUFSIZE];
		unsigned int medial_iter;

		/* Here we generate the name. */
		strlcpy(namebuf, begin_sym[rand() % BEGIN_SYM_SZ], BUFSIZE);

		for (medial_iter = rand() % 3; medial_iter > 0; medial_iter--)
			strlcat(namebuf, medial_sym[rand() % MEDIAL_SYM_SZ], BUFSIZE);

		strlcat(namebuf, end_sym[rand() % END_SYM_SZ], BUFSIZE);

		if (iter == 0)
			strlcpy(buf, namebuf, BUFSIZE);
		else
			strlcat(buf, namebuf, BUFSIZE);

		strlcat(buf, iter + 1 < amt ? ", " : ".", BUFSIZE);
	}

	gs_command_report(si, "Some names to ponder: %s", buf);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
