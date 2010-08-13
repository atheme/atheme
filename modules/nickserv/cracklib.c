/*
 * Copyright (c) 2010 William Pitcock <nenolod@atheme.org>
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for checking password security with cracklib.
 */

#include "atheme.h"
#include "conf.h"
#include <sys/stat.h>
#include <crack.h>

DECLARE_MODULE_V1
(
	"nickserv/cracklib", false, _modinit, _moddeinit,
        PACKAGE_STRING,
        "Atheme Development Group <http://www.atheme.org>"
);

bool cracklib_warn;

static void cracklib_config_ready(void *unused)
{
	module_t *m;
	m = module_find_published("nickserv/cracklib");

	if (nicksvs.cracklib_dict == NULL)
	{
		slog(LG_INFO, "%s requires that the cracklib_dict configuration option be set.", m->header->name);
		module_unload(m);
		return;
	}

	hook_del_config_ready(cracklib_config_ready);
}

static int check_dict (char *filename)
{
	struct stat sb;

	if (stat(filename, &sb) == -1)
	{
		slog(LG_ERROR, "No cracklib dictionary found, falling back to not using a dictionary.");
		return 0;
	}
	else
		return 1;
}

static void
cracklib_hook(hook_user_register_check_t *hdata)
{
	const char *cracklib_reason;
	char *dict = strcat(nicksvs.cracklib_dict, ".pwd");

	return_if_fail(hdata != NULL);
	return_if_fail(hdata->si != NULL);
	return_if_fail(hdata->password != NULL);

	if ((cracklib_reason = FascistCheck(hdata->password, check_dict(dict) ? nicksvs.cracklib_dict : NULL)) != NULL)
	{
		if (cracklib_warn)
			command_fail(hdata->si, fault_badparams, _("The password provided is insecure because %s. You may want to set a different password with /msg %s set password <password> ."), cracklib_reason, nicksvs.nick);
		else
		{
			command_fail(hdata->si, fault_badparams, _("The password provided is insecure: %s"), cracklib_reason);
			hdata->approved++;
		}
	}
}

void
_modinit(module_t *m)
{

	hook_add_event("user_can_register");
	hook_add_user_can_register(cracklib_hook);

	hook_add_event("config_ready");
	hook_add_config_ready(cracklib_config_ready); 

	add_bool_conf_item("CRACKLIB_WARN", &conf_ni_table, 0, &cracklib_warn, false);
}

void
_moddeinit(void)
{
	hook_del_user_can_register(cracklib_hook);
	hook_del_config_ready(cracklib_config_ready);

	del_conf_item("CRACKLIB_WARN", &conf_ni_table);
}
