/*
 * Copyright (c) 2006 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 * 
 * LoveServ implementation.
 *
 */

#include "atheme.h"

/* CONFIG SECTION */
#define LS_HOST		"services.int"

DECLARE_MODULE_V1
(
        "contrib/ircd_loveserv", false, _modinit, _moddeinit,
        PACKAGE_STRING,
        "Atheme Development Group <http://www.atheme.org>"
);

service_t *loveserv;
list_t ls_cmdtree;

static void _ls_admirer(char *origin)
{
	user_t *u;
	char *target = strtok(NULL, " ");

	if (target == NULL)
	{
		notice(loveserv->name, origin, STR_INSUFFICIENT_PARAMS, "ADMIRER");
		notice(loveserv->name, origin, "Syntax: ADMIRER <target>");
		return;
	}

	if ((u = user_find_named(target)) == NULL)
	{
		notice(loveserv->name, origin, "As much as I'd love to do this, you need to specify a person who really exists!");
		return;
	}

	notice(loveserv->name, origin, "They have been told that they have a secret admirer. :)");
	notice(loveserv->name, target, "You have a secret admirer ;)");
}

command_t ls_admirer = { "ADMIRER", "Tell somebody they have a secret admirer.", AC_NONE, _ls_admirer };

static void _ls_rose(char *origin)
{
	user_t *u;
	char *target = strtok(NULL, " ");

	if (target == NULL)
	{
		notice(loveserv->name, origin, STR_INSUFFICIENT_PARAMS, "ROSE");
		notice(loveserv->name, origin, "Syntax: ROSE <target>");
		return;
	}

	if ((u = user_find_named(target)) == NULL)
	{
		notice(loveserv->name, origin, "As much as I'd love to do this, you need to specify a person who really exists!");
		return;
	}

	notice(loveserv->name, origin, "Your rose has been sent! :)");
	notice(loveserv->name, target, "%s has sent you a pretty rose: \00303--<--<--<{\00304@", origin);
}

command_t ls_rose = { "ROSE", "Sends a rose to somebody.", AC_NONE, _ls_rose };

static void _ls_chocolate(char *origin)
{
	user_t *u;
	char *target = strtok(NULL, " ");

	if (target == NULL)
	{
		notice(loveserv->name, origin, STR_INSUFFICIENT_PARAMS, "CHOCOLATE");
		notice(loveserv->name, origin, "Syntax: CHOCOLATE <target>");
		return;
	}

	if ((u = user_find_named(target)) == NULL)
	{
		notice(loveserv->name, origin, "As much as I'd love to do this, you need to specify a person who really exists!");
		return;
	}

	notice(loveserv->name, origin, "Your chocolates have been sent! :)");
	notice(loveserv->name, target, "%s would like you to have this YUMMY box of chocolates.", origin);
}

command_t ls_chocolate = { "CHOCOLATE", "Sends chocolates to somebody.", AC_NONE, _ls_chocolate };

static void _ls_candy(char *origin)
{
	user_t *u;
	char *target = strtok(NULL, " ");

	if (target == NULL)
	{
		notice(loveserv->name, origin, STR_INSUFFICIENT_PARAMS, "CANDY");
		notice(loveserv->name, origin, "Syntax: CANDY <target>");
		return;
	}

	if ((u = user_find_named(target)) == NULL)
	{
		notice(loveserv->name, origin, "As much as I'd love to do this, you need to specify a person who really exists!");
		return;
	}

	notice(loveserv->name, origin, "Your bag of candy has been sent! :)");
	notice(loveserv->name, target, "%s would like you to have this bag of heart-shaped candies.", origin);
}

command_t ls_candy = { "CANDY", "Sends a bag of candy to somebody.", AC_NONE, _ls_candy };

static void _ls_hug(char *origin)
{
	user_t *u;
	char *target = strtok(NULL, " ");

	if (target == NULL)
	{
		notice(loveserv->name, origin, STR_INSUFFICIENT_PARAMS, "HUG");
		notice(loveserv->name, origin, "Syntax: HUG <target>");
		return;
	}

	if ((u = user_find_named(target)) == NULL)
	{
		notice(loveserv->name, origin, "As much as I'd love to do this, you need to specify a person who really exists!");
		return;
	}

	notice(loveserv->name, origin, "You have virtually hugged %s!", target);
	notice(loveserv->name, target, "%s has sent you a \002BIG WARM HUG\002.", origin);
}

command_t ls_hug = { "HUG", "Reach out and hug somebody.", AC_NONE, _ls_hug };

static void _ls_kiss(char *origin)
{
	user_t *u;
	char *target = strtok(NULL, " ");

	if (target == NULL)
	{
		notice(loveserv->name, origin, STR_INSUFFICIENT_PARAMS, "KISS");
		notice(loveserv->name, origin, "Syntax: KISS <target>");
		return;
	}

	if ((u = user_find_named(target)) == NULL)
	{
		notice(loveserv->name, origin, "As much as I'd love to do this, you need to specify a person who really exists!");
		return;
	}

	notice(loveserv->name, origin, "You have virtually kissed %s!", target);
	notice(loveserv->name, target, "%s has sent you a \00304kiss\003.", origin);
}

command_t ls_kiss = { "KISS", "Kiss somebody.", AC_NONE, _ls_kiss };

