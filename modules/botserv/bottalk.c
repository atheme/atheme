/*
 * Copyright (c) 2009 Celestin, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains a BService SAY and ACT which can make
 * botserv bot talk in channel.
 *
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"botserv/bottalk", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Rizon Development Group <http://dev.rizon.net>"
);

static void bs_cmd_say(sourceinfo_t *si, int parc, char *parv[]);
static void bs_cmd_act(sourceinfo_t *si, int parc, char *parv[]);

command_t bs_say = { "SAY", N_("Makes the bot say the given text on the given channel."), AC_NONE, 2, bs_cmd_say, { .path = "botserv/say" } };
command_t bs_act = { "ACT", N_("Makes the bot do the equivalent of a \"/me\" command."), AC_NONE, 2, bs_cmd_act, { .path = "botserv/act" } };

void _modinit(module_t *m)
{
	service_named_bind_command("botserv", &bs_say);
	service_named_bind_command("botserv", &bs_act);
}

void _moddeinit()
{
	service_named_unbind_command("botserv", &bs_say);
	service_named_unbind_command("botserv", &bs_act);
}

static void bs_cmd_say(sourceinfo_t *si, int parc, char *parv[])
{
	char *channel = parv[0];
	char *message = parv[1];
	channel_t *c = channel_find(channel);
	mychan_t *mc = mychan_find(channel);
	metadata_t *bs;
	user_t *bot;

	if (!channel || !message)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SAY");
		command_fail(si, fault_needmoreparams, _("Syntax: SAY <#channel> <msg>"));
		return;
	}

	if (!mc)
	{
		command_fail(si, fault_nosuch_target, _("Channel \2%s\2 is not registered."), channel);
		return;
	}

	if (!c)
	{
		command_fail(si, fault_nosuch_target, _("\2%s\2 is currently empty."), channel);
		return;
	}

	if (!si->smu)
	{
		command_fail(si, fault_noprivs, _("You are not logged in."));
		return;
	}

	if (!(chanacs_source_flags(mc, si) & (CA_OP | CA_AUTOOP)))
	{
		command_fail(si, fault_noprivs, _("You are not authorized to perform this operation."));
		return;
	}

	if ((bs = metadata_find(mc, "private:botserv:bot-assigned")) != NULL)
		bot = user_find_named(bs->value);
	else
		bot = NULL;
	if (bot == NULL)
	{
		command_fail(si, fault_nosuch_key, _("\2%s\2 does not have a bot assigned."), mc->name);
		return;
	}

	msg(bot->nick, channel, "%s", message);
	logcommand(si, CMDLOG_DO, "SAY: \2%s\2", message);
}

static void bs_cmd_act(sourceinfo_t *si, int parc, char *parv[])
{
	char *channel = parv[0];
	char *message = parv[1];
	channel_t *c = channel_find(channel);
	mychan_t *mc = mychan_find(channel);
	metadata_t *bs;
	user_t *bot;

	if (!channel || !message)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "ACT");
		command_fail(si, fault_needmoreparams, _("Syntax: ACT <#channel> <msg>"));
		return;
	}

	if (!mc)
	{
		command_fail(si, fault_nosuch_target, _("Channel \2%s\2 is not registered."), channel);
		return;
	}

	if (!c)
	{
		command_fail(si, fault_nosuch_target, _("\2%s\2 is currently empty."), channel);
		return;
	}

	if (!si->smu)
	{
		command_fail(si, fault_noprivs, _("You are not logged in."));
		return;
	}

	if (!(chanacs_source_flags(mc, si) & (CA_OP | CA_AUTOOP)))
	{
		command_fail(si, fault_noprivs, _("You are not authorized to perform this operation."));
		return;
	}

	if ((bs = metadata_find(mc, "private:botserv:bot-assigned")) != NULL)
		bot = user_find_named(bs->value);
	else
		bot = NULL;
	if (bot == NULL)
	{
		command_fail(si, fault_nosuch_key, _("\2%s\2 does not have a bot assigned."), mc->name);
		return;
	}

	msg(bot->nick, channel, "\001ACTION %s\001", message);
	logcommand(si, CMDLOG_DO, "ACT: \2%s\2", message);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */


