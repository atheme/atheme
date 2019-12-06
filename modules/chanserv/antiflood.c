/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2013 William Pitcock <nenolod@dereferenced.org>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 */

#include <atheme.h>

#define METADATA_KEY_ENFORCE_METHOD	"private:antiflood:enforce-method"

enum antiflood_enforce_method
{
	ANTIFLOOD_ENFORCE_QUIET = 0,
	ANTIFLOOD_ENFORCE_KICKBAN,
	ANTIFLOOD_ENFORCE_KLINE,
};

enum mqueue_enforce_strategy
{
	MQ_ENFORCE_NONE = 0,
	MQ_ENFORCE_MSG,
	MQ_ENFORCE_LINE,
};

struct antiflood_enforce_method_impl
{
	void (*enforce)(struct user *, struct channel *);
	void (*unenforce)(struct channel *);
};

struct flood_message_queue
{
	char *name;
	size_t max;
	time_t last_used;
	mowgli_list_t entries;
};

struct flood_message
{
	stringref source;
	char *message;
	time_t time;
	mowgli_node_t node;
};

static struct chanban *(*place_quietmask)(struct channel *, int, const char *) = NULL;

static enum antiflood_enforce_method antiflood_enforce_method = ANTIFLOOD_ENFORCE_QUIET;

static mowgli_heap_t *msg_heap = NULL;
static mowgli_heap_t *mqueue_heap = NULL;
static mowgli_patricia_t *mqueue_trie = NULL;
static mowgli_patricia_t **cs_set_cmdtree = NULL;
static mowgli_eventloop_timer_t *mqueue_gc_timer = NULL;
static mowgli_eventloop_timer_t *antiflood_unenforce_timer = NULL;

static time_t antiflood_msg_time = SECONDS_PER_MINUTE;
static size_t antiflood_msg_count = 10;

static void
msg_destroy(struct flood_message *mesg, struct flood_message_queue *mq)
{
	sfree(mesg->message);
	strshare_unref(mesg->source);
	mowgli_node_delete(&mesg->node, &mq->entries);

	mowgli_heap_free(msg_heap, mesg);
}

static struct flood_message *
msg_create(struct flood_message_queue *mq, struct user *u, const char *message)
{
	struct flood_message *mesg;

	mesg = mowgli_heap_alloc(msg_heap);
	mesg->message = sstrdup(message);
	mesg->time = CURRTIME;
	mesg->source = u->uid != NULL ? strshare_ref(u->uid) : strshare_ref(u->nick);

	if (MOWGLI_LIST_LENGTH(&mq->entries) > mq->max)
	{
		struct flood_message *head_mesg = mq->entries.head->data;
		msg_destroy(head_mesg, mq);
	}

	mowgli_node_add(mesg, &mesg->node, &mq->entries);
	mq->last_used = CURRTIME;

	return mesg;
}

static struct flood_message_queue *
mqueue_create(const char *name)
{
	struct flood_message_queue *mq;

	mq = mowgli_heap_alloc(mqueue_heap);
	mq->name = sstrdup(name);
	mq->last_used = CURRTIME;
	mq->max = antiflood_msg_count;

	mowgli_patricia_add(mqueue_trie, mq->name, mq);

	return mq;
}

static void
mqueue_free(struct flood_message_queue *mq)
{
	mowgli_node_t *n, *tn;

	MOWGLI_ITER_FOREACH_SAFE(n, tn, mq->entries.head)
	{
		struct flood_message *mesg = n->data;

		msg_destroy(mesg, mq);
	}

	sfree(mq->name);
	mowgli_heap_free(mqueue_heap, mq);
}

static struct flood_message_queue *
mqueue_get(struct mychan *mc)
{
	struct flood_message_queue *mq;

	mq = mowgli_patricia_retrieve(mqueue_trie, mc->name);
	if (mq == NULL)
		mq = mqueue_create(mc->name);

	return mq;
}

static void
mqueue_destroy(struct flood_message_queue *mq)
{
	mowgli_patricia_delete(mqueue_trie, mq->name);

	mqueue_free(mq);
}

static void
mqueue_trie_destroy_cb(const char *key, void *data, void *privdata)
{
	struct flood_message_queue *mq = data;

	mqueue_free(mq);
}

static void
mqueue_gc(void *unused)
{
	mowgli_patricia_iteration_state_t iter;
	struct flood_message_queue *mq;

	MOWGLI_PATRICIA_FOREACH(mq, &iter, mqueue_trie)
	{
		if ((mq->last_used + SECONDS_PER_HOUR) < CURRTIME)
			mqueue_destroy(mq);
	}
}

static enum mqueue_enforce_strategy
mqueue_should_enforce(struct flood_message_queue *mq)
{
	struct flood_message *oldest, *newest;
	time_t age_delta;

	if (MOWGLI_LIST_LENGTH(&mq->entries) < mq->max)
		return MQ_ENFORCE_NONE;

	oldest = mq->entries.head->data;
	newest = mq->entries.tail->data;

	if (oldest == NULL || newest == NULL || oldest == newest)
		return MQ_ENFORCE_NONE;

	age_delta = newest->time - oldest->time;

	if (age_delta <= antiflood_msg_time)
	{
		mowgli_node_t *n;
		size_t msg_matches = 0, usr_matches = 0;
		time_t usr_first_seen = 0;

		MOWGLI_ITER_FOREACH(n, mq->entries.head)
		{
			struct flood_message *mesg = n->data;

			if (!strcasecmp(mesg->message, newest->message))
				msg_matches++;

			if (mesg->source == newest->source)
			{
				usr_matches++;

				if (!usr_first_seen)
					usr_first_seen = mesg->time;
			}
		}

		if (msg_matches > (antiflood_msg_count / 2))
			return MQ_ENFORCE_MSG;

		if (usr_matches > (antiflood_msg_count / 2) &&
			((newest->time - usr_first_seen) < antiflood_msg_time / 4))
			return MQ_ENFORCE_LINE;
	}

	return MQ_ENFORCE_NONE;
}

// this requires `chanserv/quiet` to be loaded.
static void
antiflood_enforce_quiet(struct user *u, struct channel *c)
{
	char hostbuf[BUFSIZE];

	mowgli_strlcpy(hostbuf, "*!*@", sizeof hostbuf);
	mowgli_strlcat(hostbuf, u->vhost, sizeof hostbuf);

	if (place_quietmask != NULL)
	{
		struct chanban *cb;

		cb = place_quietmask(c, MTYPE_ADD, hostbuf);
		if (cb != NULL)
			cb->flags |= CBAN_ANTIFLOOD;
	slog(LG_INFO, "ANTIFLOOD:ENFORCE:QUIET: \2%s!%s@%s\2 on \2%s\2", u->nick, u->user, u->vhost, c->name);
	}
}

static void
antiflood_unenforce_banlike(struct channel *c)
{
	mowgli_node_t *n, *tn;

	MOWGLI_ITER_FOREACH_SAFE(n, tn, c->bans.head)
	{
		struct chanban *cb = n->data;

		if (!(cb->flags & CBAN_ANTIFLOOD))
			continue;

		modestack_mode_param(chansvs.nick, c, MTYPE_DEL, cb->type, cb->mask);
		chanban_delete(cb);
	}
}

static void
antiflood_enforce_kickban(struct user *u, struct channel *c)
{
	struct chanban *cb;

	ban(chansvs.me->me, c, u);
	remove_ban_exceptions(chansvs.me->me, c, u);
	try_kick(chansvs.me->me, c, u, "Flooding");

	// poison tail
	if (c->bans.tail != NULL)
	{
		cb = c->bans.tail->data;
		cb->flags |= CBAN_ANTIFLOOD;
	}
	else if (c->bans.head != NULL)
	{
		cb = c->bans.head->data;
		cb->flags |= CBAN_ANTIFLOOD;
	}
	slog(LG_INFO, "ANTIFLOOD:ENFORCE:KICKBAN: \2%s!%s@%s\2 from \2%s\2", u->nick, u->user, u->vhost, c->name);
}

static void
antiflood_enforce_kline(struct user *u, struct channel *c)
{
	kline_add_user(u, "Flooding", SECONDS_PER_DAY, chansvs.nick);
	slog(LG_INFO, "ANTIFLOOD:ENFORCE:AKILL: \2%s!%s@%s\2 from \2%s\2", u->nick, u->user, u->vhost, c->name);
}

static inline const struct antiflood_enforce_method_impl *
antiflood_enforce_method_impl_get(struct mychan *mc)
{
	static const struct antiflood_enforce_method_impl antiflood_enforce_methods[] = {
		[ANTIFLOOD_ENFORCE_QUIET]   = { &antiflood_enforce_quiet, &antiflood_unenforce_banlike },
		[ANTIFLOOD_ENFORCE_KICKBAN] = { &antiflood_enforce_kickban, &antiflood_unenforce_banlike },
		[ANTIFLOOD_ENFORCE_KLINE]   = { &antiflood_enforce_kline, NULL },
	};

	struct metadata *md;

	md = metadata_find(mc, METADATA_KEY_ENFORCE_METHOD);
	if (md != NULL)
	{
		if (!strcasecmp(md->value, "QUIET"))
			return &antiflood_enforce_methods[ANTIFLOOD_ENFORCE_QUIET];
		else if (!strcasecmp(md->value, "KICKBAN"))
			return &antiflood_enforce_methods[ANTIFLOOD_ENFORCE_KICKBAN];
		else if (!strcasecmp(md->value, "AKILL"))
			return &antiflood_enforce_methods[ANTIFLOOD_ENFORCE_KLINE];
	}

	return &antiflood_enforce_methods[antiflood_enforce_method];
}

static void
antiflood_unenforce_timer_cb(void *unused)
{
	mowgli_patricia_iteration_state_t state;
	struct mychan *mc;

	MOWGLI_PATRICIA_FOREACH(mc, &state, mclist)
	{
		const struct antiflood_enforce_method_impl *enf = antiflood_enforce_method_impl_get(mc);

		if (mc->chan == NULL)
			continue;

		if (enf->unenforce != NULL)
			enf->unenforce(mc->chan);
	}
}

static void
on_channel_message(struct hook_channel_message *data)
{
	struct chanuser *cu;
	struct mychan *mc;
	struct flood_message_queue *mq;

	return_if_fail(data != NULL);
	return_if_fail(data->msg != NULL);
	return_if_fail(data->u != NULL);
	return_if_fail(data->c != NULL);

	cu = chanuser_find(data->c, data->u);
	if (cu == NULL)
		return;

	mc = mychan_from(data->c);
	if (mc == NULL)
		return;

	mq = mqueue_get(mc);
	return_if_fail(mq != NULL);

	msg_create(mq, data->u, data->msg);

	// never enforce against any user who has special CSTATUS flags.
	if (cu->modes)
		return;

	// do not enforce unless enforcement is specifically enabled
	if (!(mc->flags & MC_ANTIFLOOD))
		return;

	if (mqueue_should_enforce(mq) != MQ_ENFORCE_NONE)
	{
		const struct antiflood_enforce_method_impl *enf = antiflood_enforce_method_impl_get(mc);

		if (enf == NULL || enf->enforce == NULL)
			return;

		enf->enforce(data->u, data->c);
	}
}

static void
on_channel_drop(struct mychan *mc)
{
	struct flood_message_queue *mq;

	mq = mqueue_get(mc);
	return_if_fail(mq != NULL);

	mqueue_destroy(mq);
}

static void
cs_set_cmd_antiflood(struct sourceinfo *si, int parc, char *parv[])
{
	struct mychan *mc;

	if (!(mc = mychan_find(parv[0])))
	{
		command_fail(si, fault_nosuch_target, STR_IS_NOT_REGISTERED, parv[0]);
		return;
	}

	/* allow opers with PRIV_CHAN_ADMIN to override this setting since it has
	   oper-specific settings (i.e. AKILL action) */
	if (!chanacs_source_has_flag(mc, si, CA_SET) && !has_priv(si, PRIV_CHAN_ADMIN))
	{
		command_fail(si, fault_noprivs, STR_NOT_AUTHORIZED);
		return;
	}

	if (parv[1] == NULL)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SET ANTIFLOOD");
		return;
	}

	if (!strcasecmp(parv[1], "OFF"))
	{
		mc->flags &= ~MC_ANTIFLOOD;
		metadata_delete(mc, METADATA_KEY_ENFORCE_METHOD);

		logcommand(si, CMDLOG_SET, "ANTIFLOOD:NONE: \2%s\2",  mc->name);
		command_success_nodata(si, _("Flood protection turned off for \2%s\2."), mc->name);
		return;
	}
	else if (!strcasecmp(parv[1], "ON"))
	{
		if (MC_ANTIFLOOD & mc->flags)
		{
			command_fail(si, fault_nochange, _("The \2%s\2 flag is already set for channel \2%s\2."), "ANTIFLOOD", mc->name);
			return;
		}
		mc->flags |= MC_ANTIFLOOD;
		metadata_delete(mc, METADATA_KEY_ENFORCE_METHOD);

		logcommand(si, CMDLOG_SET, "ANTIFLOOD: %s (%s)",  mc->name, "DEFAULT");
		command_success_nodata(si, _("Flood protection turned on for \2%s\2 with default settings."), mc->name);
		return;
	}
	else if (!strcasecmp(parv[1], "QUIET"))
	{
		mc->flags |= MC_ANTIFLOOD;
		metadata_add(mc, METADATA_KEY_ENFORCE_METHOD, "QUIET");

		logcommand(si, CMDLOG_SET, "ANTIFLOOD: %s (%s)",  mc->name, "QUIET");
		command_success_nodata(si, _("Flood protection turned on for \2%s\2 with \2%s\2 action."), mc->name, "QUIET");
		return;
	}
	else if (!strcasecmp(parv[1], "KICKBAN"))
	{
		mc->flags |= MC_ANTIFLOOD;
		metadata_add(mc, METADATA_KEY_ENFORCE_METHOD, "KICKBAN");

		logcommand(si, CMDLOG_SET, "ANTIFLOOD: %s (%s)",  mc->name, "KICKBAN");
		command_success_nodata(si, _("Flood protection turned on for \2%s\2 with \2%s\2 action."), mc->name, "KICKBAN");
		return;
	}
	else if (!strcasecmp(parv[1], "AKILL") || !strcasecmp(parv[1], "KLINE"))
	{
		if (has_priv(si, PRIV_AKILL))
		{
			mc->flags |= MC_ANTIFLOOD;
			metadata_add(mc, METADATA_KEY_ENFORCE_METHOD, "AKILL");

			logcommand(si, CMDLOG_SET, "ANTIFLOOD: %s (%s)",  mc->name, "AKILL");
			command_success_nodata(si, _("Flood protection turned on for \2%s\2 with \2%s\2 action."), mc->name, "AKILL");
			return;
		}
		else
		{
			command_fail(si, fault_noprivs, STR_NOT_AUTHORIZED);
			return;
		}
	}
}

