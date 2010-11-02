/*
 * Copyright (c) 2010 William Pitcock <nenolod@atheme.org>
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for blocking registrations of guest nicks.
 * Particularly for use with webchat clients.
 *
 * To actually use this, add a line like the following to 
 * the nickserv {} block of your atheme.conf:
 * guestnick = "mib_";
 *
 */

#include "atheme.h"
#include "conf.h"

DECLARE_MODULE_V1
(
	"nickserv/guestnoreg", false, _modinit, _moddeinit,
        PACKAGE_STRING,
        "Atheme Development Group <http://www.atheme.org>"
);

static char *guestnick = NULL;

static void guestnoreg_hook(hook_user_register_check_t *hdata)
{
	return_if_fail(hdata != NULL);
	return_if_fail(hdata->si != NULL);

        if (!irccasecmp(guestnick, hdata->account))
        {
                command_fail(hdata->si, fault_badparams, _("Registering of guest nicknames is disallowed."));
                hdata->approved++;
        }

}

void
_modinit(module_t *m)
{

	hook_add_event("user_can_register");
	hook_add_user_can_register(guestnoreg_hook);

	add_dupstr_conf_item("GUESTNICK", &conf_ni_table, 0, &guestnick, NULL);
}

void
_moddeinit(void)
{
	hook_del_user_can_register(guestnoreg_hook);

	del_conf_item("GUESTNICK", &conf_ni_table);
}
