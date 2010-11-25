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
	char *target = parv[0];

	if (!target)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "ADMIRER");
		command_fail(si, fault_needmoreparams, "Syntax: ADMIRER <target>");
		return;
	}

	if (!user_find_named(target))
	{
		command_fail(si, fault_nosuch_target, "As much as I'd love to do this, you need to specify a person who really exists!");
		return;
	}

	command_success_nodata(si, "%s has been told that they have a secret admirer. :)", target);
	notice(loveserv->nick, target, "You have a secret admirer ;)");
}

command_t ls_admirer = { "ADMIRER", "Tell somebody they have a secret admirer.", AC_NONE, 1, _ls_admirer, { .path = "" } };

static void _ls_rose(sourceinfo_t *si, int parc, char *parv[])
{
	char *target = parv[0];

	if (!target)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "ROSE");
		command_fail(si, fault_needmoreparams, "Syntax: ROSE <target>");
		return;
	}

	if (!user_find_named(target))
	{
		command_fail(si, fault_nosuch_target, "As much as I'd love to do this, you need to specify a person who really exists!");
		return;
	}

	command_success_nodata(si, "Your rose has been sent to %s! :)", target);
	notice(loveserv->nick, target, "%s has sent you a pretty rose: \00303--<--<--<{\00304@", si->su->nick);
}

command_t ls_rose = { "ROSE", "Sends a rose to somebody.", AC_NONE, 1, _ls_rose, { .path = "" } };

static void _ls_chocolate(sourceinfo_t *si, int parc, char *parv[])
{
	char *target = parv[0];

	if (!target)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "CHOCOLATE");
		command_fail(si, fault_needmoreparams, "Syntax: CHOCOLATE <target>");
		return;
	}

	if (!user_find_named(target))
	{
		command_fail(si, fault_nosuch_target, "As much as I'd love to do this, you need to specify a person who really exists!");
		return;
	}

	command_success_nodata(si, "Your chocolates have been sent to %s! :)", target);
	notice(loveserv->nick, target, "%s would like you to have this YUMMY box of chocolates.", si->su->nick);
}

command_t ls_chocolate = { "CHOCOLATE", "Sends chocolates to somebody.", AC_NONE, 1, _ls_chocolate, { .path = "" } };

static void _ls_candy(sourceinfo_t *si, int parc, char *parv[])
{
	char *target = parv[0];

	if (!target)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "CANDY");
		command_fail(si, fault_needmoreparams, "Syntax: CANDY <target>");
		return;
	}

	if (!user_find_named(target))
	{
		command_fail(si, fault_nosuch_target, "As much as I'd love to do this, you need to specify a person who really exists!");
		return;
	}

	command_success_nodata(si, "Your bag of candy has been sent to %s! :)", target);
	notice(loveserv->nick, target, "%s would like you to have this bag of heart-shaped candies.", si->su->nick);
}

command_t ls_candy = { "CANDY", "Sends a bag of candy to somebody.", AC_NONE, 1, _ls_candy, { .path = "" } };

static void _ls_hug(sourceinfo_t *si, int parc, char *parv[])
{
	char *target = parv[0];

	if (!target)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "HUG");
		command_fail(si, fault_needmoreparams, "Syntax: HUG <target>");
		return;
	}

	if (!user_find_named(target))
	{
		command_fail(si, fault_nosuch_target, "As much as I'd love to do this, you need to specify a person who really exists!");
		return;
	}

	command_success_nodata(si, "You have virtually hugged %s!", target);
	notice(loveserv->nick, target, "%s has sent you a \002BIG WARM HUG\002.", si->su->nick);
}

command_t ls_hug = { "HUG", "Reach out and hug somebody.", AC_NONE, 1, _ls_hug, { .path = "" } };

static void _ls_kiss(sourceinfo_t *si, int parc, char *parv[])
{
	char *target = parv[0];

	if (!target)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "KISS");
		command_fail(si, fault_needmoreparams, "Syntax: KISS <target>");
		return;
	}

	if (!user_find_named(target))
	{
		command_fail(si, fault_nosuch_target, "As much as I'd love to do this, you need to specify a person who really exists!");
		return;
	}

	command_success_nodata(si, "You have virtually kissed %s!", target);
	notice(loveserv->nick, target, "%s has sent you a \00304kiss\003.", si->su->nick);
}

