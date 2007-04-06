/*
 * Copyright (c) 2005-2006 William Pitcock <nenolod@nenolod.net> et al
 * Rights to this code are documented in doc/LICENSE.
 *
 * Dice generator.
 *
 * $Id: dice.c 8131 2007-04-06 02:09:53Z nenolod $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"gameserv/dice", FALSE, _modinit, _moddeinit,
	"$Id: dice.c 8131 2007-04-06 02:09:53Z nenolod $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void command_dice(sourceinfo_t *si, int parc, char *parv[]);
static void command_wod(sourceinfo_t *si, int parc, char *parv[]);

command_t cmd_dice = { "ROLL", N_("Rolls one or more dice."), AC_NONE, 2, command_dice };
command_t cmd_wod = { "WOD", N_("WOD-style dice generation."), AC_NONE, 7, command_wod };

list_t *gs_cmdtree;
list_t *cs_cmdtree;

void _modinit(module_t * m)
{
	MODULE_USE_SYMBOL(gs_cmdtree, "gameserv/main", "gs_cmdtree");
	MODULE_USE_SYMBOL(cs_cmdtree, "chanserv/main", "cs_cmdtree");	/* fantasy commands */

	command_add(&cmd_dice, gs_cmdtree);
	command_add(&cmd_wod, gs_cmdtree);

	command_add(&cmd_dice, cs_cmdtree);
	command_add(&cmd_wod, cs_cmdtree);
}

void _moddeinit()
{
	command_delete(&cmd_dice, gs_cmdtree);
	command_delete(&cmd_wod, gs_cmdtree);

	command_delete(&cmd_dice, cs_cmdtree);
	command_delete(&cmd_wod, cs_cmdtree);
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

static void command_dice(sourceinfo_t *si, int parc, char *parv[])
{
	char *arg = si->c != NULL ? parv[1] : parv[0];
	int dice = 0, sides = 0, i = 0, roll = 0, modifier = 0;
	char buf[BUFSIZE];

	if (!arg)
		return;
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

	if (dice > 256)
		dice = 256;

	if (!dice || !sides)
	{
		dice = 1;
		sides = 1;
	}

	*buf = '\0';

	for (i = 0; i < dice; i++)
	{
		int y = 0;
		char buf2[BUFSIZE];

		while (y == 0)
			y = (arc4random() % sides);

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

static void command_wod(sourceinfo_t *si, int parc, char *parv[])
{
	int ii = si->c != NULL ? 1 : 0;
	char *arg_dice = parv[ii++];
	char *arg_difficulty = parv[ii++];

	int dice, difficulty;
	int roll, total = 0, roll_count = 0, i;
	int success = 0, failure = 0, botches = 0, rerolls = 0;

	static char buf[BUFSIZE];
	char *end_p;

	if (arg_dice == NULL || arg_difficulty == NULL)
	{
		command_fail(si, fault_needmoreparams, _("Syntax: WOD <dice> <difficulty>"));
		return;
	}

	while (roll_count < 3 && arg_dice != NULL && arg_difficulty != NULL)
	{
		total = 0, success = 0, failure = 0, botches = 0, rerolls = 0;
		roll_count++;

		dice = atoi(arg_dice);
		difficulty = atoi(arg_difficulty);

		if (dice > 30 || dice < 1)
		{
			command_fail(si, fault_badparams, _("Only 1-30 dice may be thrown at one time."));
			return;
		}
		else if (difficulty > 10 || difficulty < 1)
		{
			command_fail(si, fault_badparams, _("Difficulty setting must be between 1 and 10."));
			return;
		}
		else
		{
			end_p = buf;

			for (i = 0; i < dice; i++)
			{
				roll = (rand() % 10) + 1;

				end_p += snprintf(end_p, BUFSIZE - (end_p - buf), "%d  ", roll);

				if (roll == 1)
				{
					botches++;
					continue;
				}
				else if (roll == 10)
					rerolls++;

				if (roll >= difficulty)
					success++;
				else
					failure++;
			}

			rerolls = rerolls - botches;
			total = success - botches;

			gs_command_report(si, _("%s rolls %d dice at difficulty %d: %s"), si->su->nick, dice, difficulty, buf);

			if (rerolls > 0)
				gs_command_report(si, _("Successes: %d, Failures: %d, Botches: %d, Total: %d. You may reroll %d if you have a specialty."),
					success, failure, botches, total, rerolls);
			else
				gs_command_report(si, _("Successes: %d, Failures: %d, Botches: %d, Total: %d."),
					success, failure, botches, total);
		}

		/* prepare for another go. */
		arg_dice = parv[ii++];
		arg_difficulty = parv[ii++];
	}
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
