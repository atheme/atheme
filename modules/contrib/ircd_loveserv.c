/*
 * Copyright (c) 2006 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 * 
 * LoveServ implementation.
 *
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
        "contrib/ircd_loveserv", false, _modinit, _moddeinit,
        PACKAGE_STRING,
        "Atheme Development Group <http://www.atheme.org>"
);

service_t *loveserv;
mowgli_list_t conf_ls_table;

static void _ls_admirer(sourceinfo_t *si, int parc, char *parv[])
{
	user_t *u;
	char *target = parv[0];

	if (target == NULL)
	{
		notice(loveserv->nick, si->su->nick, STR_INSUFFICIENT_PARAMS, "ADMIRER");
		notice(loveserv->nick, si->su->nick, "Syntax: ADMIRER <target>");
		return;
	}

	if ((u = user_find_named(target)) == NULL)
	{
		notice(loveserv->nick, si->su->nick, "As much as I'd love to do this, you need to specify a person who really exists!");
		return;
	}

	notice(loveserv->nick, si->su->nick, "They have been told that they have a secret admirer. :)");
	notice(loveserv->nick, target, "You have a secret admirer ;)");
}

command_t ls_admirer = { "ADMIRER", "Tell somebody they have a secret admirer.", AC_NONE, 1, _ls_admirer, { .path = "" } };

static void _ls_rose(sourceinfo_t *si, int parc, char *parv[])
{
	user_t *u;
	char *target = parv[0];

	if (target == NULL)
	{
		notice(loveserv->nick, si->su->nick, STR_INSUFFICIENT_PARAMS, "ROSE");
		notice(loveserv->nick, si->su->nick, "Syntax: ROSE <target>");
		return;
	}

	if ((u = user_find_named(target)) == NULL)
	{
		notice(loveserv->nick, si->su->nick, "As much as I'd love to do this, you need to specify a person who really exists!");
		return;
	}

	notice(loveserv->nick, si->su->nick, "Your rose has been sent! :)");
	notice(loveserv->nick, target, "%s has sent you a pretty rose: \00303--<--<--<{\00304@", si->su->nick);
}

command_t ls_rose = { "ROSE", "Sends a rose to somebody.", AC_NONE, 1, _ls_rose, { .path = "" } };

static void _ls_chocolate(sourceinfo_t *si, int parc, char *parv[])
{
	user_t *u;
	char *target = parv[0];

	if (target == NULL)
	{
		notice(loveserv->nick, si->su->nick, STR_INSUFFICIENT_PARAMS, "CHOCOLATE");
		notice(loveserv->nick, si->su->nick, "Syntax: CHOCOLATE <target>");
		return;
	}

	if ((u = user_find_named(target)) == NULL)
	{
		notice(loveserv->nick, si->su->nick, "As much as I'd love to do this, you need to specify a person who really exists!");
		return;
	}

	notice(loveserv->nick, si->su->nick, "Your chocolates have been sent! :)");
	notice(loveserv->nick, target, "%s would like you to have this YUMMY box of chocolates.", si->su->nick);
}

command_t ls_chocolate = { "CHOCOLATE", "Sends chocolates to somebody.", AC_NONE, 1, _ls_chocolate, { .path = "" } };

static void _ls_candy(sourceinfo_t *si, int parc, char *parv[])
{
	user_t *u;
	char *target = parv[0];

	if (target == NULL)
	{
		notice(loveserv->nick, si->su->nick, STR_INSUFFICIENT_PARAMS, "CANDY");
		notice(loveserv->nick, si->su->nick, "Syntax: CANDY <target>");
		return;
	}

	if ((u = user_find_named(target)) == NULL)
	{
		notice(loveserv->nick, si->su->nick, "As much as I'd love to do this, you need to specify a person who really exists!");
		return;
	}

	notice(loveserv->nick, si->su->nick, "Your bag of candy has been sent! :)");
	notice(loveserv->nick, target, "%s would like you to have this bag of heart-shaped candies.", si->su->nick);
}

command_t ls_candy = { "CANDY", "Sends a bag of candy to somebody.", AC_NONE, 1, _ls_candy, { .path = "" } };

static void _ls_hug(sourceinfo_t *si, int parc, char *parv[])
{
	user_t *u;
	char *target = parv[0];

	if (target == NULL)
	{
		notice(loveserv->nick, si->su->nick, STR_INSUFFICIENT_PARAMS, "HUG");
		notice(loveserv->nick, si->su->nick, "Syntax: HUG <target>");
		return;
	}

	if ((u = user_find_named(target)) == NULL)
	{
		notice(loveserv->nick, si->su->nick, "As much as I'd love to do this, you need to specify a person who really exists!");
		return;
	}

	notice(loveserv->nick, si->su->nick, "You have virtually hugged %s!", target);
	notice(loveserv->nick, target, "%s has sent you a \002BIG WARM HUG\002.", si->su->nick);
}

command_t ls_hug = { "HUG", "Reach out and hug somebody.", AC_NONE, 1, _ls_hug, { .path = "" } };

static void _ls_kiss(sourceinfo_t *si, int parc, char *parv[])
{
	user_t *u;
	char *target = parv[0];

	if (target == NULL)
	{
		notice(loveserv->nick, si->su->nick, STR_INSUFFICIENT_PARAMS, "KISS");
		notice(loveserv->nick, si->su->nick, "Syntax: KISS <target>");
		return;
	}

	if ((u = user_find_named(target)) == NULL)
	{
		notice(loveserv->nick, si->su->nick, "As much as I'd love to do this, you need to specify a person who really exists!");
		return;
	}

	notice(loveserv->nick, si->su->nick, "You have virtually kissed %s!", target);
	notice(loveserv->nick, target, "%s has sent you a \00304kiss\003.", si->su->nick);
}

command_t ls_kiss = { "KISS", "Kiss somebody.", AC_NONE, 1, _ls_kiss, { .path = "" } };

static void _ls_lovenote(sourceinfo_t *si, int parc, char *parv[])
{
	user_t *u;
	char *target = parv[0];
	char *note = parv[1];

	if (target == NULL || note == NULL)
	{
		notice(loveserv->nick, si->su->nick, STR_INSUFFICIENT_PARAMS, "LOVENOTE");
		notice(loveserv->nick, si->su->nick, "Syntax: LOVENOTE <target> <message>");
		return;
	}

	if ((u = user_find_named(target)) == NULL)
	{
		notice(loveserv->nick, si->su->nick, "As much as I'd love to do this, you need to specify a person who really exists!");
		return;
	}

	notice(loveserv->nick, si->su->nick, "Your love-note to %s has been sent.", target);
	notice(loveserv->nick, target, "%s has sent you a love-note which reads: %s", si->su->nick, note);
}

command_t ls_lovenote = { "LOVENOTE", "Sends a lovenote to somebody.", AC_NONE, 2, _ls_lovenote, { .path = "" } };

static void _ls_apology(sourceinfo_t *si, int parc, char *parv[])
{
	user_t *u;
	char *target = parv[0];
	char *note = parv[1];

	if (target == NULL || note == NULL)
	{
		notice(loveserv->nick, si->su->nick, STR_INSUFFICIENT_PARAMS, "APOLOGY");
		notice(loveserv->nick, si->su->nick, "Syntax: APOLOGY <target> <message>");
		return;
	}

	if ((u = user_find_named(target)) == NULL)
	{
		notice(loveserv->nick, si->su->nick, "As much as I'd love to do this, you need to specify a person who really exists!");
		return;
	}

	notice(loveserv->nick, si->su->nick, "Your apology to %s has been sent.", target);
	notice(loveserv->nick, target, "%s would like to apologize for: %s", si->su->nick, note);
}

command_t ls_apology = { "APOLOGY", "Sends an apology to somebody.", AC_NONE, 2, _ls_apology, { .path = "" } };

static void _ls_thankyou(sourceinfo_t *si, int parc, char *parv[])
{
	user_t *u;
	char *target = parv[0];
	char *note = parv[1];

	if (target == NULL || note == NULL)
	{
		notice(loveserv->nick, si->su->nick, STR_INSUFFICIENT_PARAMS, "THANKYOU");
		notice(loveserv->nick, si->su->nick, "Syntax: THANKYOU <target> <message>");
		return;
	}

	if ((u = user_find_named(target)) == NULL)
	{
		notice(loveserv->nick, si->su->nick, "As much as I'd love to do this, you need to specify a person who really exists!");
		return;
	}

	notice(loveserv->nick, si->su->nick, "Your thank-you note to %s has been sent.", target);
	notice(loveserv->nick, target, "%s would like to thank you for: %s", si->su->nick, note);
}

command_t ls_thankyou = { "THANKYOU", "Sends a thank-you note to somebody.", AC_NONE, 2, _ls_thankyou, { .path = "" } };

static void _ls_spank(sourceinfo_t *si, int parc, char *parv[])
{
	user_t *u;
	char *target = parv[0];

	if (target == NULL)
	{
		notice(loveserv->nick, si->su->nick, STR_INSUFFICIENT_PARAMS, "SPANK");
		notice(loveserv->nick, si->su->nick, "Syntax: SPANK <target>");
		return;
	}

	if ((u = user_find_named(target)) == NULL)
	{
		notice(loveserv->nick, si->su->nick, "As much as I'd love to do this, you need to specify a person who really exists!");
		return;
	}

	notice(loveserv->nick, si->su->nick, "You have virtually spanked %s!", target);
	notice(loveserv->nick, target, "%s has given you a virtual playful spanking.", si->su->nick);
}

command_t ls_spank = { "SPANK", "Gives somebody a spanking.", AC_NONE, 1, _ls_spank, { .path = "" } };

static void _ls_chocobo(sourceinfo_t *si, int parc, char *parv[])	/* silly */
{
	user_t *u;
	char *target = parv[0];

	if (target == NULL)
	{
		notice(loveserv->nick, si->su->nick, STR_INSUFFICIENT_PARAMS, "CHOCOBO");
		notice(loveserv->nick, si->su->nick, "Syntax: CHOCOBO <target>");
		return;
	}

	if ((u = user_find_named(target)) == NULL)
	{
		notice(loveserv->nick, si->su->nick, "As much as I'd love to do this, you need to specify a person who really exists!");
		return;
	}

	notice(loveserv->nick, si->su->nick, "Your chocobo has been sent to %s.", target);
	notice(loveserv->nick, target, "%s would like you to have this chocobo. \00308Kweh!\003", si->su->nick);
}

