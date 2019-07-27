/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2019 Atheme Development Group (https://atheme.github.io/)
 *
 * Reminds user of their current account e-mail address once per year
 * (configurable), useful for reducing password reset problems on networks
 * with large amounts of users.
 */

#include <atheme.h>

#define EMAIL_REMIND_INTERVAL_MIN       4U
#define EMAIL_REMIND_INTERVAL_DEF       52U
#define EMAIL_REMIND_INTERVAL_MAX       52U

#define METADATA_KEYNAME                "private:remindemail:nextts"

static unsigned int email_remind_interval;

static void
config_ready_hook(void ATHEME_VATTR_UNUSED *const restrict unused)
{
	if (email_remind_interval < (SECONDS_PER_WEEK * EMAIL_REMIND_INTERVAL_MIN))
		email_remind_interval = (SECONDS_PER_WEEK * EMAIL_REMIND_INTERVAL_MIN);

	if (email_remind_interval > (SECONDS_PER_WEEK * EMAIL_REMIND_INTERVAL_MAX))
		email_remind_interval = (SECONDS_PER_WEEK * EMAIL_REMIND_INTERVAL_MAX);
}

static void
user_identify_hook(struct user *const restrict u)
{
	return_if_fail(u != NULL);
	return_if_fail(u->myuser != NULL);

	if (! module_find_published("nickserv/sendpass") && ! module_find_published("nickserv/sendpass_user"))
		return;

	if (! module_find_published("nickserv/set_email"))
		return;

	char buf[BUFSIZE];
	struct metadata *md;

	if ((md = metadata_find(u->myuser, METADATA_KEYNAME)) != NULL)
	{
		if (atoi(md->value) > CURRTIME)
			return;

		(void) metadata_delete(u->myuser, METADATA_KEYNAME);
	}

	(void) snprintf(buf, sizeof buf, "%lu", (unsigned long) (CURRTIME + email_remind_interval));
	(void) metadata_add(u->myuser, METADATA_KEYNAME, buf);

	if (validemail(u->myuser->email))
		(void) notice(nicksvs.nick, u->nick, "Your services account e-mail address is currently set to: "
		                                     "\2%s\2. If this is no longer correct, please change it by "
		                                     "executing the following command:", u->myuser->email);
	else
		(void) notice(nicksvs.nick, u->nick, "Your services account does not have a valid e-mail address "
		                                     "associated with it. To enable your account password to be "
		                                     "reset in the future, it is recommended that you set a valid "
		                                     "address now. To do so, please execute the following command:");

	(void) notice(nicksvs.nick, u->nick, "/msg %s SET EMAIL \2your.new@email.address\2", nicksvs.me->disp);
}

static void
mod_init(struct module ATHEME_VATTR_UNUSED *const restrict m)
{
	(void) hook_add_config_ready(&config_ready_hook);
	(void) hook_add_user_identify(&user_identify_hook);

	(void) add_duration_conf_item("EMAIL_REMIND_INTERVAL", &nicksvs.me->conf_table, 0, &email_remind_interval,
	                              "w", EMAIL_REMIND_INTERVAL_DEF);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	(void) hook_del_config_ready(&config_ready_hook);
	(void) hook_del_user_identify(&user_identify_hook);

	(void) del_conf_item("EMAIL_REMIND_INTERVAL", &nicksvs.me->conf_table);
}

SIMPLE_DECLARE_MODULE_V1("nickserv/remind_email", MODULE_UNLOAD_CAPABILITY_OK)
