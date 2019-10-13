/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2010 Atheme Project (http://atheme.org/), et al.
 * Copyright (C) 2017 ChatLounge IRC Network Development Team
 *
 * This file contains code for OS INFO
 */

#include <atheme.h>

static void
os_cmd_info(struct sourceinfo *si, int parc, char *parv[])
{
	mowgli_node_t *tn, *n2;

	logcommand(si, CMDLOG_GET, "INFO");

	command_success_nodata(si, _("How often services writes changes to the database: %u minutes"), config_options.commit_interval / SECONDS_PER_MINUTE);
	command_success_nodata(si, _("Default kline time: %u days"), config_options.kline_time / SECONDS_PER_DAY);
	command_success_nodata(si, _("Will services be sending WALLOPS/GLOBOPS about various things: %s"), config_options.silent ? _("No") : _("Yes"));
	command_success_nodata(si, _("How many messages before a flood is triggered, (if 0, flood protection is disabled): %u"), config_options.flood_msgs);
	command_success_nodata(si, _("How long before the flood counter resets: %u seconds"), config_options.flood_time);
	command_success_nodata(si, _("Default maximum number of clones allowed: %u"), config_options.default_clone_allowed);
	command_success_nodata(si, _("Number of commands used before ratelimiting starts, (if 0, ratelimiting is disabled): %u"), config_options.ratelimit_uses);
	command_success_nodata(si, _("How long before ratelimiting counter resets, (if 0, ratelimiting is disabled): %u seconds"), config_options.ratelimit_period);
	command_success_nodata(si, _("No nick ownership enabled: %s"), nicksvs.no_nick_ownership ? _("Yes") : _("No"));
	command_success_nodata(si, _("Nickname expiration time: %u days"), nicksvs.expiry / SECONDS_PER_DAY);
	command_success_nodata(si, _("Nickname enforce expiry time: %u days"), nicksvs.enforce_expiry / SECONDS_PER_DAY);
	command_success_nodata(si, _("Default nickname enforce delay: %u seconds"), nicksvs.enforce_delay);
	command_success_nodata(si, _("Nickname enforce prefix: %s"), nicksvs.enforce_prefix);
	command_success_nodata(si, _("Maximum number of logins allowed per username: %u"), me.maxlogins);
	command_success_nodata(si, _("Maximum number of usernames that can be registered to one email address: %u"), me.maxusers);
	if (!nicksvs.no_nick_ownership)
		command_success_nodata(si, _("Maximum number of nicknames that one user can own: %u"), nicksvs.maxnicks);
	command_success_nodata(si, _("Maximum number of channels that one user can own: %u"), chansvs.maxchans);
	command_success_nodata(si, _("Channel expiration time: %u days"), chansvs.expiry / SECONDS_PER_DAY);
	if (chansvs.fantasy)
		command_success_nodata(si, _("Default channel fantasy trigger: %s"), chansvs.trigger);
	command_success_nodata(si, _("Maximum number of entries allowed in a channel access list (if 0, unlimited): %u"), chansvs.maxchanacs);
	command_success_nodata(si, _("Maximum number of founders allowed per channel: %u"), chansvs.maxfounders);
	command_success_nodata(si, _("Show entity IDs to everyone: %s"),
		config_options.show_entity_id ? _("Yes") : _("No"));

	if (IS_TAINTED)
	{
		mowgli_node_t *n;

		command_success_nodata(si, _("Services is presently \2TAINTED\2, no support will be given for this configuration."));
		command_success_nodata(si, _("List of active taints:"));

		MOWGLI_ITER_FOREACH(n, taint_list.head)
		{
			struct taint_reason *tr = n->data;

			command_success_nodata(si, _("Taint Condition: %s"), tr->condition);
			command_success_nodata(si, _("Taint Location: %s:%u"), tr->file, tr->line);
			command_success_nodata(si, _("Taint Explanation: %s"), tr->buf);
		}
	}

	MOWGLI_ITER_FOREACH(tn, nicksvs.emailexempts.head)
	{
		command_success_nodata(si, _("Email address(es) exempt from the maximum usernames check: %s"), (char *)tn->data);
	}

	MOWGLI_ITER_FOREACH(n2, config_options.exempts.head)
	{
		command_success_nodata(si, _("user@host mask(s) that are autokline exempt: %s"), (char *)n2->data);
	}

	hook_call_operserv_info(si);
}

static struct command os_info = {
	.name           = "INFO",
	.desc           = N_("Shows some useful information about the current settings of services."),
	.access         = PRIV_SERVER_AUSPEX,
	.maxparc        = 1,
	.cmd            = &os_cmd_info,
	.help           = { .path = "oservice/info" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "operserv/main")

        service_named_bind_command("operserv", &os_info);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("operserv", &os_info);
}

SIMPLE_DECLARE_MODULE_V1("operserv/info", MODULE_UNLOAD_CAPABILITY_OK)
