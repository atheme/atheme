/*
 * Copyright (c) 2005-2007 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Autokline channels.
 *
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/klinechan", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Jilles Tjoelker <http://www.stack.nl/~jilles/irc/>"
);

static void os_cmd_klinechan(sourceinfo_t *si, int parc, char *parv[]);
static void os_cmd_listklinechans(sourceinfo_t *si, int parc, char *parv[]);

command_t os_klinechan = { "KLINECHAN", "Klines all users joining a channel.",
			PRIV_MASS_AKILL, 3, os_cmd_klinechan, { .path = "contrib/klinechan" } };
command_t os_listklinechans = { "LISTKLINECHAN", "Lists active K:line channels.", PRIV_MASS_AKILL, 1, os_cmd_listklinechans, { .path = "contrib/listklinechans" } };

static void klinechan_check_join(hook_channel_joinpart_t *hdata);
static void klinechan_show_info(hook_channel_req_t *hdata);

void _modinit(module_t *m)
{
	service_named_bind_command("operserv", &os_klinechan);
	service_named_bind_command("operserv", &os_listklinechans);
	hook_add_event("channel_join");
	hook_add_first_channel_join(klinechan_check_join);
	hook_add_event("channel_info");
	hook_add_channel_info(klinechan_show_info);
}

void _moddeinit()
{
	service_named_unbind_command("operserv", &os_klinechan);
	service_named_unbind_command("operserv", &os_listklinechans);
	hook_del_channel_join(klinechan_check_join);
	hook_del_channel_info(klinechan_show_info);
}

static void klinechan_check_join(hook_channel_joinpart_t *hdata)
{
	mychan_t *mc;
	chanuser_t *cu = hdata->cu;
	service_t *svs;
	char reason[256];

	svs = service_find("operserv");
	if (svs == NULL)
		return;

	if (cu == NULL || is_internal_client(cu->user))
		return;

	if (!(mc = mychan_find(cu->chan->name)))
		return;

	if (metadata_find(mc, "private:klinechan:closer"))
	{
		if (has_priv_user(cu->user, PRIV_JOIN_STAFFONLY))
			notice(svs->me->nick, cu->user->nick,
					"Warning: %s klines normal users",
					cu->chan->name);
		else if (is_autokline_exempt(cu->user))
		{
			char buf[BUFSIZE];
			snprintf(buf, sizeof(buf), "Not klining *@%s due to klinechan %s (user %s!%s@%s is exempt)",
					cu->user->host, cu->chan->name,
					cu->user->nick, cu->user->user, cu->user->host);
			wallops_sts(buf);
		}
		else
		{
			snprintf(reason, sizeof reason, "Joining %s",
					cu->chan->name);
			slog(LG_INFO, "klinechan_check_join(): klining \2*@%s\2 (user \2%s!%s@%s\2 joined \2%s\2)",
					cu->user->host, cu->user->nick,
					cu->user->user, cu->user->host,
					cu->chan->name);
			kline_sts("*", "*", cu->user->host, 86400, reason);
		}
	}
}

static void klinechan_show_info(hook_channel_req_t *hdata)
{
	metadata_t *md;
	const char *setter, *reason;
	time_t ts;
	struct tm tm;
	char strfbuf[32];

	if (!has_priv(hdata->si, PRIV_CHAN_AUSPEX))
		return;
	md = metadata_find(hdata->mc, "private:klinechan:closer");
	if (md == NULL)
		return;
	setter = md->value;
	md = metadata_find(hdata->mc, "private:klinechan:reason");
	reason = md != NULL ? md->value : "unknown";
	md = metadata_find(hdata->mc, "private:klinechan:timestamp");
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

		if (metadata_find(mc, "private:klinechan:closer"))
		{
			command_fail(si, fault_nochange, "\2%s\2 is already on autokline.", target);
			return;
		}

		metadata_add(mc, "private:klinechan:closer", si->su->nick);
		metadata_add(mc, "private:klinechan:reason", reason);
		metadata_add(mc, "private:klinechan:timestamp", number_to_string(CURRTIME));

		wallops("%s enabled automatic klines on the channel \2%s\2 (%s).", get_oper_name(si), target, reason);
		logcommand(si, CMDLOG_ADMIN, "KLINECHAN:ON: \2%s\2 (reason: \2%s\2)", target, reason);
		command_success_nodata(si, "Klining all users joining \2%s\2.", target);
	}
	else if (!strcasecmp(action, "OFF"))
	{
		if (!metadata_find(mc, "private:klinechan:closer"))
		{
			command_fail(si, fault_nochange, "\2%s\2 is not closed.", target);
			return;
		}

		metadata_delete(mc, "private:klinechan:closer");
		metadata_delete(mc, "private:klinechan:reason");
		metadata_delete(mc, "private:klinechan:timestamp");

		wallops("%s disabled automatic klines on the channel \2%s\2.", get_oper_name(si), target);
		logcommand(si, CMDLOG_ADMIN, "KLINECHAN:OFF: \2%s\2", target);
		command_success_nodata(si, "No longer klining users joining \2%s\2.", target);
	}
	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "KLINECHAN");
		command_fail(si, fault_badparams, "Usage: KLINECHAN <#channel> <ON|OFF> [reason]");
	}
}

static void os_cmd_listklinechans(sourceinfo_t *si, int parc, char *parv[])
{
	const char *pattern;
	mowgli_patricia_iteration_state_t state;
	mychan_t *mc;
	metadata_t *md;
	int matches = 0;

	pattern = parc >= 1 ? parv[0] : "*";

	MOWGLI_PATRICIA_FOREACH(mc, &state, mclist)
	{
		md = metadata_find(mc, "private:klinechan:closer");
		if (md == NULL)
			continue;
		if (!match(pattern, mc->name))
		{
			command_success_nodata(si, "- %-30s", mc->name);
			matches++;
		}
	}

	logcommand(si, CMDLOG_ADMIN, "LISTKLINECHANS: \2%s\2 (\2%d\2 matches)", pattern, matches);
	if (matches == 0)
		command_success_nodata(si, _("No K:line channels matched pattern \2%s\2"), pattern);
	else
		command_success_nodata(si, ngettext(N_("\2%d\2 match for pattern \2%s\2"),
						    N_("\2%d\2 matches for pattern \2%s\2"), matches), matches, pattern);
}
