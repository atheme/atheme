/*
 * Copyright (c) 2005-2006 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Jupiters a server.
 *
 * $Id: jupe.c 8105 2007-04-05 16:02:05Z nenolod $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/jupe", FALSE, _modinit, _moddeinit,
	"$Id: jupe.c 8105 2007-04-05 16:02:05Z nenolod $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void os_cmd_jupe(sourceinfo_t *si, int parc, char *parv[]);

command_t os_jupe = { "JUPE", N_("Jupiters a server."), PRIV_JUPE, 2, os_cmd_jupe };

list_t *os_cmdtree;
list_t *os_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(os_cmdtree, "operserv/main", "os_cmdtree");
	MODULE_USE_SYMBOL(os_helptree, "operserv/main", "os_helptree");

	command_add(&os_jupe, os_cmdtree);
	help_addentry(os_helptree, "JUPE", "help/oservice/jupe", NULL);
}

void _moddeinit()
{
	command_delete(&os_jupe, os_cmdtree);
	help_delentry(os_helptree, "JUPE");
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

	if (!strchr(server, '.'))
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

	logcommand(si, CMDLOG_ADMIN, "JUPE %s %s", server, reason);

	snprintf(reasonbuf, BUFSIZE, "[%s] %s", get_oper_name(si), reason);
	jupe(server, reasonbuf);

	command_success_nodata(si, _("\2%s\2 has been jupitered."), server);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
