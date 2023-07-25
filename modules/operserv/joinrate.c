/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2021 Jess <github@lolnerd.net>
 * Copyright (C) 2023 Atheme Development Group (https://atheme.github.io/)
 *
 * Watches channels for abnormally high join rates
 */

#include <atheme.h>

#define RELOAD_CAPSULE_MDNAME   "atheme.mod.operserv.joinrate.reload"

#ifdef ATHEME_ENABLE_LARGE_NET
#  define JOINRATE_HEAP_SIZE    16384
#else
#  define JOINRATE_HEAP_SIZE    256
#endif

#define JOINRATE_DBENTRY_INST   "JR"

#define JOINRATE_RATE_DEFAULT   5
#define JOINRATE_TIME_DEFAULT   5

struct jr_instance
{
	char *                  chname;
	int                     rate;
	int                     time;
	int                     tokens;
	time_t                  last_join_time;
	time_t                  last_warn_time;
};

struct ATHEME_VATTR_ALIGNED_MAX reload_capsule
{
	unsigned int            version;
};

struct ATHEME_VATTR_ALIGNED_MAX reload_capsule_v1
{
	unsigned int            version;
	mowgli_heap_t *         instance_heap;
	mowgli_patricia_t *     instance_trie;
	struct jr_instance *    default_instance;
};

static mowgli_heap_t *instance_heap = NULL;
static mowgli_patricia_t *instance_trie = NULL;
static struct jr_instance *default_instance = NULL;

static struct jr_instance * ATHEME_FATTR_WUR
jr_instance_create(const char *const restrict chname)
{
	return_val_if_fail(chname != NULL, NULL);

	struct jr_instance *const instance = mowgli_heap_alloc(instance_heap);

	instance->chname         = sstrdup(chname);
	instance->rate           = -1;
	instance->time           = -1;
	instance->tokens         = -1;
	instance->last_join_time = CURRTIME;

	(void) mowgli_patricia_add(instance_trie, instance->chname, instance);

	return instance;
}

static struct jr_instance *
jr_instance_retrieve(const char *const restrict chname)
{
	return_val_if_fail(chname != NULL, NULL);

	return mowgli_patricia_retrieve(instance_trie, chname);
}

static struct jr_instance * ATHEME_FATTR_WUR
jr_instance_retrieve_or_create(const char *const restrict chname)
{
	return_val_if_fail(chname != NULL, NULL);

	struct jr_instance *const instance = jr_instance_retrieve(chname);

	if (instance)
		return instance;

	return jr_instance_create(chname);
}

static void
jr_instance_destroy(struct jr_instance *const restrict instance)
{
	return_if_fail(instance != NULL);

	(void) mowgli_patricia_delete(instance_trie, instance->chname);
	(void) sfree(instance->chname);
	(void) mowgli_heap_free(instance_heap, instance);
}

static void
channel_join_hook(struct hook_channel_joinpart *const restrict hdata)
{
	if (! (hdata && hdata->cu))
		// A previous channel_join hook may have kicked the user; do nothing
		return;

	return_if_fail(hdata->cu->chan != NULL);
	return_if_fail(hdata->cu->chan->name != NULL);
	return_if_fail(hdata->cu->user != NULL);
	return_if_fail(hdata->cu->user->server != NULL);

	const char *const chname = hdata->cu->chan->name;
	struct user *const userptr = hdata->cu->user;

	// Don't count JOINs in a netjoin, or from our clients
	if (is_internal_client(userptr) || me.bursting || ! (userptr->server->flags & SF_EOB))
		return;

	struct jr_instance *const instance = jr_instance_retrieve_or_create(chname);

	// Pick up default rate/time if this channel is set to defaults
	const int chrate = (instance->rate == -1 ? default_instance->rate : instance->rate) - 1;
	const int chtime = (instance->time == -1 ? default_instance->time : instance->time);
	if (instance->tokens == -1)
		instance->tokens = chrate;

	// Do some token bucket magic
	const time_t elapsed = (CURRTIME - instance->last_join_time);
	instance->tokens += (elapsed * chrate) / chtime;

	// Cap tokens at `rate`
	if (instance->tokens > chrate)
		instance->tokens = chrate;

	// Is there a token left for us to consume?
	if (instance->tokens > 0)
	{
		instance->tokens--;
	}
	else if ((CURRTIME - instance->last_warn_time) >= 30)
	{
		instance->last_warn_time = CURRTIME;

		(void) slog(LG_INFO, "JOINRATE: \2%s\2 exceeds warning threshold (%d joins in %ds)",
		                     chname, (chrate + 1), chtime);
	}

	instance->last_join_time = CURRTIME;
}

static void
db_write_hook(struct database_handle *const restrict db)
{
	mowgli_patricia_iteration_state_t state;
	struct jr_instance *instance;

	MOWGLI_PATRICIA_FOREACH(instance, &state, instance_trie)
	{
		if (instance->rate == -1)
			continue;

		(void) db_start_row(db, JOINRATE_DBENTRY_INST);
		(void) db_write_word(db, instance->chname);
		(void) db_write_int(db, instance->rate);
		(void) db_write_int(db, instance->time);
		(void) db_commit_row(db);
	}
}

static void
db_read_hook(struct database_handle *const restrict db, const char ATHEME_VATTR_UNUSED *const restrict type)
{
	const char *const chname = db_sread_word(db);
	const int rate = db_sread_int(db);
	const int time = db_sread_int(db);

	struct jr_instance *const instance = jr_instance_retrieve_or_create(chname);

	instance->rate = rate;
	instance->time = time;
	instance->tokens = -1;
}

static void
os_cmd_joinrate_func(struct sourceinfo *const restrict si, const int parc, char **const restrict parv)
{
	const char *const chname = parv[0];
	const char *const action = parv[1];
	const char *const arg_rate = parv[2];
	const char *const arg_time = parv[3];

	if (! (chname && action))
	{
		(void) command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "JOINRATE");
		(void) command_fail(si, fault_needmoreparams, _("Syntax: JOINRATE <#channel|DEFAULT> GET|SET|UNSET "
		                                                "[parameters]"));
		return;
	}

	struct jr_instance *instance = jr_instance_retrieve(chname);

	if (strcasecmp(action, "GET") == 0)
	{
		if (instance && instance->rate != -1)
			(void) command_success_nodata(si, _("Join rate warning threshold for \2%s\2 is %d joins in "
			                                    "%ds"), chname, instance->rate, instance->time);
		else
			(void) command_success_nodata(si, _("Join rate warning threshold for \2%s\2 is %d joins in "
		                                            "%ds (DEFAULT)"), chname, default_instance->rate,
			                                    default_instance->time);
	}
	else if (strcasecmp(action, "UNSET") == 0)
	{
		if (! (instance && instance->rate != -1))
		{
			(void) command_fail(si, fault_nochange, _("\2%s\2 is already set to the default join rate "
			                                          "warning thresholds"), chname);
			return;
		}

		if (strcasecmp(chname, "DEFAULT") == 0)
		{
			default_instance->rate = JOINRATE_RATE_DEFAULT;
			default_instance->time = JOINRATE_TIME_DEFAULT;

			// Reset available token-bucket tokens for channels using default rate/time
		        mowgli_patricia_iteration_state_t state;
		        MOWGLI_PATRICIA_FOREACH(instance, &state, instance_trie)
				if (instance->rate == -1)
					instance->tokens = -1;
		}
		else
			(void) jr_instance_destroy(instance);

		(void) command_success_nodata(si, _("Reset \2%s\2 to default join rate warning threshold (%d joins "
		                                    "in %ds)"), chname, default_instance->rate,
		                                    default_instance->time);
	}
	else if (strcasecmp(action, "SET") == 0)
	{
		unsigned int rate, time;

		if (! (arg_rate && arg_time))
		{
			(void) command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "JOINRATE");
			(void) command_fail(si, fault_needmoreparams, _("Syntax: JOINRATE <#channel|DEFAULT> SET "
			                                                "<rate> <time>"));
			return;
		}
		if (! (string_to_uint(arg_rate, &rate) && rate))
		{
			(void) command_fail(si, fault_badparams, _("Rate must be a non-zero positive integer"));
			return;
		}
		if (! (string_to_uint(arg_time, &time) && time))
		{
			(void) command_fail(si, fault_badparams, _("Time must be a non-zero positive integer"));
			return;
		}

		if (! instance)
			instance = jr_instance_create(chname);

		if (strcasecmp(chname, "DEFAULT") == 0)
		{
			default_instance->rate = (int) rate;
			default_instance->time = (int) time;

			// Reset available token-bucket tokens for channels using default rate/time
		        mowgli_patricia_iteration_state_t state;
		        MOWGLI_PATRICIA_FOREACH(instance, &state, instance_trie)
				if (instance->rate == -1)
					instance->tokens = -1;
		}
		else
		{
			instance->rate = (int) rate;
			instance->time = (int) time;
			instance->tokens = -1;
		}

		(void) command_success_nodata(si, _("Set \2%s\2 join rate warning threshold to %d joins in %ds"),
		                                    chname, (int) rate, (int) time);
	}
	else
		(void) command_fail(si, fault_badparams, STR_INVALID_PARAMS, "JOINRATE");
}

