/*
 * Copyright (c) 2005-2006 William Pitcock <nenolod@nenolod.net> et al
 * Rights to this code are documented in doc/LICENSE.
 *
 * Name generator.
 *
 */

#include "atheme.h"
#include "gameserv_common.h"
#include "namegen_tab.h"

DECLARE_MODULE_V1
(
	"gameserv/namegen", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void command_namegen(sourceinfo_t *si, int parc, char *parv[]);

command_t cmd_namegen = { "NAMEGEN", N_("Generates some names to ponder."), AC_NONE, 2, command_namegen, { .path = "gameserv/namegen" } };

void _modinit(module_t * m)
{
	service_named_bind_command("gameserv", &cmd_namegen);

	service_named_bind_command("chanserv", &cmd_namegen);
}

void _moddeinit(module_unload_intent_t intent)
{
	service_named_unbind_command("gameserv", &cmd_namegen);

	service_named_unbind_command("chanserv", &cmd_namegen);
}

static void command_namegen(sourceinfo_t *si, int parc, char *parv[])
{
	unsigned int iter;
	unsigned int amt = 20;
	char buf[BUFSIZE];
	mychan_t *mc;

	if (!gs_do_parameters(si, &parc, &parv, &mc))
		return;

	if (parv[0])
		amt = atoi(parv[0]);


	/* limit to 20 */
	if (amt > 20)
		amt = 20;

	*buf = '\0';

	for (iter = 0; iter < amt; iter++)
	{
		char namebuf[BUFSIZE];
		unsigned int medial_iter;

		/* Here we generate the name. */
		mowgli_strlcpy(namebuf, begin_sym[rand() % BEGIN_SYM_SZ], BUFSIZE);

		for (medial_iter = rand() % 3; medial_iter > 0; medial_iter--)
			mowgli_strlcat(namebuf, medial_sym[rand() % MEDIAL_SYM_SZ], BUFSIZE);

		mowgli_strlcat(namebuf, end_sym[rand() % END_SYM_SZ], BUFSIZE);

		if (iter == 0)
			mowgli_strlcpy(buf, namebuf, BUFSIZE);
		else
			mowgli_strlcat(buf, namebuf, BUFSIZE);

		mowgli_strlcat(buf, iter + 1 < amt ? ", " : ".", BUFSIZE);
	}

	gs_command_report(si, "Some names to ponder: %s", buf);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
