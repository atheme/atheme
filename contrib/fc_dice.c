/*
 * Copyright (c) 2005 William Pitcock <nenolod@nenolod.net>
 * Rights to this code are documented in doc/LICENSE.
 *
 * Dice generator fantasy command.
 *
 * $Id: fc_dice.c 814 2005-07-16 03:37:24Z nenolod $
 */

#include "atheme.h"

static void fcommand_dice(char *origin, char *channel);

fcommand_t fc_dice = { "!roll", AC_NONE, fcommand_dice };

extern list_t cs_fcmdtree;

void _modinit(module_t * m)
{
	fcommand_add(&fc_dice, &cs_fcmdtree);
}

void _moddeinit()
{
	fcommand_delete(&fc_dice, &cs_fcmdtree);
}

static void fcommand_dice(char *origin, char *channel)
{
	char *arg = strtok(NULL, " ");
	int32_t dice, sides, i, roll = 1;

	if (!arg)
		return;

	sscanf(arg, "%dd%d", &dice, &sides);

	if (dice < 0)
	{
		dice = 1;
		sscanf(arg, "d%d", &sides);
	}

	srand(time(0));

	for (i = 0; i < dice; i++)
		roll += (rand() % sides);

	notice(chansvs.nick, channel, "%d", roll);
}
