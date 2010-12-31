/*
 * Copyright (c) 2005-2007 William Pitcock <nenolod@nenolod.net>
 * Copyright (c) 2006-2007 Jilles Tjoelker <jilles@stack.nl>
 * 
 * Rights to this code are documented in doc/LICENSE.
 *
 * Dice generator.
 *
 */

#include "atheme.h"
#include "gameserv_common.h"

DECLARE_MODULE_V1
(
	"gameserv/dice", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void command_dice(sourceinfo_t *si, int parc, char *parv[]);

command_t cmd_dice = { "ROLL", N_("Rolls one or more dice."), AC_NONE, 2, command_dice, { .path = "gameserv/roll" } };

void _modinit(module_t * m)
{
	service_named_bind_command("gameserv", &cmd_dice);

	service_named_bind_command("chanserv", &cmd_dice);
}

void _moddeinit(module_unload_intent_t intent)
{
	service_named_unbind_command("gameserv", &cmd_dice);

	service_named_unbind_command("chanserv", &cmd_dice);
}

static void command_dice(sourceinfo_t *si, int parc, char *parv[])
{
	char *arg;
	mychan_t *mc;
	int dice = 0, sides = 0, i = 0, roll = 0, modifier = 0;
	char buf[BUFSIZE];

	if (!gs_do_parameters(si, &parc, &parv, &mc))
		return;
	if (parc < 1)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "ROLL");
		command_fail(si, fault_needmoreparams, _("Syntax: ROLL [dice]d<sides>"));
		return;
	}
	arg = parv[0];

	sscanf(arg, "%dd%d+%d", &dice, &sides, &modifier);

	if (dice <= 0)
	{
		modifier = 0;
		sscanf(arg, "%dd%d", &dice, &sides);

		if (dice <= 0)
		{
			dice = 1;

			sscanf(arg, "d%d+%d", &sides, &modifier);

			if (sides <= 0)
				sscanf(arg, "d%d", &sides);
		}
	}

	if (dice > 100)
		dice = 100;
	if (sides > 100)
		sides = 100;

	if (dice <= 0 || sides <= 0)
	{
		dice = 1;
		sides = 1;
	}

	*buf = '\0';

	for (i = 0; i < dice; i++)
	{
		unsigned int y = 1 + arc4random() % sides;
		char buf2[BUFSIZE];

		if (*buf != '\0')
		{
			snprintf(buf2, BUFSIZE, ", %d", y);
			strlcat(buf, buf2, BUFSIZE);
		}
		else
		{
			snprintf(buf2, BUFSIZE, "%d", y);
			strlcpy(buf, buf2, BUFSIZE);
		}

		roll += y;
	}

	if (modifier != 0)
		gs_command_report(si, "(%s) + %d == \2%d\2", buf, modifier, (roll + modifier));
	else
		gs_command_report(si, "%s == \2%d\2", buf, roll);
}
