/*
 * Copyright (c) 2008 William Pitcock <nenolod@atheme.org>
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Allows extension of expiry times if user will be away from IRC for
 * a month or two.
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"nickserv/vacation", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void ns_cmd_vacation(sourceinfo_t *si, int parc, char *parv[])
{
	char tsbuf[BUFSIZE];

	if (!si->smu)
	{
		command_fail(si, fault_noprivs, _("You are not logged in."));
		return;
	}

	if (CURRTIME < (time_t)(si->smu->registered + nicksvs.expiry))
	{
		command_fail(si, fault_noprivs, _("You must be registered for at least \2%d\2 days in order to enable VACATION mode."), 
			(nicksvs.expiry / 3600 / 24));
		return;
	}

	snprintf(tsbuf, BUFSIZE, "%lu", (unsigned long)CURRTIME);
	metadata_add(si->smu, "private:vacation", tsbuf);

	logcommand(si, CMDLOG_SET, "VACATION");

	command_success_nodata(si, _("Your account is now marked as being on vacation.\n"
				"Please be aware that this will be automatically removed the next time you identify to \2%s\2."),
				nicksvs.nick);
	if (nicksvs.expiry > 0)
		command_success_nodata(si, _("Your account will automatically expire in %d days if you do not return."),
				(nicksvs.expiry / 3600 / 24) * 3);
}

command_t ns_vacation = { "VACATION", N_("Sets an account as being on vacation."), AC_NONE, 1, ns_cmd_vacation, { .path = "nickserv/vacation" } };

static void user_identify_hook(user_t *u)
{
	if (!metadata_find(u->myuser, "private:vacation"))
		return;

	notice(nicksvs.nick, u->nick, _("Your account is no longer marked as being on vacation."));
	metadata_delete(u->myuser, "private:vacation");
}

static void user_expiry_hook(hook_expiry_req_t *req)
{
	myuser_t *mu = req->data.mu;

	if (!metadata_find(mu, "private:vacation"))
		return;

	if (mu->lastlogin >= CURRTIME || (unsigned int)(CURRTIME - mu->lastlogin) < nicksvs.expiry * 3)
		req->do_expire = 0;
}

static void nick_expiry_hook(hook_expiry_req_t *req)
{
	mynick_t *mn = req->data.mn;
	myuser_t *mu = mn->owner;

	if (!metadata_find(mu, "private:vacation"))
		return;

	if (mu->lastlogin >= CURRTIME || (unsigned int)(CURRTIME - mu->lastlogin) < nicksvs.expiry * 3)
		req->do_expire = 0;
}

static void info_hook(hook_user_req_t *hdata)
{
	if (metadata_find(hdata->mu, "private:vacation"))
		command_success_nodata(hdata->si, "%s is on vacation and has an extended expiry time", entity(hdata->mu)->name);
}

void _modinit(module_t *m)
{
	service_named_bind_command("nickserv", &ns_vacation);

	hook_add_event("user_identify");
	hook_add_user_identify(user_identify_hook);

	hook_add_event("user_check_expire");
	hook_add_user_check_expire(user_expiry_hook);

	hook_add_event("nick_check_expire");
	hook_add_nick_check_expire(nick_expiry_hook);

	hook_add_event("user_info");
	hook_add_user_info(info_hook);
}

void _moddeinit(void)
{
	service_named_unbind_command("nickserv", &ns_vacation);

	hook_del_user_identify(user_identify_hook);
	hook_del_user_check_expire(user_expiry_hook);
	hook_del_nick_check_expire(nick_expiry_hook);
	hook_del_user_info(info_hook);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
