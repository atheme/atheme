/*
 * Copyright (c) 2005 Greg Feigenson
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Generates a new password, either n digits long (w/ nickserv arg), or 7 digits
 *
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"nickserv/generatepass", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Epiphanic Networks <http://www.epiphanic.org>"
);

static void ns_cmd_generatepass(sourceinfo_t *si, int parc, char *parv[]);

command_t ns_generatepass = { "GENERATEPASS", "Generates a random password.",
                        AC_NONE, 1, ns_cmd_generatepass, { .path = "contrib/generatepass" } };
                                                                                   
void _modinit(module_t *m)
{
	service_named_bind_command("nickserv", &ns_generatepass);
}

void _moddeinit()
{
	service_named_unbind_command("nickserv", &ns_generatepass);
}

static void ns_cmd_generatepass(sourceinfo_t *si, int parc, char *parv[])
{
	int n = 0;
	char *newpass;
   
	if (parc >= 1)
		n = atoi(parv[0]);

	if (n <= 0 || n > 127)
		n = 7;

	newpass = gen_pw(n);

	command_success_string(si, newpass, "Randomly generated password: %s", newpass);
	free(newpass);
	logcommand(si, CMDLOG_GET, "GENERATEPASS");
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