static int
c_ci_antiflood_enforce_method(mowgli_config_file_entry_t *ce)
{
	if (ce->vardata == NULL)
	{
		conf_report_warning(ce, "no parameter for configuration option");
		return 0;
	}

	if (!strcasecmp(ce->vardata, "QUIET"))
		antiflood_enforce_method = ANTIFLOOD_ENFORCE_QUIET;
	else if (!strcasecmp(ce->vardata, "KICKBAN"))
		antiflood_enforce_method = ANTIFLOOD_ENFORCE_KICKBAN;
	else if (!strcasecmp(ce->vardata, "AKILL") || !strcasecmp(ce->vardata, "KLINE"))
		antiflood_enforce_method = ANTIFLOOD_ENFORCE_KLINE;

	return 0;
}

static struct command cs_set_antiflood = {
	.name           = "ANTIFLOOD",
	.desc           = N_("Set anti-flood action"),
	.access         = AC_NONE,
	.maxparc        = 2,
	.cmd            = &cs_set_cmd_antiflood,
	.help           = { .path = "cservice/set_antiflood" },
};

static void
mod_init(struct module *m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "chanserv/main")
	MODULE_TRY_REQUEST_SYMBOL(m, cs_set_cmdtree, "chanserv/set_core", "cs_set_cmdtree")

	/* attempt to pull in the place_quietmask() routine from chanserv/quiet,
	   we don't see it as a hardfail because we can run without QUIET support. */
	if (module_request("chanserv/quiet"))
	{
		place_quietmask = module_locate_symbol("chanserv/quiet", "place_quietmask");
		if (place_quietmask == NULL)
			antiflood_enforce_method = ANTIFLOOD_ENFORCE_KICKBAN;
	}

	hook_add_channel_message(on_channel_message);
	hook_add_channel_drop(on_channel_drop);

	msg_heap = sharedheap_get(sizeof(struct flood_message));

	mqueue_heap = sharedheap_get(sizeof(struct flood_message_queue));
	mqueue_trie = mowgli_patricia_create(irccasecanon);
	mqueue_gc_timer = mowgli_timer_add(base_eventloop, "mqueue_gc", mqueue_gc, NULL, 5 * SECONDS_PER_MINUTE);

	antiflood_unenforce_timer = mowgli_timer_add(base_eventloop, "antiflood_unenforce", antiflood_unenforce_timer_cb, NULL, SECONDS_PER_HOUR);

	command_add(&cs_set_antiflood, *cs_set_cmdtree);

	add_conf_item("ANTIFLOOD_ENFORCE_METHOD", &chansvs.me->conf_table, c_ci_antiflood_enforce_method);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	command_delete(&cs_set_antiflood, *cs_set_cmdtree);

	hook_del_channel_message(on_channel_message);
	hook_del_channel_drop(on_channel_drop);

	mowgli_patricia_destroy(mqueue_trie, mqueue_trie_destroy_cb, NULL);
	mowgli_timer_destroy(base_eventloop, mqueue_gc_timer);
	mowgli_timer_destroy(base_eventloop, antiflood_unenforce_timer);

	del_conf_item("ANTIFLOOD_ENFORCE_METHOD", &chansvs.me->conf_table);
}

SIMPLE_DECLARE_MODULE_V1("chanserv/antiflood", MODULE_UNLOAD_CAPABILITY_OK)
