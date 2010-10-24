/*
 * Copyright (c) 2009 Celestin, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains a BService INFO which can show
 * botserv settings on channel or bot.
 *
 */

#include "atheme.h"
#include "botserv.h"

DECLARE_MODULE_V1
(
	"botserv/info", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://atheme.org/>"
);

static void bs_cmd_info(sourceinfo_t *si, int parc, char *parv[]);

command_t bs_info = { "INFO", N_("Allows you to see BotServ information about a channel or a bot."), AC_NONE, 1, bs_cmd_info, { .path = "botserv/info" } };

fn_botserv_bot_find *botserv_bot_find;
mowgli_list_t *bs_bots;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(bs_bots, "botserv/main", "bs_bots");
	MODULE_USE_SYMBOL(botserv_bot_find, "botserv/main", "botserv_bot_find");

	service_named_bind_command("botserv", &bs_info);
}

void _moddeinit()
{
	service_named_unbind_command("botserv", &bs_info);
}

/* ******************************************************************** */

static void bs_cmd_info(sourceinfo_t *si, int parc, char *parv[])
{
	char *dest = parv[0];
	mychan_t *mc = NULL;
	botserv_bot_t* bot = NULL;
	metadata_t *md;
	int comma = 0, i;
	char buf[BUFSIZE], strfbuf[32], *end;
	time_t registered;
	struct tm tm;
	mowgli_node_t *n;
	chanuser_t *cu;

	if (parc < 1 || parv[0] == NULL)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "INFO");
		command_fail(si, fault_needmoreparams, _("Syntax: INFO <#channel>"));
		command_fail(si, fault_needmoreparams, _("Syntax: INFO <botnick>"));
		return;
	}

	if (parv[0][0] == '#')
	{
		mc = mychan_find(dest);
	}
	else
	{
		bot = botserv_bot_find(dest);
	}

	if (bot != NULL)
	{
		command_success_nodata(si, _("Information for bot \2%s\2:"), bot->nick);
		command_success_nodata(si, _("     Mask : %s@%s"), bot->user, bot->host);
		command_success_nodata(si, _("Real name : %s"), bot->real);
		registered = bot->registered;
		tm = *localtime(&registered);
		strftime(strfbuf, sizeof(strfbuf) - 1, "%b %d %H:%M:%S %Y", &tm);
		command_success_nodata(si, _("  Created : %s (%s ago)"), strfbuf, time_ago(registered));
		if (bot->private)
			command_success_nodata(si, _("  Options : Private"));
		else
			command_success_nodata(si, _("  Options : None"));
		command_success_nodata(si, _("  Used on : %zu channel(s)"), MOWGLI_LIST_LENGTH(&bot->me->me->channels));
		if (has_priv(si, PRIV_CHAN_AUSPEX))
		{
			i = 0;
			MOWGLI_ITER_FOREACH(n, bot->me->me->channels.head)
			{
				cu = (chanuser_t *)n->data;
				command_success_nodata(si, _("%d: %s"), ++i, cu->chan->name);
			}
		}
	}
	else if (mc != NULL)
	{
		if (!chanacs_source_has_flag(mc, si, CA_ACLVIEW) && !has_priv(si, PRIV_CHAN_AUSPEX))
		{
			command_fail(si, fault_noprivs, _("You are not authorized to perform this operation."));
			return;
		}

		command_success_nodata(si, _("Information for channel \2%s\2:"), mc->name);
		if ((md = metadata_find(mc, "private:botserv:bot-assigned")) != NULL)
			command_success_nodata(si, _("         Bot nick : %s"), md->value);
		else
			command_success_nodata(si, _("         Bot nick : not assigned yet"));
		end = buf;
		*end = '\0';
		if (metadata_find(mc, "private:botserv:bot-handle-fantasy"))
		{
			end += snprintf(end, sizeof(buf) - (end - buf), "%s%s", (comma) ? ", " : "", "Fantasy");
			comma = 1;
		}
		if (metadata_find(mc, "private:botserv:no-bot"))
		{
			end += snprintf(end, sizeof(buf) - (end - buf), "%s%s", (comma) ? ", " : "", "No bot");
			comma = 1;
		}
		command_success_nodata(si, _("          Options : %s"), (*buf) ? buf : "None");
	}
	else
	{
		command_fail(si, fault_nosuch_target, STR_INSUFFICIENT_PARAMS, "INFO");
		command_fail(si, fault_nosuch_target, _("Syntax: INFO <#channel>"));
		command_fail(si, fault_nosuch_target, _("Syntax: INFO <botnick>"));
	}
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */


