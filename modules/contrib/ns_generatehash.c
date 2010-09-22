/*
 * Copyright (c) 2010 Atheme development group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Generates a hash for use as a operserv "password".
 *
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"nickserv/generatehash", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme development group"
);

static void ns_cmd_generatehash(sourceinfo_t *si, int parc, char *parv[]);

command_t ns_generatehash = { "GENERATEHASH", "Generates a hash for SOPER.",
                        AC_NONE, 1, ns_cmd_generatehash, { .path = "contrib/generatehash" } };
                                                                                   
void _modinit(module_t *m)
{
	service_named_bind_command("nickserv", &ns_generatehash);
}

void _moddeinit()
{
	service_named_unbind_command("nickserv", &ns_generatehash);
}

static void ns_cmd_generatehash(sourceinfo_t *si, int parc, char *parv[])
{
	char *pass = parv[0];
	char hash[PASSLEN];

	if (parc < 1)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "GENERATEHASH");
		command_fail(si, fault_needmoreparams, _("Syntax: GENERATEHASH <password>")); 
		return;
	}

	if (crypto_module_loaded)
	{
		strlcpy(hash, crypt_string(pass, gen_salt()), PASSLEN);
		command_success_string(si, hash, "Hash is: %s", hash);
	}
	else
		command_success_nodata(si, "No crypto module loaded so could not hash anything.");

	logcommand(si, CMDLOG_GET, "GENERATEHASH");
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
