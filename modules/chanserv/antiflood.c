/*
 * Copyright (c) 2013 William Pitcock <nenolod@dereferenced.org>.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/antiflood", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org/>"
);

static int antiflood_msg_time = 60;
static int antiflood_msg_count = 10;

#define METADATA_KEY_ENFORCE_METHOD	"private:antiflood:enforce-method"

typedef enum {
	ANTIFLOOD_ENFORCE_QUIET = 0,
	ANTIFLOOD_ENFORCE_KICKBAN,
	ANTIFLOOD_ENFORCE_KLINE,
	ANTIFLOOD_ENFORCE_COUNT
} antiflood_enforce_method_t;

static antiflood_enforce_method_t antiflood_enforce_method = ANTIFLOOD_ENFORCE_QUIET;

typedef enum {
	MQ_ENFORCE_NONE = 0,
	MQ_ENFORCE_MSG,
	MQ_ENFORCE_LINE,
} mqueue_enforce_strategy_t;

typedef struct {
	char *name;
	size_t max;
	time_t last_used;
	mowgli_list_t entries;
} mqueue_t;

typedef struct {
	stringref source;
	char *message;
	time_t time;
	mowgli_node_t node;
} msg_t;

static mowgli_heap_t *msg_heap = NULL;

static void
msg_destroy(msg_t *msg, mqueue_t *mq)
{
	free(msg->message);
	strshare_unref(msg->source);
	mowgli_node_delete(&msg->node, &mq->entries);

	mowgli_heap_free(msg_heap, msg);
}

static msg_t *
msg_create(mqueue_t *mq, user_t *u, const char *message)
{
	msg_t *msg;

	msg = mowgli_heap_alloc(msg_heap);
	msg->message = sstrdup(message);
	msg->time = CURRTIME;
	msg->source = u->uid != NULL ? strshare_ref(u->uid) : strshare_ref(u->nick);

	if (MOWGLI_LIST_LENGTH(&mq->entries) > mq->max)
	{
		msg_t *head_msg = mq->entries.head->data;
		msg_destroy(head_msg, mq);
	}

	mowgli_node_add(msg, &msg->node, &mq->entries);
	mq->last_used = CURRTIME;

	return msg;
}

static mowgli_patricia_t *mqueue_trie = NULL;
static mowgli_heap_t *mqueue_heap = NULL;
static mowgli_eventloop_timer_t *mqueue_gc_timer = NULL;

static mqueue_t *
mqueue_create(const char *name)
{
	mqueue_t *mq;

	mq = mowgli_heap_alloc(mqueue_heap);
	mq->name = sstrdup(name);
	mq->last_used = CURRTIME;
	mq->max = antiflood_msg_count;

	mowgli_patricia_add(mqueue_trie, mq->name, mq);

	return mq;
}

static void
mqueue_free(mqueue_t *mq)
{
	mowgli_node_t *n, *tn;

	MOWGLI_ITER_FOREACH_SAFE(n, tn, mq->entries.head)
	{
		msg_t *msg = n->data;

		msg_destroy(msg, mq);
	}

	free(mq->name);
	mowgli_heap_free(mqueue_heap, mq);
}

static mqueue_t *
mqueue_get(mychan_t *mc)
{
	mqueue_t *mq;

	mq = mowgli_patricia_retrieve(mqueue_trie, mc->name);
	if (mq == NULL)
		mq = mqueue_create(mc->name);

	return mq;
}

static void
mqueue_destroy(mqueue_t *mq)
{
	mowgli_patricia_delete(mqueue_trie, mq->name);

	mqueue_free(mq);
}

static void
mqueue_trie_destroy_cb(const char *key, void *data, void *privdata)
{
	mqueue_t *mq = data;

	mqueue_free(mq);
}

static void
mqueue_gc(void *unused)
{
	mowgli_patricia_iteration_state_t iter;
	mqueue_t *mq;

	MOWGLI_PATRICIA_FOREACH(mq, &iter, mqueue_trie)
	{
		if ((mq->last_used + 3600) < CURRTIME)
			mqueue_destroy(mq);
	}
}

static mqueue_enforce_strategy_t
mqueue_should_enforce(mqueue_t *mq)
{
	msg_t *oldest, *newest;
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
			msg_t *msg = n->data;

			if (!strcasecmp(msg->message, newest->message))
				msg_matches++;

			if (msg->source == newest->source)
			{
				usr_matches++;

				if (!usr_first_seen)
					usr_first_seen = msg->time;
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

static chanban_t *(*place_quietmask)(channel_t *c, int dir, const char *hostbuf) = NULL;

/* this requires `chanserv/quiet` to be loaded. */
static void
antiflood_enforce_quiet(user_t *u, channel_t *c)
{
	char hostbuf[BUFSIZE];

	mowgli_strlcpy(hostbuf, "*!*@", sizeof hostbuf);
	mowgli_strlcat(hostbuf, u->vhost, sizeof hostbuf);

	if (place_quietmask != NULL)
	{
		chanban_t *cb;

		cb = place_quietmask(c, MTYPE_ADD, hostbuf);
		if (cb != NULL)
			cb->flags |= CBAN_ANTIFLOOD;
	slog(LG_INFO, "ANTIFLOOD:ENFORCE:QUIET: \2%s!%s@%s\2 on \2%s\2", u->nick, u->user, u->vhost, c->name);
	}
}