command_t ls_chocobo = { "CHOCOBO", "Sends a chocobo to somebody.", AC_NONE, 1, _ls_chocobo, { .path = "" } };

static void _ls_help(sourceinfo_t *si, int parc, char *parv[])
{
        command_help(si, si->service->commands);
}

command_t ls_help = { "HELP", "Displays contextual help information.",
                                AC_NONE, 1, _ls_help, { .path = "help" } };

void _modinit(module_t *m)
{
        loveserv = service_add("LoveServ", NULL, &conf_ls_table);

	service_bind_command(loveserv, &ls_admirer);
	service_bind_command(loveserv, &ls_rose);
	service_bind_command(loveserv, &ls_chocolate);
	service_bind_command(loveserv, &ls_candy);
	service_bind_command(loveserv, &ls_hug);
	service_bind_command(loveserv, &ls_kiss);
	service_bind_command(loveserv, &ls_lovenote);
	service_bind_command(loveserv, &ls_apology);
	service_bind_command(loveserv, &ls_thankyou);
	service_bind_command(loveserv, &ls_spank);
	service_bind_command(loveserv, &ls_chocobo);
	service_bind_command(loveserv, &ls_help);
}

void _moddeinit()
{
	if (loveserv)
		service_delete(loveserv);
	
	service_unbind_command(loveserv, &ls_admirer);
	service_unbind_command(loveserv, &ls_rose);
	service_unbind_command(loveserv, &ls_chocolate);
	service_unbind_command(loveserv, &ls_candy);
	service_unbind_command(loveserv, &ls_hug);
	service_unbind_command(loveserv, &ls_kiss);
	service_unbind_command(loveserv, &ls_lovenote);
	service_unbind_command(loveserv, &ls_apology);
	service_unbind_command(loveserv, &ls_thankyou);
	service_unbind_command(loveserv, &ls_spank);
	service_unbind_command(loveserv, &ls_chocobo);
	service_unbind_command(loveserv, &ls_help);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
