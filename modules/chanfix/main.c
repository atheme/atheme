/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2010 Atheme Project (http://atheme.org/)
 * Copyright (C) 2014-2016 Xtheme Development Group
 *
 * chanfix - channel fixing service
 */

#include <atheme.h>
#include "chanfix.h"

#define CHANFIX_PERSIST_STORAGE_NAME "atheme.chanfix.main.persist"
#define CHANFIX_PERSIST_VERSION      2

static mowgli_eventloop_timer_t *chanfix_autofix_timer = NULL;

struct service *chanfix;

static void
mod_init(struct module *const restrict m)
{
	struct chanfix_persist_record *rec = mowgli_global_storage_get(CHANFIX_PERSIST_STORAGE_NAME);

	if (rec && rec->version > CHANFIX_PERSIST_VERSION)
	{
		slog(LG_ERROR, "chanfix/main: attempted downgrade is not supported (from %d to %d)", rec->version, CHANFIX_PERSIST_VERSION);
		m->mflags = MODFLAG_FAIL;

		sfree(rec);
		mowgli_global_storage_free(CHANFIX_PERSIST_STORAGE_NAME);

		return;
	}

	chanfix_gather_init(rec);

	if (rec != NULL)
	{
		sfree(rec);
		mowgli_global_storage_free(CHANFIX_PERSIST_STORAGE_NAME);
	}

	chanfix = service_add("chanfix", NULL);

	service_bind_command(chanfix, &cmd_list);
	service_bind_command(chanfix, &cmd_chanfix);
	service_bind_command(chanfix, &cmd_scores);
	service_bind_command(chanfix, &cmd_info);
	service_bind_command(chanfix, &cmd_help);
	service_bind_command(chanfix, &cmd_mark);
	service_bind_command(chanfix, &cmd_nofix);

	hook_add_channel_can_register(chanfix_can_register);

	add_bool_conf_item("AUTOFIX", &chanfix->conf_table, 0, &chanfix_do_autofix, false);

	chanfix_autofix_timer = mowgli_timer_add(base_eventloop, "chanfix_autofix", chanfix_autofix_ev, NULL, SECONDS_PER_MINUTE);

	m->mflags |= MODFLAG_DBHANDLER;
}

static void
mod_deinit(const enum module_unload_intent intent)
{
	hook_del_channel_can_register(chanfix_can_register);

	mowgli_timer_destroy(base_eventloop, chanfix_autofix_timer);

	service_unbind_command(chanfix, &cmd_list);
	service_unbind_command(chanfix, &cmd_chanfix);
	service_unbind_command(chanfix, &cmd_scores);
	service_unbind_command(chanfix, &cmd_info);
	service_unbind_command(chanfix, &cmd_help);
	service_unbind_command(chanfix, &cmd_mark);
	service_unbind_command(chanfix, &cmd_nofix);

	hook_del_channel_can_register(chanfix_can_register);

	del_conf_item("AUTOFIX", &chanfix->conf_table);

	service_delete(chanfix);

	struct chanfix_persist_record *rec = smalloc(sizeof *rec);
	rec->version = CHANFIX_PERSIST_VERSION;

	chanfix_gather_deinit(rec);

	mowgli_global_storage_put(CHANFIX_PERSIST_STORAGE_NAME, rec);
}

SIMPLE_DECLARE_MODULE_V1("chanfix/main", MODULE_UNLOAD_CAPABILITY_RELOAD_ONLY)
