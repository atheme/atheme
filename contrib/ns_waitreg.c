/*
 * Copyright (c) 2010 William Pitcock <nenolod@atheme.org>
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for delaying user registration
 */

#include "atheme.h"
#include "conf.h"
#include <limits.h>

DECLARE_MODULE_V1
(
	"nickserv/waitreg", false, _modinit, _moddeinit,
        PACKAGE_STRING,
        "Atheme Development Group <http://www.atheme.org>"
);

unsigned int waitreg_time = 0;

static void waitreg_hook(hook_user_register_check_t *hdata)
{

	return_if_fail(hdata != NULL);
	return_if_fail(hdata->si != NULL);
	return_if_fail(hdata->password != NULL);

	unsigned int nickage = CURRTIME - hdata->si->su->ts;

	if (nickage < waitreg_time)
	{
		command_fail(hdata->si, fault_badparams, _("You can not register your nick so soon after connecting. Please wait a while and try again."));
		hdata->approved++;
	}
}

void
_modinit(module_t *m)
{

	hook_add_event("user_can_register");
	hook_add_user_can_register(waitreg_hook);

	add_uint_conf_item("WAITREG_TIME", &conf_ni_table, 0, &waitreg_time, 0, INT_MAX, 0);
}

void
_moddeinit(void)
{
	hook_del_user_can_register(waitreg_hook);

	del_conf_item("WAITREG_TIME", &conf_ni_table);
}
