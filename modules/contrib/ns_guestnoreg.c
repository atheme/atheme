/*
 * Copyright (c) 2010 William Pitcock <nenolod@atheme.org>
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for blocking registrations of guest nicks.
 * Particularly for use with webchat clients.
 *
 * To actually use this, add a something like the following to 
 * the nickserv {} block of your atheme.conf:
 * guestnicks {
 *      "mib_";
 *      "WebUser";
 *  };
 *
 */

#include "atheme.h"
#include "conf.h"

DECLARE_MODULE_V1
(
	"contrib/ns_guestnoreg", false, _modinit, _moddeinit,
        PACKAGE_STRING,
        "Atheme Development Group <http://www.atheme.org>"
);

static mowgli_list_t guestnicks = { NULL, NULL, 0 };

static void guestnoreg_hook(hook_user_register_check_t *hdata)
{
        mowgli_node_t *n;

	return_if_fail(hdata != NULL);
	return_if_fail(hdata->si != NULL);

        MOWGLI_ITER_FOREACH(n, guestnicks.head)
        {
                char *nick = n->data;
                int nicklen = strlen(nick);

                if (!strncasecmp(hdata->account, nick, nicklen))
                {
                        command_fail(hdata->si, fault_badparams, _("Registering of guest nicknames is disallowed."));
                        hdata->approved++;
                }
        }

}

static int guestnoreg_config_handler(mowgli_config_file_entry_t *ce)
{
        mowgli_config_file_entry_t *cce;

	MOWGLI_ITER_FOREACH(cce, ce->entries)
        {
                char *nick = sstrdup(cce->varname);
                mowgli_node_add(nick, mowgli_node_create(), &guestnicks);
        }

        return 0;
}

static void guestnoreg_config_purge(void *unused)
{
        mowgli_node_t *n, *tn;

        MOWGLI_ITER_FOREACH_SAFE(n, tn, guestnicks.head)
        {
                char *nick = n->data;

                free(nick);
                mowgli_node_delete(n, &guestnicks);
                mowgli_node_free(n);
        }
}

void
_modinit(module_t *m)
{
        hook_add_event("config_purge");
        hook_add_config_purge(guestnoreg_config_purge);

	hook_add_event("user_can_register");
	hook_add_user_can_register(guestnoreg_hook);

        add_conf_item("GUESTNICKS", &nicksvs.me->conf_table, guestnoreg_config_handler);
}

void
_moddeinit(module_unload_intent_t intent)
{
	hook_del_user_can_register(guestnoreg_hook);
        hook_del_config_purge(guestnoreg_config_purge);

        del_conf_item("GUESTNICKS", &nicksvs.me->conf_table);
}