command_t ls_kiss = { "KISS", "Kiss somebody.", AC_NONE, 1, _ls_kiss, { .path = "" } };

static void _ls_lovenote(sourceinfo_t *si, int parc, char *parv[])
{
	char *target = parv[0];
	char *note = parv[1];

	if (!target || !note)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "LOVENOTE");
		command_fail(si, fault_needmoreparams, "Syntax: LOVENOTE <target> <message>");
		return;
	}

	if (!user_find_named(target))
	{
		command_fail(si, fault_nosuch_target, "As much as I'd love to do this, you need to specify a person who really exists!");
		return;
	}

	command_success_nodata(si, "Your love-note to %s has been sent.", target);
	notice(loveserv->nick, target, "%s has sent you a love-note which reads: %s", si->su->nick, note);
}

command_t ls_lovenote = { "LOVENOTE", "Sends a lovenote to somebody.", AC_NONE, 2, _ls_lovenote, { .path = "" } };

static void _ls_apology(sourceinfo_t *si, int parc, char *parv[])
{
	char *target = parv[0];
	char *note = parv[1];

	if (!target || !note)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "APOLOGY");
		command_fail(si, fault_needmoreparams, "Syntax: APOLOGY <target> <message>");
		return;
	}

	if (!user_find_named(target))
	{
		command_fail(si, fault_nosuch_target, "As much as I'd love to do this, you need to specify a person who really exists!");
		return;
	}

	command_success_nodata(si, "Your apology to %s has been sent.", target);
	notice(loveserv->nick, target, "%s would like to apologize for: %s", si->su->nick, note);
}

command_t ls_apology = { "APOLOGY", "Sends an apology to somebody.", AC_NONE, 2, _ls_apology, { .path = "" } };

static void _ls_thankyou(sourceinfo_t *si, int parc, char *parv[])
{
	char *target = parv[0];
	char *note = parv[1];

	if (!target || !note)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "THANKYOU");
		command_fail(si, fault_needmoreparams, "Syntax: THANKYOU <target> <message>");
		return;
	}

	if (!user_find_named(target))
	{
		command_fail(si, fault_nosuch_target, "As much as I'd love to do this, you need to specify a person who really exists!");
		return;
	}

	command_success_nodata(si, "Your thank-you note to %s has been sent.", target);
	notice(loveserv->nick, target, "%s would like to thank you for: %s", si->su->nick, note);
}

command_t ls_thankyou = { "THANKYOU", "Sends a thank-you note to somebody.", AC_NONE, 2, _ls_thankyou, { .path = "" } };

static void _ls_spank(sourceinfo_t *si, int parc, char *parv[])
{
	char *target = parv[0];

	if (!target)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SPANK");
		command_fail(si, fault_needmoreparams, "Syntax: SPANK <target>");
		return;
	}

	if (!user_find_named(target))
	{
		command_fail(si, fault_nosuch_target, "As much as I'd love to do this, you need to specify a person who really exists!");
		return;
	}

	command_success_nodata(si, "You have virtually spanked %s!", target);
	notice(loveserv->nick, target, "%s has given you a virtual playful spanking.", si->su->nick);
}

command_t ls_spank = { "SPANK", "Gives somebody a spanking.", AC_NONE, 1, _ls_spank, { .path = "" } };

static void _ls_chocobo(sourceinfo_t *si, int parc, char *parv[])	/* silly */
{
	char *target = parv[0];

	if (!target)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "CHOCOBO");
		command_fail(si, fault_needmoreparams, "Syntax: CHOCOBO <target>");
		return;
	}

	if (!user_find_named(target))
	{
		command_fail(si, fault_nosuch_target, "As much as I'd love to do this, you need to specify a person who really exists!");
		return;
	}

	command_success_nodata(si, "Your chocobo has been sent to %s.", target);
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