static void _ls_lovenote(char *origin)
{
	user_t *u;
	char *target = strtok(NULL, " ");
	char *note = strtok(NULL, "");

	if (target == NULL || note == NULL)
	{
		notice(loveserv->name, origin, STR_INSUFFICIENT_PARAMS, "LOVENOTE");
		notice(loveserv->name, origin, "Syntax: LOVENOTE <target> <message>");
		return;
	}

	if ((u = user_find_named(target)) == NULL)
	{
		notice(loveserv->name, origin, "As much as I'd love to do this, you need to specify a person who really exists!");
		return;
	}

	notice(loveserv->name, origin, "Your love-note to %s has been sent.", target);
	notice(loveserv->name, target, "%s has sent you a love-note which reads: %s", origin, note);
}

command_t ls_lovenote = { "LOVENOTE", "Sends a lovenote to somebody.", AC_NONE, _ls_lovenote };

static void _ls_apology(char *origin)
{
	user_t *u;
	char *target = strtok(NULL, " ");
	char *note = strtok(NULL, "");

	if (target == NULL || note == NULL)
	{
		notice(loveserv->name, origin, STR_INSUFFICIENT_PARAMS, "APOLOGY");
		notice(loveserv->name, origin, "Syntax: APOLOGY <target> <message>");
		return;
	}

	if ((u = user_find_named(target)) == NULL)
	{
		notice(loveserv->name, origin, "As much as I'd love to do this, you need to specify a person who really exists!");
		return;
	}

	notice(loveserv->name, origin, "Your apology to %s has been sent.", target);
	notice(loveserv->name, target, "%s would like to apologize for: %s", origin, note);
}

command_t ls_apology = { "APOLOGY", "Sends an apology to somebody.", AC_NONE, _ls_apology };

static void _ls_thankyou(char *origin)
{
	user_t *u;
	char *target = strtok(NULL, " ");
	char *note = strtok(NULL, "");

	if (target == NULL || note == NULL)
	{
		notice(loveserv->name, origin, STR_INSUFFICIENT_PARAMS, "THANKYOU");
		notice(loveserv->name, origin, "Syntax: THANKYOU <target> <message>");
		return;
	}

	if ((u = user_find_named(target)) == NULL)
	{
		notice(loveserv->name, origin, "As much as I'd love to do this, you need to specify a person who really exists!");
		return;
	}

	notice(loveserv->name, origin, "Your thank-you note to %s has been sent.", target);
	notice(loveserv->name, target, "%s would like to thank you for: %s", origin, note);
}

command_t ls_thankyou = { "THANKYOU", "Sends a thank-you note to somebody.", AC_NONE, _ls_thankyou };

static void _ls_spank(char *origin)
{
	user_t *u;
	char *target = strtok(NULL, " ");

	if (target == NULL)
	{
		notice(loveserv->name, origin, STR_INSUFFICIENT_PARAMS, "SPANK");
		notice(loveserv->name, origin, "Syntax: SPANK <target>");
		return;
	}

	if ((u = user_find_named(target)) == NULL)
	{
		notice(loveserv->name, origin, "As much as I'd love to do this, you need to specify a person who really exists!");
		return;
	}

	notice(loveserv->name, origin, "You have virtually spanked %s!", target);
	notice(loveserv->name, target, "%s has given you a virtual playful spanking.", origin);
}

command_t ls_spank = { "SPANK", "Gives somebody a spanking.", AC_NONE, _ls_spank };

static void _ls_chocobo(char *origin)	/* silly */
{
	user_t *u;
	char *target = strtok(NULL, " ");

	if (target == NULL)
	{
		notice(loveserv->name, origin, STR_INSUFFICIENT_PARAMS, "CHOCOBO");
		notice(loveserv->name, origin, "Syntax: CHOCOBO <target>");
		return;
	}

	if ((u = user_find_named(target)) == NULL)
	{
		notice(loveserv->name, origin, "As much as I'd love to do this, you need to specify a person who really exists!");
		return;
	}

	notice(loveserv->name, origin, "Your chocobo has been sent to %s.", target);
	notice(loveserv->name, target, "%s would like you to have this chocobo. \00308Kweh!\003", origin);
}

command_t ls_chocobo = { "CHOCOBO", "Sends a chocobo to somebody.", AC_NONE, _ls_chocobo };

static void _ls_help(char *origin)
{
        command_help(loveserv->name, origin, &ls_cmdtree);
}

command_t ls_help = { "HELP", "Displays contextual help information.",
                                AC_NONE, _ls_help };

static void ls_handler(char *origin, int parc, char *parv[])
{
        char *cmd;
        char orig[BUFSIZE];

        /* this should never happen */
        if (parv[0][0] == '&')
        {
                slog(LG_ERROR, "services(): got parv with local channel: %s", parv[0]);
                return;
        }

        /* make a copy of the original for debugging */
        strlcpy(orig, parv[parc - 1], BUFSIZE);

        /* lets go through this to get the command */
        cmd = strtok(parv[parc - 1], " ");

        if (!cmd)
                return;

        /* take the command through the hash table */
        command_exec(loveserv, origin, cmd, &ls_cmdtree);
}

command_t *ls_commands[] = {
	&ls_admirer,
	&ls_rose,
	&ls_chocolate,
	&ls_candy,
	&ls_hug,
	&ls_kiss,
	&ls_lovenote,
	&ls_apology,
	&ls_thankyou,
	&ls_spank,
	&ls_chocobo,
	&ls_help,
	NULL
};

void _modinit(module_t *m)
{
        loveserv = add_service("LoveServ", "LoveServ", LS_HOST, "LoveServ, etc.", ls_handler);

	command_add_many(ls_commands, &ls_cmdtree);
}

void _moddeinit()
{
	command_delete_many(ls_commands, &ls_cmdtree);

        del_service(loveserv);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
