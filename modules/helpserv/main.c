/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains the main() routine.
 *
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"helpserv/main", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

service_t *helpserv;
list_t helpserv_cmdtree;
list_t helpserv_helptree;
list_t helpserv_conftable;

static void helpserv_cmd_help(sourceinfo_t *si, const int parc, char *parv[]);

command_t helpserv_help = { "HELP", N_(N_("Displays contextual help information.")), AC_NONE, 2, helpserv_cmd_help };

/* HELP <command> [params] */
void helpserv_cmd_help(sourceinfo_t *si, int parc, char *parv[])
{
	char *command = parv[0];

	if (!command)
	{
		command_success_nodata(si, _("***** \2%s Help\2 *****"), si->service->nick);
		command_success_nodata(si, _("\2%s\2 allows users to request help from network staff."), si->service->nick);
		command_success_nodata(si, " ");
		command_success_nodata(si, _("For more information on a command, type:"));
		command_success_nodata(si, "\2/%s%s help <command>\2", (ircd->uses_rcommand == false) ? "msg " : "", helpserv->disp);
		command_success_nodata(si, " ");

		command_help(si, &helpserv_cmdtree);

		command_success_nodata(si, _("***** \2End of Help\2 *****"));
		return;
	}

	/* take the command through the hash table */
	help_display(si, si->service, command, &helpserv_helptree);
}

/* main services client routine */
static void helpserv_handler(sourceinfo_t *si, int parc, char *parv[])
{
	char *cmd;
        char *text;
	char orig[BUFSIZE];

	/* this should never happen */
	if (parv[0][0] == '&')
	{
		slog(LG_ERROR, "services(): got parv with local channel: %s", parv[0]);
		return;
	}

	/* make a copy of the original for debugging */
	strlcpy(orig, parv[parc - 1], BUFSIZE);

	/* lets go through this to get the command */
	cmd = strtok(parv[parc - 1], " ");
	text = strtok(NULL, "");

	if (!cmd)
		return;
	if (*cmd == '\001')
	{
		handle_ctcp_common(si, cmd, text);
		return;
	}

	/* take the command through the hash table */
	command_exec_split(si->service, si, cmd, text, &helpserv_cmdtree);
}

void _modinit(module_t *m)
{
	/* Gotta use long-form here because hs already is in use for HostServ */
	helpserv = service_add("helpserv", helpserv_handler, &helpserv_cmdtree, &helpserv_conftable);

	command_add(&helpserv_help, &helpserv_cmdtree);

	help_addentry(&helpserv_helptree, "HELP", "help/help", NULL);
}

void _moddeinit(void)
{
	if (helpserv)
	{
		service_delete(helpserv);
		helpserv = NULL;
	}
	
	command_delete(&helpserv_help, &helpserv_cmdtree);

	help_delentry(&helpserv_helptree, "HELP");
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
