/*
 * Copyright (c) 2005-2006 William Pitcock <nenolod@nenolod.net> et al
 * Rights to this code are documented in doc/LICENSE.
 *
 * A Magic 8 Ball. Oh noes!
 *
 */

#include "atheme.h"
#include "gameserv_common.h"

DECLARE_MODULE_V1
(
	"gameserv/eightball", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void command_eightball(sourceinfo_t *si, int parc, char *parv[]);

command_t cmd_eightball = { "EIGHTBALL", N_("Ask the 8-Ball a question."), AC_NONE, 2, command_eightball, { .path = "gameserv/eightball" } };

void _modinit(module_t * m)
{
	service_named_bind_command("gameserv", &cmd_eightball);

	service_named_bind_command("chanserv", &cmd_eightball);
}

void _moddeinit(module_unload_intent_t intent)
{
	service_named_unbind_command("gameserv", &cmd_eightball);

	service_named_unbind_command("chanserv", &cmd_eightball);
}

static void command_eightball(sourceinfo_t *si, int parc, char *parv[])
{
	mychan_t *mc;

	if (!gs_do_parameters(si, &parc, &parv, &mc))
		return;

	static const char *eightball_responses[28] = {
		N_("Absolutely yes!"),
		N_("Prospect looks hopeful."),
		N_("I'd like to think so."),
		N_("Yes, yes, yes, and yes again."),
		N_("Most likely."),
		N_("All signs point to yes."),
		N_("Yes."),
		N_("Without a doubt."),
		N_("Sometime in the near future."),
		N_("Of course!"),
		N_("Definitely."),
		N_("Answer hazy."),
		N_("Prospect looks bleak."),
		N_("That's a question you should ask yourself."),
		N_("Maybe."),
		N_("That question is better remained unanswered."),
		N_("The stars would have to align for that to happen."),
		N_("No."),
		N_("Not even on a GOOD day."),
		N_("It would take a disturbed person to even ask."),
		N_("You wish."),
		N_("Not bloody likely."),
		N_("No way."),
		N_("Never."),
		N_("NO!"),
		N_("Over my dead body."),
		N_("We won't go there"),
		N_("No chance at all!")
	};

	gs_command_report(si, "%s", eightball_responses[rand() % 28]);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
