/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2010 Atheme Project (http://atheme.org/)
 * Copyright (C) 2014-2016 Xtheme Development Group
 *
 * chanfix - channel fixing service
 */

#include "atheme.h"
#include "chanfix.h"

static mowgli_eventloop_timer_t *chanfix_autofix_timer = NULL;

struct service *chanfix;

static void
mod_init(struct module *const restrict m)
{
	struct chanfix_persist_record *rec = mowgli_global_storage_get("atheme.chanfix.main.persist");

	chanfix_gather_init(rec);

	if (rec != NULL)
	{
		sfree(rec);
		return;
	}

	chanfix = service_add("chanfix", NULL);
	service_bind_command(chanfix, &cmd_list);
	service_bind_command(chanfix, &cmd_chanfix);
	service_bind_command(chanfix, &cmd_scores);
	service_bind_command(chanfix, &cmd_info);
	service_bind_command(chanfix, &cmd_help);
	service_bind_command(chanfix, &cmd_mark);
	service_bind_command(chanfix, &cmd_nofix);

	hook_add_event("channel_can_register");
	hook_add_channel_can_register(chanfix_can_register);

	add_bool_conf_item("AUTOFIX", &chanfix->conf_table, 0, &chanfix_do_autofix, false);

	chanfix_autofix_timer = mowgli_timer_add(base_eventloop, "chanfix_autofix", chanfix_autofix_ev, NULL, 60);

	m->mflags |= MODFLAG_DBHANDLER;
}

static void
mod_deinit(const enum module_unload_intent intent)
{
	struct chanfix_persist_record *rec = NULL;

	hook_del_channel_can_register(chanfix_can_register);

	mowgli_timer_destroy(base_eventloop, chanfix_autofix_timer);

	if (chanfix)
		service_delete(chanfix);

	switch (intent)
	{
		case MODULE_UNLOAD_INTENT_RELOAD:
		{
			rec = smalloc(sizeof *rec);
			rec->version = 1;

			mowgli_global_storage_put("atheme.chanfix.main.persist", rec);
			break;
		}

		case MODULE_UNLOAD_INTENT_PERM:
			break;
	}

	chanfix_gather_deinit(intent, rec);
}

SIMPLE_DECLARE_MODULE_V1("chanfix/main", MODULE_UNLOAD_CAPABILITY_OK)
