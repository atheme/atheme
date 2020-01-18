/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2009 Celestin, et al.
 *
 * This file contains a BService INFO which can show
 * botserv settings on channel or bot.
 */

#include <atheme.h>

static struct botserv_main_symbols *bs_main_syms = NULL;

static void
bs_cmd_info(struct sourceinfo *si, int parc, char *parv[])
{
	char *dest = parv[0];
	struct mychan *mc = NULL;
	struct botserv_bot* bot = NULL;
	struct metadata *md;
	unsigned int comma = 0, i;
	char buf[BUFSIZE], strfbuf[BUFSIZE], *end;
	time_t registered;
	struct tm *tm;
	mowgli_node_t *n;
	struct chanuser *cu;

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
		bot = bs_main_syms->bot_find(dest);
	}

	if (bot != NULL)
	{
		command_success_nodata(si, _("Information for bot \2%s\2:"), bot->nick);
		command_success_nodata(si, _("     Mask : %s@%s"), bot->user, bot->host);
		command_success_nodata(si, _("Real name : %s"), bot->real);
		registered = bot->registered;
		tm = localtime(&registered);
		strftime(strfbuf, sizeof strfbuf, TIME_FORMAT, tm);
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
				cu = (struct chanuser *)n->data;
				command_success_nodata(si, _("%u: %s"), ++i, cu->chan->name);
			}
		}
	}
	else if (mc != NULL)
	{
		if (!(mc->flags & MC_PUBACL) && !chanacs_source_has_flag(mc, si, CA_ACLVIEW) && !has_priv(si, PRIV_CHAN_AUSPEX))
		{
			command_fail(si, fault_noprivs, STR_NOT_AUTHORIZED);
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
		if (metadata_find(mc, "private:botserv:saycaller"))
		{
			end += snprintf(end, sizeof(buf) - (end - buf), "%s%s", (comma) ? ", " : "", "Say Caller");
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

static struct command bs_info = {
	.name           = "INFO",
	.desc           = N_("Allows you to see BotServ information about a channel or a bot."),
	.access         = AC_NONE,
	.maxparc        = 1,
	.cmd            = &bs_cmd_info,
	.help           = { .path = "botserv/info" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_SYMBOL(m, bs_main_syms, "botserv/main", "botserv_main_symbols")

	service_named_bind_command("botserv", &bs_info);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("botserv", &bs_info);
}

SIMPLE_DECLARE_MODULE_V1("botserv/info", MODULE_UNLOAD_CAPABILITY_OK)
