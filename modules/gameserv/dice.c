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

DECLARE_MODULE_V1
(
	"gameserv/dice", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void command_dice(sourceinfo_t *si, int parc, char *parv[]);
static void command_wod(sourceinfo_t *si, int parc, char *parv[]);
static void command_df(sourceinfo_t *si, int parc, char *parv[]);

command_t cmd_dice = { "ROLL", N_("Rolls one or more dice."), AC_NONE, 2, command_dice };
command_t cmd_wod = { "WOD", N_("WOD-style dice generation."), AC_NONE, 7, command_wod };
command_t cmd_df = { "DF", N_("Fudge-style dice generation."), AC_NONE, 2, command_df };

list_t *gs_helptree;
list_t *cs_helptree;

void _modinit(module_t * m)
{
	MODULE_USE_SYMBOL(gs_helptree, "gameserv/main", "gs_helptree");
	MODULE_USE_SYMBOL(cs_helptree, "chanserv/main", "cs_helptree");	/* fantasy commands */

	service_named_bind_command("gameserv", &cmd_dice);
	service_named_bind_command("gameserv", &cmd_wod);
	service_named_bind_command("gameserv", &cmd_df);
	help_addentry(gs_helptree, "ROLL", "help/gameserv/roll", NULL);

	service_named_bind_command("chanserv", &cmd_dice);
	service_named_bind_command("chanserv", &cmd_wod);
	service_named_bind_command("chanserv", &cmd_df);
	help_addentry(cs_helptree, "ROLL", "help/gameserv/roll", NULL);
}

void _moddeinit()
{
	service_named_unbind_command("gameserv", &cmd_dice);
	service_named_unbind_command("gameserv", &cmd_wod);
	service_named_unbind_command("gameserv", &cmd_df);
	help_delentry(gs_helptree, "ROLL");

	service_named_unbind_command("chanserv", &cmd_dice);
	service_named_unbind_command("chanserv", &cmd_wod);
	service_named_unbind_command("chanserv", &cmd_df);
	help_delentry(cs_helptree, "ROLL");
}

/*
 * Handle reporting for both fantasy commands and normal commands in GameServ
 * quickly and easily. Of course, sourceinfo has a vtable that can be manipulated,
 * but this is quicker and easier...                                  -- nenolod
 */
static void gs_command_report(sourceinfo_t *si, mychan_t *mc, const char *fmt, ...)
{
	va_list args;
	char buf[BUFSIZE];

	va_start(args, fmt);
	vsnprintf(buf, BUFSIZE, fmt, args);
	va_end(args);

	if (si->c != NULL)
		msg(chansvs.nick, si->c->name, "%s", buf);
	else if (mc != NULL)
		notice(si->service->nick, mc->name, "(%s) %s", si->su ? si->su->nick : get_source_name(si), buf);
	else
		command_success_nodata(si, "%s", buf);
}

static bool gs_do_parameters(sourceinfo_t *si, int *parc, char ***parv, mychan_t **pmc)
{
	mychan_t *mc;
	chanuser_t *cu;
	metadata_t *md;
	const char *who;
	bool allow;

	if (*parc == 0)
		return true;
	if ((*parv)[0][0] == '#')
	{
		mc = mychan_find((*parv)[0]);
		if (mc == NULL)
		{
			command_fail(si, fault_nosuch_target, _("Channel \2%s\2 is not registered."), (*parv)[0]);
			return false;
		}
		if (mc->chan == NULL)
		{
			command_fail(si, fault_nosuch_target, _("\2%s\2 is currently empty."), mc->name);
			return false;
		}
		if (si->c == NULL)
		{
			md = metadata_find(mc, "gameserv");
			if (md == NULL)
			{
				command_fail(si, fault_noprivs, _("%s is not enabled on \2%s\2."), "GAMESERV", mc->name);
				return false;
			}
			cu = chanuser_find(mc->chan, si->su);
			if (cu == NULL)
			{
				command_fail(si, fault_nosuch_target, _("You are not on \2%s\2."), mc->name);
				return false;
			}
			who = md->value;
			/* don't subvert +m; other modes can be subverted
			 * though
			 */
			if (mc->chan->modes & CMODE_MOD && !strcasecmp(who, "all"))
				who = "voice";
			if (!strcasecmp(who, "all"))
				allow = true;
			else if (!strcasecmp(who, "voice") || !strcmp(who, "1"))
				allow = cu->modes != 0 || chanacs_source_flags(mc, si) & (CA_AUTOOP | CA_OP | CA_AUTOVOICE | CA_VOICE);
			else if (!strcasecmp(who, "op"))
				allow = cu->modes & CSTATUS_OP || chanacs_source_flags(mc, si) & (CA_AUTOOP | CA_OP);
			else
			{
				command_fail(si, fault_noprivs, _("%s is not enabled on \2%s\2."), "GAMESERV", mc->name);
				return false;
			}
			if (!allow)
			{
				command_fail(si, fault_noprivs, _("You are not authorized to perform this operation."));
				return false;
			}
		}
		(*parc)--;
		(*parv)++;
		*pmc = mc;
	}
	else
		*pmc = NULL;
	return true;
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
		gs_command_report(si, mc, "(%s) + %d == \2%d\2", buf, modifier, (roll + modifier));
	else
		gs_command_report(si, mc, "%s == \2%d\2", buf, roll);
}

static void command_wod(sourceinfo_t *si, int parc, char *parv[])
{
	mychan_t *mc;
	char *arg_dice, *arg_difficulty;
	int ii = 0;
	int dice, difficulty;
	int roll, total, roll_count = 0, i;
	int success = 0, failure = 0, botches = 0, rerolls = 0;
	static char buf[BUFSIZE];
	char *end_p;

	if (!gs_do_parameters(si, &parc, &parv, &mc))
		return;
	if (parc < 2)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "WOD");
		command_fail(si, fault_needmoreparams, _("Syntax: WOD <dice> <difficulty>"));
		return;
	}
	arg_dice = parv[ii++];
	arg_difficulty = parv[ii++];

	while (roll_count < 3 && arg_dice != NULL && arg_difficulty != NULL)
	{
		success = 0, failure = 0, botches = 0, rerolls = 0;
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
				roll = (arc4random() % 10) + 1;

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

			gs_command_report(si, mc, _("%s rolls %d dice at difficulty %d: %s"), si->su->nick, dice, difficulty, buf);

			if (rerolls > 0)
				gs_command_report(si, mc, _("Successes: %d, Failures: %d, Botches: %d, Total: %d. You may reroll %d if you have a specialty."),
					success, failure, botches, total, rerolls);
			else
				gs_command_report(si, mc, _("Successes: %d, Failures: %d, Botches: %d, Total: %d."),
					success, failure, botches, total);
		}

		/* prepare for another go. */
		arg_dice = parv[ii++];
		arg_difficulty = parv[ii++];
	}
}

static const char *df_dice_table[3] = { "[-]", "[ ]", "[+]" };

static void command_df(sourceinfo_t *si, int parc, char *parv[])
{
	mychan_t *mc;
	char *arg_dice;
	char buf[BUFSIZE];
	int i, dice;

	if (!gs_do_parameters(si, &parc, &parv, &mc))
		return;
	if (parc < 1)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "DF");
		command_fail(si, fault_needmoreparams, _("Syntax: DF <dice>"));
		return;
	}
	arg_dice = parv[0];

	dice = atoi(arg_dice);
	*buf = '\0';

	if (dice > 30 || dice < 1)
	{
		command_fail(si, fault_badparams, _("Only 1-30 dice may be thrown at one time."));
		return;
	}

	for (i = 0; i < dice; i++)
	{
		int roll = arc4random() % 3;

		if (*buf != '\0')
			strlcat(buf, df_dice_table[roll], BUFSIZE);
		else
			strlcpy(buf, df_dice_table[roll], BUFSIZE);
	}

	gs_command_report(si, mc, _("Result: %s"), buf);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
