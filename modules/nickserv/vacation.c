/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2008 William Pitcock <nenolod@dereferenced.org>
 *
 * Allows extension of expiry times if user will be away from IRC for
 * a month or two.
 */

#include <atheme.h>
#include "list_common.h"
#include "list.h"

static void
ns_cmd_vacation(struct sourceinfo *si, int parc, char *parv[])
{
	char tsbuf[BUFSIZE];

	if (CURRTIME < (time_t)(si->smu->registered + nicksvs.expiry))
	{
		command_fail(si, fault_noprivs, _("You must be registered for at least \2%u\2 days in order to enable VACATION mode."),
			(nicksvs.expiry / SECONDS_PER_DAY));
		return;
	}

	snprintf(tsbuf, BUFSIZE, "%lu", (unsigned long)CURRTIME);
	metadata_add(si->smu, "private:vacation", tsbuf);

	logcommand(si, CMDLOG_SET, "VACATION");

	command_success_nodata(si, _("Your account is now marked as being on vacation.\n"
	                             "Please be aware that this will be automatically\n"
	                             "removed the next time you identify to \2%s\2."),
	                             nicksvs.nick);

	if (nicksvs.expiry > 0)
		command_success_nodata(si, _("Your account will automatically expire in %u days if you do not return."),
		                             ((nicksvs.expiry * 3) / SECONDS_PER_DAY));
}

static void
user_identify_hook(struct user *u)
{
	if (!metadata_find(u->myuser, "private:vacation"))
		return;

	notice(nicksvs.nick, u->nick, "Your account is no longer marked as being on vacation.");
	metadata_delete(u->myuser, "private:vacation");
}

static void
user_expiry_hook(struct hook_expiry_req *req)
{
	struct myuser *mu = req->data.mu;

	if (!metadata_find(mu, "private:vacation"))
		return;

	if (mu->lastlogin >= CURRTIME || (unsigned int)(CURRTIME - mu->lastlogin) < nicksvs.expiry * 3)
		req->do_expire = 0;
}

static void
nick_expiry_hook(struct hook_expiry_req *req)
{
	struct mynick *mn = req->data.mn;
	struct myuser *mu = mn->owner;

	if (!metadata_find(mu, "private:vacation"))
		return;

	if (mu->lastlogin >= CURRTIME || (unsigned int)(CURRTIME - mu->lastlogin) < nicksvs.expiry * 3)
		req->do_expire = 0;
}

static void
info_hook(struct hook_user_req *hdata)
{
	if (metadata_find(hdata->mu, "private:vacation"))
		command_success_nodata(hdata->si, _("%s is on vacation and has an extended expiry time"),
		                                    entity(hdata->mu)->name);
}

static bool
is_vacation(const struct mynick *mn, const void *arg)
{
	struct myuser *mu = mn->owner;

	return ( metadata_find(mu, "private:vacation") != NULL );
}

static struct command ns_vacation = {
	.name           = "VACATION",
	.desc           = N_("Sets an account as being on vacation."),
	.access         = AC_AUTHENTICATED,
	.maxparc        = 1,
	.cmd            = &ns_cmd_vacation,
	.help           = { .path = "nickserv/vacation" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "nickserv/main")

	use_nslist_main_symbols(m);

	service_named_bind_command("nickserv", &ns_vacation);

	hook_add_user_identify(user_identify_hook);
	hook_add_user_check_expire(user_expiry_hook);
	hook_add_nick_check_expire(nick_expiry_hook);
	hook_add_user_info(info_hook);

	static struct list_param vacation;
	vacation.opttype = OPT_BOOL;
	vacation.is_match = is_vacation;

	list_register("vacation", &vacation);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("nickserv", &ns_vacation);

	hook_del_user_identify(user_identify_hook);
	hook_del_user_check_expire(user_expiry_hook);
	hook_del_nick_check_expire(nick_expiry_hook);
	hook_del_user_info(info_hook);

	list_unregister("vacation");
}

SIMPLE_DECLARE_MODULE_V1("nickserv/vacation", MODULE_UNLOAD_CAPABILITY_OK)
