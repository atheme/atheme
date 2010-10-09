/*
 * Copyright (c) 2005 William Pitcock, et al.
 * The rights to this code are as documented under doc/LICENSE.
 *
 * Meow!
 *
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"contrib/ircd_catserv", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

service_t *catserv;
mowgli_list_t catserv_conftable;

static void catserv_cmd_meow(sourceinfo_t *si, int parc, char *parv[]);
static void catserv_cmd_help(sourceinfo_t *si, int parc, char *parv[]);

command_t catserv_meow = { "MEOW", "Makes the cute little kitty-cat meow!",
				AC_NONE, 0, catserv_cmd_meow, { .path = "" } };
command_t catserv_help = { "HELP", "Displays contextual help information.",
				AC_NONE, 1, catserv_cmd_help, { .path = "help" } };

void _modinit(module_t *m)
{
	catserv = service_add("catserv", NULL, &catserv_conftable);

	service_bind_command(catserv, &catserv_meow);
	service_bind_command(catserv, &catserv_help);
}

void _moddeinit()
{
	service_unbind_command(catserv, &catserv_meow);
	service_unbind_command(catserv, &catserv_help);

	service_delete(catserv);
}

static void catserv_cmd_meow(sourceinfo_t *si, int parc, char *parv[])
{
	command_success_nodata(si, "Meow!");
}

static void catserv_cmd_help(sourceinfo_t *si, int parc, char *parv[])
{
	command_help(si, si->service->commands);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
