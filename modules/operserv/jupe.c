/*
 * Copyright (c) 2005-2006 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Jupiters a server.
 *
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/jupe", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void os_cmd_jupe(sourceinfo_t *si, int parc, char *parv[]);

command_t os_jupe = { "JUPE", N_("Jupiters a server."), PRIV_JUPE, 2, os_cmd_jupe, { .path = "oservice/jupe" } };

void _modinit(module_t *m)
{
	service_named_bind_command("operserv", &os_jupe);
}

void _moddeinit()
{
	service_named_unbind_command("operserv", &os_jupe);
}

static void os_cmd_jupe(sourceinfo_t *si, int parc, char *parv[])
{
	char *server = parv[0];
	char *reason = parv[1];
	char reasonbuf[BUFSIZE];

	if (!server || !reason)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "JUPE");
		command_fail(si, fault_needmoreparams, _("Usage: JUPE <server> <reason>"));
		return;
	}

	/* Disallow * in a jupe as a minimal sanity check; it makes it hard to
	 * squit safely. */
	if (!strchr(server, '.') || strchr(server, '*') || strchr(server, '?'))
	{
		command_fail(si, fault_badparams, _("\2%s\2 is not a valid server name."), server);
		return;
	}

	if (!irccasecmp(server, me.name))
	{
		command_fail(si, fault_noprivs, _("\2%s\2 is the services server; it cannot be jupitered!"), server);
		return;
	}

	if (!irccasecmp(server, me.actual))
	{
		command_fail(si, fault_noprivs, _("\2%s\2 is the current uplink; it cannot be jupitered!"), server);
		return;
	}

	logcommand(si, CMDLOG_ADMIN, "JUPE: \2%s\2 (reason: \2%s\2)", server, reason);
	wallops(_("%s jupitered server \2%s\2 (%s)."), get_oper_name(si),
			server, reason);

	snprintf(reasonbuf, BUFSIZE, "[%s] %s", get_oper_name(si), reason);
	jupe(server, reasonbuf);

	command_success_nodata(si, _("\2%s\2 has been jupitered."), server);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
