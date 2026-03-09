/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2006 Robin Burchell <surreal.w00t@gmail.com>
 *
 * This file contains functionality implementing OperServ CLEARCHAN.
 *
 * Default AKILL time is 7 days (604800 seconds)
 */

#include <atheme.h>

#define CLEAR_KICK 1
#define CLEAR_KILL 2
#define CLEAR_AKILL 3

static void
os_cmd_clearchan(struct sourceinfo *si, int parc, char *parv[])
{
	struct chanuser *cu = NULL;
	mowgli_node_t *n, *tn;
	struct channel *c = NULL;
	int action;
	char *actionstr = parv[0];
	char *targchan = parv[1];
	char *treason = parv[2];
	char reason[512];
	unsigned int matches = 0;
	unsigned int ignores = 0;

	if (!actionstr || !targchan || !treason)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "CLEARCHAN");
		command_fail(si, fault_needmoreparams, _("Syntax: CLEARCHAN KICK|KILL|AKILL <#channel> <reason>"));
 		return;
	}

	c = channel_find(targchan);

	if (!c)
	{
                command_fail(si, fault_nosuch_target, _("Channel \2%s\2 does not exist."), targchan);
 		return;
	}

	// check what they are doing is valid
	if (!strcasecmp(actionstr, "KICK"))
		action = CLEAR_KICK;
	else if (!strcasecmp(actionstr, "KILL"))
		action = CLEAR_KILL;
	else if (!strcasecmp(actionstr, "AKILL") || !strcasecmp(actionstr, "GLINE") || !strcasecmp(actionstr, "KLINE"))
		action = CLEAR_AKILL;
	else
	{
		// not valid!
		command_fail(si, fault_badparams, _("\2%s\2 is not a valid action."), actionstr);
 		return;
	}

	if (action != CLEAR_KICK && !has_priv(si, PRIV_MASS_AKILL))
	{
		command_fail(si, fault_noprivs, STR_NO_PRIVILEGE, PRIV_MASS_AKILL);
		return;
	}

	if (action == CLEAR_KICK)
		snprintf(reason, sizeof reason, "(Clearing) %s", treason);
	else
		snprintf(reason, sizeof reason, "(Clearing %s) %s", targchan, treason);

	wallops("\2%s\2 is clearing channel %s with %s",
			get_oper_name(si), targchan, actionstr);
	command_success_nodata(si, _("Clearing \2%s\2 with \2%s\2"), targchan, actionstr);

	// iterate over the users in channel
	MOWGLI_ITER_FOREACH_SAFE(n, tn, c->members.head)
	{
		cu = n->data;

		if (is_internal_client(cu->user))
			;
		else if (is_ircop(cu->user))
		{
			command_success_nodata(si, _("\2CLEARCHAN\2: Ignoring IRC Operator \2%s\2!%s@%s {%s}"), cu->user->nick, cu->user->user, cu->user->host, cu->user->gecos);
			ignores++;
		}
		else
		{
			command_success_nodata(si, _("\2CLEARCHAN\2: \2%s\2 hit \2%s\2!%s@%s {%s}"), actionstr, cu->user->nick, cu->user->user, cu->user->host, cu->user->gecos);
			matches++;

			switch (action)
			{
				case CLEAR_KICK:
					kick(si->service->me, c, cu->user, reason);
					break;
				case CLEAR_KILL:
					kill_user(si->service->me, cu->user, "%s", reason);
					break;
				case CLEAR_AKILL:
					if (is_autokline_exempt(cu->user)) {
						command_success_nodata(si, _("\2CLEARCHAN\2: Not klining exempt user %s!%s@%s"),
								cu->user->nick, cu->user->user, cu->user->host);
					} else {
						if (! (cu->user->flags & UF_KLINESENT)) {
							kline_add(cu->user->user, cu->user->host, reason, config_options.kline_time, si->su->nick);
							cu->user->flags |= UF_KLINESENT;
						}
					}
			}
		}
	}

	command_success_nodata(si, _("\2%u\2 match(es), \2%u\2 ignore(s) for \2%s\2 on \2%s\2"), matches, ignores, actionstr, targchan);
	logcommand(si, CMDLOG_ADMIN, "CLEARCHAN: \2%s\2 \2%s\2 (reason: \2%s\2) (\2%u\2 matches, \2%u\2 ignores)", actionstr, targchan, treason, matches, ignores);
}

static struct command os_clearchan = {
	.name           = "CLEARCHAN",
	.desc           = N_("Clears a channel via KICK, KILL or AKILL"),
	.access         = PRIV_CHAN_ADMIN,
	.maxparc        = 3,
	.cmd            = &os_cmd_clearchan,
	.help           = { .path = "oservice/clearchan" }
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "operserv/main")

	service_named_bind_command("operserv", &os_clearchan);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("operserv", &os_clearchan);
}

SIMPLE_DECLARE_MODULE_V1("operserv/clearchan", MODULE_UNLOAD_CAPABILITY_OK)
