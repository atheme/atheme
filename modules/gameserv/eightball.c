/*
 * Copyright (c) 2005-2006 William Pitcock <nenolod@nenolod.net> et al
 * Rights to this code are documented in doc/LICENSE.
 *
 * A Magic 8 Ball. Oh noes!
 *
 * $Id: dice.c 7449 2007-01-13 04:00:04Z nenolod $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"gameserv/eightball", FALSE, _modinit, _moddeinit,
	"$Id: dice.c 7449 2007-01-13 04:00:04Z nenolod $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void command_eightball(sourceinfo_t *si, int parc, char *parv[]);

command_t cmd_eightball = { "EIGHTBALL", "Ask the 8-Ball a question.", AC_NONE, 0, command_eightball };

list_t *gs_cmdtree;
list_t *cs_cmdtree;

void _modinit(module_t * m)
{
	MODULE_USE_SYMBOL(gs_cmdtree, "gameserv/main", "gs_cmdtree");
	MODULE_USE_SYMBOL(cs_cmdtree, "chanserv/main", "cs_cmdtree");	/* fantasy commands */

	command_add(&cmd_eightball, gs_cmdtree);

	command_add(&cmd_eightball, cs_cmdtree);
}

void _moddeinit()
{
	command_delete(&cmd_eightball, gs_cmdtree);

	command_delete(&cmd_eightball, cs_cmdtree);
}

/*
 * Handle reporting for both fantasy commands and normal commands in GameServ
 * quickly and easily. Of course, sourceinfo has a vtable that can be manipulated,
 * but this is quicker and easier...                                  -- nenolod
 */
static void gs_command_report(sourceinfo_t *si, char *fmt, ...)
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

static void command_eightball(sourceinfo_t *si, int parc, char *parv[])
{
	static char *eightball_responses[28] = {
		"Absolutely yes!",
		"Prospect looks hopeful.",
		"I'd like to think so.",
		"Yes, yes, yes, and yes again.",
		"Most likely.",
		"All signs point to yes.",
		"Yes.",
		"Without a doubt.",
		"Sometime in the near future.",
		"Of course!",
		"Definitely.",
		"Answer hazy.",
		"Prospect looks bleak.",
		"That's a question you should ask yourself.",
		"Maybe.",
		"That question is better remained unanswered.",
		"The stars would have to align for that to happen.",
		"No.",
		"Not even on a GOOD day.",
		"It would take a disturbed person to even ask.",
		"You wish.",
		"Not bloody likely.",
		"No way.",
		"Never.",
		"NO!",
		"Over my dead body.",
		"We won't go there",
		"No chance at all!"
	};

	gs_command_report(si, "%s", eightball_responses[rand() % 28]);
}
