/*
 * Copyright (c) 2005 William Pitcock <nenolod -at- nenolod.net>
 * Copyright (c) 2007 Jilles Tjoelker
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Prevent a bot from being assigned by non IRC operators.
 *
 */

#include "atheme.h"
#include "botserv.h"

DECLARE_MODULE_V1
(
	"botserv/set_private", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

mowgli_patricia_t **bs_set_cmdtree;

fn_botserv_bot_find *botserv_bot_find;
mowgli_list_t *bs_bots;

static void bs_cmd_set_private(sourceinfo_t *si, int parc, char *parv[]);

command_t bs_set_private = { "PRIVATE", N_("Prevent a bot from being assigned by non IRC operators."), PRIV_CHAN_ADMIN, 2, bs_cmd_set_private, { .path = "botserv/set_private" } };

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(bs_set_cmdtree, "botserv/set_core", "bs_set_cmdtree");

	MODULE_USE_SYMBOL(bs_bots, "botserv/main", "bs_bots");
	MODULE_USE_SYMBOL(botserv_bot_find, "botserv/main", "botserv_bot_find");

	command_add(&bs_set_private, *bs_set_cmdtree);
}

void _moddeinit(void)
{
	command_delete(&bs_set_private, *bs_set_cmdtree);
}

static void bs_cmd_set_private(sourceinfo_t *si, int parc, char *parv[])
{
	char *botserv = parv[0];
	char *option = parv[1];
	botserv_bot_t *bot;

	if (parc < 2 || !botserv || !option)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SET PRIVATE");
		command_fail(si, fault_needmoreparams, _("Syntax: SET <botnick> PRIVATE {ON|OFF}"));
		return;
	}

	bot = botserv_bot_find(botserv);
	if (!bot)
	{
		command_fail(si, fault_nosuch_target, _("\2%s\2 is not a bot."), botserv);
		return;
	}

	if (!si->smu)
	{
		command_fail(si, fault_noprivs, _("You are not logged in."));
		return;
	}

	if (!has_priv(si, PRIV_CHAN_ADMIN))
	{
		command_fail(si, fault_noprivs, _("You are not authorized to perform this operation."));
		return;
	}

	if (!irccasecmp(option, "ON"))
	{
		bot->private = true;
		logcommand(si, CMDLOG_SET, "SET:PRIVATE:ON: \2%s\2", bot->nick);
		command_success_nodata(si, _("Private mode of bot %s is now \2ON\2."), bot->nick);
	}
	else if(!irccasecmp(option, "OFF"))
	{
		bot->private = false;
		logcommand(si, CMDLOG_SET, "SET:PRIVATE:OFF: \2%s\2", bot->nick);
		command_success_nodata(si, _("Private mode of bot %s is now \2OFF\2."), bot->nick);
	}
	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "SET PRIVATE");
		command_fail(si, fault_badparams, _("Syntax: SET <botnick> PRIVATE {ON|OFF}"));
	}
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