static void
antiflood_unenforce_banlike(channel_t *c)
{
	mowgli_node_t *n, *tn;

	MOWGLI_ITER_FOREACH_SAFE(n, tn, c->bans.head)
	{
		chanban_t *cb = n->data;

		if (!(cb->flags & CBAN_ANTIFLOOD))
			continue;

		modestack_mode_param(chansvs.nick, c, MTYPE_DEL, cb->type, cb->mask);
		chanban_delete(cb);
	}
}

static void
antiflood_enforce_kickban(user_t *u, channel_t *c)
{
	chanban_t *cb;

	ban(chansvs.me->me, c, u);
	remove_ban_exceptions(chansvs.me->me, c, u);
	try_kick(chansvs.me->me, c, u, "Flooding");

	/* poison tail */
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
antiflood_enforce_kline(user_t *u, channel_t *c)
{
	kline_add_user(u, "Flooding", 86400, chansvs.nick);
	slog(LG_INFO, "ANTIFLOOD:ENFORCE:AKILL: \2%s!%s@%s\2 from \2%s\2", u->nick, u->user, u->vhost, c->name);
}

typedef struct {
	void (*enforce)(user_t *u, channel_t *c);
	void (*unenforce)(channel_t *c);
} antiflood_enforce_method_impl_t;

static antiflood_enforce_method_impl_t antiflood_enforce_methods[ANTIFLOOD_ENFORCE_COUNT] = {
	[ANTIFLOOD_ENFORCE_QUIET]   = { &antiflood_enforce_quiet, &antiflood_unenforce_banlike },
	[ANTIFLOOD_ENFORCE_KICKBAN] = { &antiflood_enforce_kickban, &antiflood_unenforce_banlike },
	[ANTIFLOOD_ENFORCE_KLINE]   = { &antiflood_enforce_kline },
};

static inline antiflood_enforce_method_impl_t *
antiflood_enforce_method_impl_get(mychan_t *mc)
{
	metadata_t *md;

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
	mychan_t *mc;

	MOWGLI_PATRICIA_FOREACH(mc, &state, mclist)
	{
		antiflood_enforce_method_impl_t *enf = antiflood_enforce_method_impl_get(mc);

		if (mc->chan == NULL)
			continue;

		if (enf->unenforce != NULL)
			enf->unenforce(mc->chan);
	}
}

static mowgli_eventloop_timer_t *antiflood_unenforce_timer = NULL;

static void
on_channel_message(hook_cmessage_data_t *data)
{
	chanuser_t *cu;
	mychan_t *mc;
	mqueue_t *mq;
	msg_t *msg;

	return_if_fail(data != NULL);
	return_if_fail(data->msg != NULL);
	return_if_fail(data->u != NULL);
	return_if_fail(data->c != NULL);

	cu = chanuser_find(data->c, data->u);
	if (cu == NULL)
		return;

	mc = MYCHAN_FROM(data->c);
	if (mc == NULL)
		return;

	mq = mqueue_get(mc);
	return_if_fail(mq != NULL);

	msg = msg_create(mq, data->u, data->msg);

	/* never enforce against any user who has special CSTATUS flags. */
	if (cu->modes)
		return;

	/* do not enforce unless enforcement is specifically enabled */
	if (!(mc->flags & MC_ANTIFLOOD))
		return;

	if (mqueue_should_enforce(mq) != MQ_ENFORCE_NONE)
	{
		antiflood_enforce_method_impl_t *enf = antiflood_enforce_method_impl_get(mc);

		if (enf == NULL || enf->enforce == NULL)
			return;

		enf->enforce(data->u, data->c);
	}
}

static void
on_channel_drop(mychan_t *mc)
{
	mqueue_t *mq;

	mq = mqueue_get(mc);
	return_if_fail(mq != NULL);

	mqueue_destroy(mq);
}

static void
cs_set_cmd_antiflood(sourceinfo_t *si, int parc, char *parv[])
{
	mychan_t *mc;

	if (!(mc = mychan_find(parv[0])))
	{
		command_fail(si, fault_nosuch_target, _("Channel \2%s\2 is not registered."), parv[0]);
		return;
	}

	/* allow opers with PRIV_CHAN_ADMIN to override this setting since it has
	   oper-specific settings (i.e. AKILL action) */ 
	if (!chanacs_source_has_flag(mc, si, CA_SET) && !has_priv(si, PRIV_CHAN_ADMIN))
	{
		command_fail(si, fault_noprivs, _("You are not authorized to perform this command."));
		return;
	}

	if (parv[1] == NULL)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SET FLOOD");
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
			command_fail(si, fault_noprivs, _("You are not authorized to perform this command."));
			return;
		}
	}
}

