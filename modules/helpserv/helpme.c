/*
 * Copyright (c) 2005-2006 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains functionality which implements the HelpServ HELPME command.
 *
 */

#include "atheme.h"
#include "uplink.h"

DECLARE_MODULE_V1
(
	"helpserv/helpme", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

unsigned int ratelimit_count = 0;
time_t ratelimit_firsttime = 0;

static void helpserv_cmd_helpme(sourceinfo_t *si, int parc, char *parv[]);

command_t helpserv_helpme = { "HELPME", N_("Request help from network staff."), AC_NONE, 1, helpserv_cmd_helpme, { .path = "helpserv/helpme" } };

void _modinit(module_t *m)
{
        service_named_bind_command("helpserv", &helpserv_helpme);
}

void _moddeinit()
{
        service_named_unbind_command("helpserv", &helpserv_helpme);
}

static void helpserv_cmd_helpme(sourceinfo_t *si, int parc, char *parv[])
{
        char *topic = parv[0];

	if ((unsigned int)(CURRTIME - ratelimit_firsttime) > config_options.ratelimit_period)
		ratelimit_count = 0, ratelimit_firsttime = CURRTIME;

	if (ratelimit_count > config_options.ratelimit_uses && !has_priv(si, PRIV_FLOOD))
        {
                command_fail(si, fault_toomany, _("The system is currently too busy to process your help request, please try again later."));
                slog(LG_INFO, "HELPME:THROTTLED: %s", si->su->nick);
                return;
        }

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

	if (config_options.ratelimit_uses && config_options.ratelimit_period)
		ratelimit_count++;

        return;
}
/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
