/*
 * Copyright (c) 2005-2007 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Autokline channels.
 *
 * $Id$
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/klinechan", FALSE, _modinit, _moddeinit,
	"$Id$",
	"Jilles Tjoelker <http://www.stack.nl/~jilles/irc/>"
);

static void os_cmd_klinechan(sourceinfo_t *si, int parc, char *parv[]);

command_t os_klinechan = { "KLINECHAN", "Klines all users joining a channel.",
			PRIV_MASS_AKILL, 3, os_cmd_klinechan };

static void klinechan_check_join(void *vcu);
static void klinechan_show_info(void *vdata);

list_t *os_cmdtree;
list_t *os_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(os_cmdtree, "operserv/main", "os_cmdtree");
	MODULE_USE_SYMBOL(os_helptree, "operserv/main", "os_helptree");

	command_add(&os_klinechan, os_cmdtree);
	hook_add_event("channel_join");
	hook_add_hook_first("channel_join", klinechan_check_join);
	hook_add_event("channel_info");
	hook_add_hook("channel_info", klinechan_show_info);
	help_addentry(os_helptree, "KLINECHAN", "help/oservice/klinechan", NULL);
}

void _moddeinit()
{
	command_delete(&os_klinechan, os_cmdtree);
	hook_del_hook("channel_join", klinechan_check_join);
	hook_del_hook("channel_info", klinechan_show_info);
	help_delentry(os_helptree, "KLINECHAN");
}

static void klinechan_check_join(void *vdata)
{
	mychan_t *mc;
	chanuser_t *cu = ((hook_channel_joinpart_t *)vdata)->cu;
	char reason[256];

	if (cu == NULL || is_internal_client(cu->user))
		return;

	if (!(mc = mychan_find(cu->chan->name)))
		return;

	if (metadata_find(mc, METADATA_CHANNEL, "private:klinechan:closer"))
	{
		if (has_priv_user(cu->user, PRIV_JOIN_STAFFONLY))
			notice(opersvs.nick, cu->user->nick,
					"Warning: %s klines normal users",
					cu->chan->name);
		else
		{
			snprintf(reason, sizeof reason, "Joining %s",
					cu->chan->name);
			slog(LG_INFO, "klinechan_check_join(): klining *@%s (user %s!%s@%s joined %s)",
					cu->user->host, cu->user->nick,
					cu->user->user, cu->user->host,
					cu->chan->name);
			kline_sts("*", "*", cu->user->host, 86400, reason);
		}
	}
}

static void klinechan_show_info(void *vdata)
{
	hook_channel_req_t *hdata = vdata;
	metadata_t *md;
	const char *setter, *reason;
	time_t ts;
	struct tm tm;
	char strfbuf[32];

	if (!has_priv(hdata->si, PRIV_CHAN_AUSPEX))
		return;
	md = metadata_find(hdata->mc, METADATA_CHANNEL, "private:klinechan:closer");
	if (md == NULL)
		return;
	setter = md->value;
	md = metadata_find(hdata->mc, METADATA_CHANNEL, "private:klinechan:reason");
	reason = md != NULL ? md->value : "unknown";
	md = metadata_find(hdata->mc, METADATA_CHANNEL, "private:klinechan:timestamp");
	ts = md != NULL ? atoi(md->value) : 0;

	tm = *localtime(&ts);
	strftime(strfbuf, sizeof(strfbuf) - 1, "%b %d %H:%M:%S %Y", &tm);

	command_success_nodata(hdata->si, "%s had \2automatic klines\2 enabled on it by %s on %s (%s)", hdata->mc->name, setter, strfbuf, reason);
}

static void os_cmd_klinechan(sourceinfo_t *si, int parc, char *parv[])
{
	char *target = parv[0];
	char *action = parv[1];
	char *reason = parv[2];
	mychan_t *mc;
	channel_t *c;
	chanuser_t *cu;
	node_t *n;

	if (!target || !action)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "KLINECHAN");
		command_fail(si, fault_needmoreparams, "Usage: KLINECHAN <#channel> <ON|OFF> [reason]");
		return;
	}

	if (!(mc = mychan_find(target)))
	{
		command_fail(si, fault_nosuch_target, "\2%s\2 is not registered.", target);
		return;
	}

	if (!strcasecmp(action, "ON"))
	{
		if (!reason)
		{
			command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "KLINECHAN");
			command_fail(si, fault_needmoreparams, "Usage: KLINECHAN <#channel> ON <reason>");
			return;
		}

		if (config_options.chan && !irccasecmp(config_options.chan, target))
		{
			command_fail(si, fault_noprivs, "\2%s\2 cannot be closed.", target);
			return;
		}

		if (metadata_find(mc, METADATA_CHANNEL, "private:klinechan:closer"))
		{
			command_fail(si, fault_nochange, "\2%s\2 is already on autokline.", target);
			return;
		}

		metadata_add(mc, METADATA_CHANNEL, "private:klinechan:closer", si->su->nick);
		metadata_add(mc, METADATA_CHANNEL, "private:klinechan:reason", reason);
		metadata_add(mc, METADATA_CHANNEL, "private:klinechan:timestamp", itoa(CURRTIME));

		wallops("%s enabled automatic klines on the channel \2%s\2 (%s).", get_oper_name(si), target, reason);
		snoop("KLINECHAN:ON: \2%s\2 by \2%s\2 (%s)", target, get_oper_name(si), reason);
		logcommand(si, CMDLOG_ADMIN, "%s KLINECHAN ON %s", target, reason);
		command_success_nodata(si, "Klining all users joining \2%s\2.", target);
	}
	else if (!strcasecmp(action, "OFF"))
	{
		if (!metadata_find(mc, METADATA_CHANNEL, "private:klinechan:closer"))
		{
			command_fail(si, fault_nochange, "\2%s\2 is not closed.", target);
			return;
		}

		metadata_delete(mc, METADATA_CHANNEL, "private:klinechan:closer");
		metadata_delete(mc, METADATA_CHANNEL, "private:klinechan:reason");
		metadata_delete(mc, METADATA_CHANNEL, "private:klinechan:timestamp");

		wallops("%s disabled automatic klines on the channel \2%s\2.", get_oper_name(si), target);
		snoop("KLINECHAN:OFF: \2%s\2 by \2%s\2", target, get_oper_name(si));
		logcommand(si, CMDLOG_ADMIN, "%s KLINECHAN OFF", target);
		command_success_nodata(si, "No longer klining users joining \2%s\2.", target);
	}
	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "KLINECHAN");
		command_fail(si, fault_badparams, "Usage: KLINECHAN <#channel> <ON|OFF> [reason]");
	}
}
