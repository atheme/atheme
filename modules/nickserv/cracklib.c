/*
 * Copyright (c) 2010 William Pitcock <nenolod@atheme.org>
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for checking password security with cracklib.
 */

#include "atheme.h"
#include "conf.h"
#include <crack.h>

DECLARE_MODULE_V1
(
	"nickserv/cracklib", false, _modinit, _moddeinit,
        "$Id$",
        "Atheme Development Group <http://www.atheme.org>"
);

void
cracklib_hook(hook_user_register_check_t *hdata)
{
	const char *cracklib_reason;

	return_if_fail(hdata != NULL);
	return_if_fail(hdata->si != NULL);
	return_if_fail(hdata->password != NULL);

	if ((cracklib_reason = FascistCheck(hdata->password, nicksvs.cracklib_dict)) != NULL)
	{
		command_fail(hdata->si, fault_badparams, _("The password provided is insecure: %s"), cracklib_reason);
		hdata->approved++;
	}
}

void
_modinit(module_t *m)
{
	hook_add_event("user_can_register");
	hook_add_user_can_register(cracklib_hook);
}

void
_moddeinit(void)
{
	hook_del_user_can_register(cracklib_hook);
}