static command_t cs_set_antiflood = {
	"ANTIFLOOD", N_("Set anti-flood action"), AC_NONE, 2,
	cs_set_cmd_antiflood, { .path = "cservice/set_antiflood" }
};

mowgli_patricia_t **cs_set_cmdtree;

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

void
_modinit(module_t *m)
{
	MODULE_TRY_REQUEST_SYMBOL(m, cs_set_cmdtree, "chanserv/set_core", "cs_set_cmdtree");

	/* attempt to pull in the place_quietmask() routine from chanserv/quiet,
	   we don't see it as a hardfail because we can run without QUIET support. */
	if (module_request("chanserv/quiet"))
	{
		place_quietmask = module_locate_symbol("chanserv/quiet", "place_quietmask");
		if (place_quietmask == NULL)
			antiflood_enforce_method = ANTIFLOOD_ENFORCE_KICKBAN;
	}

	hook_add_event("channel_message");
	hook_add_channel_message(on_channel_message);

	hook_add_event("channel_drop");
	hook_add_channel_drop(on_channel_drop);

	msg_heap = sharedheap_get(sizeof(msg_t));

	mqueue_heap = sharedheap_get(sizeof(mqueue_t));
	mqueue_trie = mowgli_patricia_create(irccasecanon);
	mqueue_gc_timer = mowgli_timer_add(base_eventloop, "mqueue_gc", mqueue_gc, NULL, 300);

	antiflood_unenforce_timer = mowgli_timer_add(base_eventloop, "antiflood_unenforce", antiflood_unenforce_timer_cb, NULL, 3600);

	command_add(&cs_set_antiflood, *cs_set_cmdtree);

	add_conf_item("ANTIFLOOD_ENFORCE_METHOD", &chansvs.me->conf_table, c_ci_antiflood_enforce_method);
}

void
_moddeinit(module_unload_intent_t intent)
{
	command_delete(&cs_set_antiflood, *cs_set_cmdtree);

	hook_del_channel_message(on_channel_message);
	hook_del_channel_drop(on_channel_drop);

	mowgli_patricia_destroy(mqueue_trie, mqueue_trie_destroy_cb, NULL);
	mowgli_timer_destroy(base_eventloop, mqueue_gc_timer);
	mowgli_timer_destroy(base_eventloop, antiflood_unenforce_timer);

	del_conf_item("ANTIFLOOD_ENFORCE_METHOD", &chansvs.me->conf_table);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
