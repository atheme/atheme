/*
 * Copyright (c) 2005-2007 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the NickServ FREGISTER function.
 *
 * Remember to give the user:fregister priv to any soper you want
 * to be able to use this command.
 *
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"nickserv/fregister", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void ns_cmd_fregister(sourceinfo_t *si, int parc, char *parv[]);

command_t ns_fregister = { "FREGISTER", "Registers a nickname on behalf of another user.", PRIV_USER_FREGISTER, 20, ns_cmd_fregister, { .path = "contrib/fregister" } };

void _modinit(module_t *m)
{
	service_named_bind_command("nickserv", &ns_fregister);
}

void _moddeinit()
{
	service_named_unbind_command("nickserv", &ns_fregister);
}

static void ns_cmd_fregister(sourceinfo_t *si, int parc, char *parv[])
{
	myuser_t *mu;
	mynick_t *mn = NULL;
	char *account;
	char *pass;
	char *email;
	int i, uflags = 0;
	hook_user_req_t req;

	account = parv[0], pass = parv[1], email = parv[2];

	if (!account || !pass || !email)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "FREGISTER");
		command_fail(si, fault_needmoreparams, "Syntax: FREGISTER <account> <password> <email> [CRYPTPASS]");
		return;
	}

	for (i = 3; i < parc; i++)
	{
		if (!strcasecmp(parv[i], "CRYPTPASS"))
			uflags |= MU_CRYPTPASS;
		else if (!strcasecmp(parv[i], "HIDEMAIL"))
			uflags |= MU_HIDEMAIL;
		else if (!strcasecmp(parv[i], "NOOP"))
			uflags |= MU_NOOP;
		else if (!strcasecmp(parv[i], "NEVEROP"))
			uflags |= MU_NEVEROP;
	}

	if (!(uflags & MU_CRYPTPASS) && strlen(pass) > 32)
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "FREGISTER");
		return;
	}

	if (!nicksvs.no_nick_ownership && IsDigit(*account))
	{
		command_fail(si, fault_badparams, "For security reasons, you can't register your UID.");
		return;
	}

	if (strchr(account, ' ') || strchr(account, '\n') || strchr(account, '\r') || account[0] == '=' || account[0] == '#' || account[0] == '@' || account[0] == '+' || account[0] == '%' || account[0] == '!' || strchr(account, ','))
	{
		command_fail(si, fault_badparams, "The account name \2%s\2 is invalid.", account);
		return;
	}

	if (!validemail(email))
	{
		command_fail(si, fault_badparams, "\2%s\2 is not a valid email address.", email);
		return;
	}

	/* make sure it isn't registered already */
	if (nicksvs.no_nick_ownership ? myuser_find(account) != NULL : mynick_find(account) != NULL)
	{
		command_fail(si, fault_alreadyexists, "\2%s\2 is already registered.", account);
		return;
	}

	mu = myuser_add(account, pass, email, uflags | config_options.defuflags | MU_NOBURSTLOGIN);
	mu->registered = CURRTIME;
	mu->lastlogin = CURRTIME;
	if (!nicksvs.no_nick_ownership)
	{
		mn = mynick_add(mu, entity(mu)->name);
		mn->registered = CURRTIME;
		mn->lastseen = CURRTIME;
	}

	logcommand(si, CMDLOG_REGISTER, "FREGISTER: \2%s\2 to \2%s\2", account, email);
	if (is_soper(mu))
	{
		wallops("%s used FREGISTER on account \2%s\2 with services operator privileges.", get_oper_name(si), entity(mu)->name);
		slog(LG_INFO, "SOPER: \2%s\2", entity(mu)->name);
	}

	command_success_nodata(si, "\2%s\2 is now registered to \2%s\2.", entity(mu)->name, mu->email);
	hook_call_user_register(mu);
	req.si = si;
	req.mu = mu;
	req.mn = mn;
	hook_call_user_verify_register(&req);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
