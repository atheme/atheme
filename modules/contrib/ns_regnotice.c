/*
 * Copyright (c) 2010 William Pitcock <nenolod@atheme.org>
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Sends a customized welcome message on nickname registration.
 */

#include "atheme.h"
#include "conf.h"

DECLARE_MODULE_V1
(
	"nickserv/regnotice", false, _modinit, _moddeinit,
        PACKAGE_STRING,
        "Atheme Development Group <http://www.atheme.org>"
);

static list_t regnotices = {};

static void regnotice_hook(myuser_t *mu)
{
	node_t *n;

	return_if_fail(mu != NULL);

	LIST_FOREACH(n, regnotices.head)
	{
		char *line = n->data;

		myuser_notice(nicksvs.nick, mu, "%s", line);
	}
}

static int regnotice_config_handler(config_entry_t *ce)
{
	node_t *n;
	config_entry_t *cce;

	for (cce = ce->ce_entries; cce != NULL; cce = cce->ce_next)
	{
		char *line = sstrdup(cce->ce_varname);
		node_add(line, node_create(), &regnotices);
	}

	return 0;
}

static void regnotice_config_purge(void *unused)
{
	node_t *n, *tn;

	LIST_FOREACH_SAFE(n, tn, regnotices.head)
	{
		char *line = n->data;

		free(line);
		node_del(n, &regnotices);
		node_free(n);
	}
}

void
_modinit(module_t *m)
{
	hook_add_event("config_purge");
	hook_add_config_purge(regnotice_config_purge);

	hook_add_event("user_register");
	hook_add_user_register(regnotice_hook);

	add_conf_item("REGNOTICE", &conf_ni_table, regnotice_config_handler);
}

void
_moddeinit(void)
{
	hook_del_user_register(regnotice_hook);
	hook_del_config_purge(regnotice_config_purge);

	del_conf_item("REGNOTICE", &conf_ni_table);
}
