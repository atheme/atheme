/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2010 Atheme Project (http://atheme.org/)
 *
 * This file contains functionality which implements the HelpServ HELPME command.
 */

#include <atheme.h>

static unsigned int ratelimit_count = 0;
static time_t ratelimit_firsttime = 0;

static void
helpserv_cmd_helpme(struct sourceinfo *si, int parc, char *parv[])
{
        char *topic = parv[0];

	if ((unsigned int)(CURRTIME - ratelimit_firsttime) > config_options.ratelimit_period)
	{
		ratelimit_count = 0;
		ratelimit_firsttime = CURRTIME;
	}

	if (ratelimit_count > config_options.ratelimit_uses && !has_priv(si, PRIV_FLOOD))
        {
                command_fail(si, fault_toomany, _("The system is currently too busy to process your help request, please try again later."));
                slog(LG_INFO, "HELPME:THROTTLED: %s", si->su->nick);
                return;
        }

	if (si->smu != NULL && metadata_find(si->smu, "private:restrict:setter"))
	{
		command_fail(si, fault_noprivs, _("You have been restricted from requesting help by network staff."));
		return;
	}

	command_add_flood(si, FLOOD_HEAVY);

        if (topic)
        {
                logcommand(si, CMDLOG_ADMIN, "HELPME: \2%s\2", topic);
                wallops("\2%s\2 has requested help about \2%s\2", get_source_name(si), topic);
        }
        else
        {
                logcommand(si, CMDLOG_ADMIN, "HELPME");
                wallops("\2%s\2 has requested help.", get_source_name(si));
        }

	command_success_nodata(si, _("The network staff has been notified that you need help and will be with you shortly."));

	if (config_options.ratelimit_uses && config_options.ratelimit_period)
		ratelimit_count++;

        return;
}

static struct command helpserv_helpme = {
	.name           = "HELPME",
	.desc           = N_("Request help from network staff."),
	.access         = AC_NONE,
	.maxparc        = 1,
	.cmd            = &helpserv_cmd_helpme,
	.help           = { .path = "helpserv/helpme" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "helpserv/main")

        service_named_bind_command("helpserv", &helpserv_helpme);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
        service_named_unbind_command("helpserv", &helpserv_helpme);
}

SIMPLE_DECLARE_MODULE_V1("helpserv/helpme", MODULE_UNLOAD_CAPABILITY_OK)
