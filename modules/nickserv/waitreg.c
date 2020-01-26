/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2010 William Pitcock <nenolod@dereferenced.org>
 * Copyright (C) 2018 Atheme Development Group (https://atheme.github.io/)
 *
 * This file contains code for delaying user registration.
 */

#include <atheme.h>

#define WAITREG_TIME_MIN        0U
#define WAITREG_TIME_DEF        0U
#define WAITREG_TIME_MAX        43200U /* 12 hours */

static unsigned int waitreg_time = WAITREG_TIME_DEF;

static void
waitreg_infohook(struct sourceinfo *const restrict si)
{
	return_if_fail(si != NULL);

	(void) command_success_nodata(si, _("Time (in seconds) before users may register an account: %u"), waitreg_time);
}

static void
waitreg_registerhook(struct hook_user_register_check *const restrict hdata)
{
	if (! waitreg_time)
		return;

	return_if_fail(hdata != NULL);
	return_if_fail(hdata->si != NULL);
	return_if_fail(hdata->password != NULL);

	if (! (hdata->si->su && hdata->si->su->ts))
		return;

	return_if_fail(hdata->si->su->ts <= CURRTIME);

	const unsigned int delta_time = (unsigned int) (CURRTIME - hdata->si->su->ts);

	if (delta_time < waitreg_time)
	{
		(void) logcommand(hdata->si, CMDLOG_REGISTER, "denied REGISTER by \2%s\2 (too soon)",
		                  hdata->si->su->nick);

		(void) command_fail(hdata->si, fault_badparams, _("You cannot register so soon after connecting. "
		                    "Please wait %u seconds and try again."), (waitreg_time - delta_time));

		hdata->approved++;
	}
}

static void
mod_init(struct module *const restrict m)
{
	MODULE_CONFLICT(m, "contrib/ns_waitreg")

	(void) hook_add_operserv_info(&waitreg_infohook);
	(void) hook_add_user_can_register(&waitreg_registerhook);

	(void) add_uint_conf_item("WAITREG_TIME", &nicksvs.me->conf_table, 0, &waitreg_time,
	                          WAITREG_TIME_MIN, WAITREG_TIME_MAX, WAITREG_TIME_DEF);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	(void) hook_del_operserv_info(&waitreg_infohook);
	(void) hook_del_user_can_register(&waitreg_registerhook);

	(void) del_conf_item("WAITREG_TIME", &nicksvs.me->conf_table);
}

SIMPLE_DECLARE_MODULE_V1("nickserv/waitreg", MODULE_UNLOAD_CAPABILITY_OK)
