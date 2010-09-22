/*
 * Copyright (c) 2005-2006 William Pitcock <nenolod@nenolod.net> et al
 * Rights to this code are documented in doc/LICENSE.
 *
 * Rock Paper Scissors
 *
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"gameserv/rps", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void command_rps(sourceinfo_t *si, int parc, char *parv[]);

command_t cmd_rps = { "RPS", N_("Rock Paper Scissors."), AC_NONE, 0, command_rps, { .path = "" } };

void _modinit(module_t * m)
{
	service_named_bind_command("gameserv", &cmd_rps);

	service_named_bind_command("chanserv", &cmd_rps);
}

void _moddeinit()
{
	service_named_unbind_command("gameserv", &cmd_rps);

	service_named_unbind_command("chanserv", &cmd_rps);
}

/*
 * Handle reporting for both fantasy commands and normal commands in GameServ
 * quickly and easily. Of course, sourceinfo has a vtable that can be manipulated,
 * but this is quicker and easier...                                  -- nenolod
 */
static void gs_command_report(sourceinfo_t *si, const char *fmt, ...)
{
	va_list args;
	char buf[BUFSIZE];

	va_start(args, fmt);
	vsnprintf(buf, BUFSIZE, fmt, args);
	va_end(args);

	if (si->c != NULL)
		msg(chansvs.nick, si->c->name, "%s", buf);
	else
		command_success_nodata(si, "%s", buf);
}

static void command_rps(sourceinfo_t *si, int parc, char *parv[])
{
	static const char *rps_responses[3] = {
		N_("Rock"),
		N_("Paper"),
		N_("Scissors")
	};

	gs_command_report(si, "%s", rps_responses[rand() % 3]);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