static struct command os_cmd_joinrate = {
	.name           = "JOINRATE",
	.desc           = N_("Configure join rate thresholds for channels"),
	.access         = PRIV_USER_ADMIN,
	.maxparc        = 4,
	.cmd            = &os_cmd_joinrate_func,
	.help           = { .path = "oservice/joinrate" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "operserv/main")

	if (! module_find_published("backend/opensex"))
	{
		(void) slog(LG_INFO, "Module %s requires use of the OpenSEX database backend, refusing to load",
		                     m->name);

		m->mflags |= MODFLAG_FAIL;
		return;
	}

	// Determine whether we were loaded or reloaded
	struct reload_capsule *const rc = mowgli_global_storage_get(RELOAD_CAPSULE_MDNAME);

	if (rc)
	{
		// Reloaded; we don't free any memory on error, because services is going to abort anyway

		if (rc->version == 1)
		{
			const struct reload_capsule_v1 *const rcv1 = (const struct reload_capsule_v1 *) rc;

			instance_heap    = rcv1->instance_heap;
			instance_trie    = rcv1->instance_trie;
			default_instance = rcv1->default_instance;
		}
		else
		{
			(void) slog(LG_ERROR, "%s: unknown reload schema '%u'; refusing to load",
			                      m->name, rc->version);

			m->mflags |= MODFLAG_FAIL;
			return;
		}

		(void) mowgli_global_storage_free(RELOAD_CAPSULE_MDNAME);
		(void) sfree(rc);
	}
	else
	{
		if (! (instance_heap = mowgli_heap_create(sizeof(struct jr_instance), JOINRATE_HEAP_SIZE, BH_NOW)))
		{
			(void) slog(LG_ERROR, "%s: mowgli_heap_create() failed; refusing to load", m->name);

			m->mflags |= MODFLAG_FAIL;
			return;
		}
		if (! (instance_trie = mowgli_patricia_create(&irccasecanon)))
		{
			(void) slog(LG_ERROR, "%s: mowgli_patricia_create() failed; refusing to load", m->name);
			(void) mowgli_heap_destroy(instance_heap);

			m->mflags |= MODFLAG_FAIL;
			return;
		}

		default_instance = jr_instance_create("DEFAULT");
		default_instance->rate = JOINRATE_RATE_DEFAULT;
		default_instance->time = JOINRATE_TIME_DEFAULT;
	}

	(void) service_named_bind_command("operserv", &os_cmd_joinrate);

	(void) hook_add_channel_join(&channel_join_hook);
	(void) hook_add_db_write(&db_write_hook);

	(void) db_register_type_handler(JOINRATE_DBENTRY_INST, &db_read_hook);

	m->mflags |= MODFLAG_DBHANDLER;
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	(void) service_named_unbind_command("operserv", &os_cmd_joinrate);

	(void) hook_del_channel_join(&channel_join_hook);
	(void) hook_del_db_write(&db_write_hook);

	(void) db_unregister_type_handler(JOINRATE_DBENTRY_INST);

	struct reload_capsule_v1 *const rc = smalloc(sizeof *rc);

	rc->version             = 1U;
	rc->instance_heap       = instance_heap;
	rc->instance_trie       = instance_trie;
	rc->default_instance    = default_instance;

	(void) mowgli_global_storage_put(RELOAD_CAPSULE_MDNAME, rc);
}

SIMPLE_DECLARE_MODULE_V1("operserv/joinrate", MODULE_UNLOAD_CAPABILITY_RELOAD_ONLY)
