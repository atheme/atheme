/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005 William Pitcock, et al.
 *
 * This file contains code for the CService AKICK functions.
 */

#include <atheme.h>

struct akick_timeout
{
	time_t expiration;

	struct myentity *entity;
	struct mychan *chan;

	char host[NICKLEN + 1 + USERLEN + 1 + HOSTLEN + 1 + 4];

	mowgli_node_t node;
};

static time_t akickdel_next;
static mowgli_list_t akickdel_list;

static mowgli_heap_t *akick_timeout_heap = NULL;
static mowgli_patricia_t *cs_akick_cmds = NULL;
static mowgli_eventloop_timer_t *akick_timeout_check_timer = NULL;

static void
clear_bans_matching_entity(struct mychan *mc, struct myentity *mt)
{
	mowgli_node_t *n;
	struct myuser *tmu;

	if (mc->chan == NULL)
		return;

	if (!isuser(mt))
		return;

	tmu = user(mt);

	MOWGLI_ITER_FOREACH(n, tmu->logins.head)
	{
		struct user *tu;
		mowgli_node_t *it, *itn;

		char hostbuf2[BUFSIZE];

		tu = n->data;

		snprintf(hostbuf2, BUFSIZE, "%s!%s@%s", tu->nick, tu->user, tu->vhost);
		for (it = next_matching_ban(mc->chan, tu, 'b', mc->chan->bans.head); it != NULL; it = next_matching_ban(mc->chan, tu, 'b', itn))
		{
			struct chanban *cb;

			itn = it->next;
			cb = it->data;

			modestack_mode_param(chansvs.nick, mc->chan, MTYPE_DEL, cb->type, cb->mask);
			chanban_delete(cb);
		}
	}

	modestack_flush_channel(mc->chan);
}

static void
cs_cmd_akick(struct sourceinfo *const restrict si, const int parc, char **const restrict parv)
{
	if (parc < 2)
	{
		(void) command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "AKICK");
		(void) command_fail(si, fault_needmoreparams, _("Syntax: AKICK <#channel> <ADD|DEL|LIST> [parameters]"));
		return;
	}

	char *subcmd;
	char *target;

	if (parv[0][0] == '#')
	{
		subcmd = parv[1];
		target = parv[0];
	}
	else if (parv[1][0] == '#')
	{
		subcmd = parv[0];
		target = parv[1];
	}
	else
	{
		(void) command_fail(si, fault_badparams, STR_INVALID_PARAMS, "AKICK");
		(void) command_fail(si, fault_badparams, _("Syntax: AKICK <#channel> <ADD|DEL|LIST> [parameters]"));
		return;
	}

	parv[0] = subcmd;
	parv[1] = target;

	(void) subcommand_dispatch_simple(chansvs.me, si, parc, parv, cs_akick_cmds, "AKICK");
}

static struct akick_timeout *
akick_add_timeout(struct mychan *mc, struct myentity *mt, const char *host, time_t expireson)
{
	mowgli_node_t *n;
	struct akick_timeout *timeout, *timeout2;

	timeout = mowgli_heap_alloc(akick_timeout_heap);

	timeout->entity = mt;
	timeout->chan = mc;
	timeout->expiration = expireson;

	mowgli_strlcpy(timeout->host, host, sizeof timeout->host);

	MOWGLI_ITER_FOREACH_PREV(n, akickdel_list.tail)
	{
		timeout2 = n->data;
		if (timeout2->expiration <= timeout->expiration)
			break;
	}
	if (n == NULL)
		mowgli_node_add_head(timeout, &timeout->node, &akickdel_list);
	else if (n->next == NULL)
		mowgli_node_add(timeout, &timeout->node, &akickdel_list);
	else
		mowgli_node_add_before(timeout, &timeout->node, &akickdel_list, n->next);

	return timeout;
}

static void
akick_timeout_check(void *arg)
{
	mowgli_node_t *n, *tn;
	struct akick_timeout *timeout;
	struct chanacs *ca;
	struct mychan *mc;

	struct chanban *cb;
	akickdel_next = 0;

	MOWGLI_ITER_FOREACH_SAFE(n, tn, akickdel_list.head)
	{
		timeout = n->data;
		mc = timeout->chan;

		if (timeout->expiration > CURRTIME)
		{
			akickdel_next = timeout->expiration;
			akick_timeout_check_timer = mowgli_timer_add_once(base_eventloop, "akick_timeout_check", akick_timeout_check, NULL, akickdel_next - CURRTIME);
			break;
		}

		ca = NULL;

		if (timeout->entity == NULL)
		{
			if ((ca = chanacs_find_host_literal(mc, timeout->host, CA_AKICK)) && mc->chan != NULL && (cb = chanban_find(mc->chan, ca->host, 'b')))
			{
				modestack_mode_param(chansvs.nick, mc->chan, MTYPE_DEL, cb->type, cb->mask);
				chanban_delete(cb);
			}
		}
		else
		{
			ca = chanacs_find_literal(mc, timeout->entity, CA_AKICK);
			if (ca == NULL)
			{
				mowgli_node_delete(&timeout->node, &akickdel_list);
				mowgli_heap_free(akick_timeout_heap, timeout);

				continue;
			}

			clear_bans_matching_entity(mc, timeout->entity);
		}

		if (ca)
		{
			chanacs_modify_simple(ca, 0, CA_AKICK, NULL);
			chanacs_close(ca);
		}

		mowgli_node_delete(&timeout->node, &akickdel_list);
		mowgli_heap_free(akick_timeout_heap, timeout);
	}
}

static void
cs_cmd_akick_add(struct sourceinfo *si, int parc, char *parv[])
{
	struct myentity *mt;
	struct mychan *mc;
	struct hook_channel_acl_req req;
	struct chanacs *ca, *ca2;
	char *chan = parv[0];
	long duration;
	char expiry[512];
	char *s;
	char *target;
	char *uname;
	char *token;
	char *treason, reason[BUFSIZE];

	target = parv[1];
	token = strtok(parv[2], " ");

	if (!target)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "AKICK");
		command_fail(si, fault_needmoreparams, _("Syntax: AKICK <#channel> ADD <nickname|hostmask> [!P|!T <minutes>] [reason]"));
		return;
	}

	mc = mychan_find(chan);
	if (!mc)
	{
		command_fail(si, fault_nosuch_target, STR_IS_NOT_REGISTERED, chan);
		return;
	}

	if (metadata_find(mc, "private:close:closer"))
	{
		command_fail(si, fault_noprivs, STR_CHANNEL_IS_CLOSED, chan);
		return;
	}

	// A duration, reason or duration and reason.
	if (token)
	{
		if (!strcasecmp(token, "!P")) // A duration [permanent]
		{
			duration = 0;
			treason = strtok(NULL, "");

			if (treason)
				mowgli_strlcpy(reason, treason, BUFSIZE);
			else
				reason[0] = 0;
		}
		else if (!strcasecmp(token, "!T")) // A duration [temporary]
		{
			s = strtok(NULL, " ");
			treason = strtok(NULL, "");

			if (treason)
				mowgli_strlcpy(reason, treason, BUFSIZE);
			else
				reason[0] = 0;

			if (s)
			{
				duration = (atol(s) * SECONDS_PER_MINUTE);
				while (isdigit((unsigned char)*s))
					s++;
				if (*s == 'h' || *s == 'H')
					duration *= MINUTES_PER_HOUR;
				else if (*s == 'd' || *s == 'D')
					duration *= MINUTES_PER_DAY;
				else if (*s == 'w' || *s == 'W')
					duration *= MINUTES_PER_WEEK;
				else if (*s == '\0')
					;
				else
					duration = 0;

				if (duration == 0)
				{
					command_fail(si, fault_badparams, _("Invalid duration given."));
					command_fail(si, fault_badparams, _("Syntax: AKICK <#channel> ADD <nick|hostmask> [!P|!T <minutes>] [reason]"));
					return;
				}
			}
			else
			{
				command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "AKICK ADD");
				command_fail(si, fault_needmoreparams, _("Syntax: AKICK <#channel> ADD <nick|hostmask> [!P|!T <minutes>] [reason]"));
				return;
			}
		}
		else
		{
			duration = chansvs.akick_time;
			mowgli_strlcpy(reason, token, BUFSIZE);
			treason = strtok(NULL, "");

			if (treason)
			{
				mowgli_strlcat(reason, " ", BUFSIZE);
				mowgli_strlcat(reason, treason, BUFSIZE);
			}
		}
	}
	else
	{
		// No reason and no duration
		duration = chansvs.akick_time;
		reason[0] = 0;
	}

	if ((chanacs_source_flags(mc, si) & (CA_FLAGS | CA_REMOVE)) != (CA_FLAGS | CA_REMOVE))
	{
		command_fail(si, fault_noprivs, STR_NOT_AUTHORIZED);
		return;
	}

	mt = myentity_find_ext(target);
	if (!mt)
	{
		uname = pretty_mask(target);
		if (uname == NULL)
			uname = target;

		// we might be adding a hostmask
		if (!validhostmask(uname))
		{
			command_fail(si, fault_badparams, _("\2%s\2 is neither a nickname nor a hostmask."), uname);
			return;
		}

		uname = collapse(uname);

		ca = chanacs_find_host_literal(mc, uname, 0);
		if (ca != NULL)
		{
			if (ca->level & CA_AKICK)
				command_fail(si, fault_nochange, _("\2%s\2 is already on the AKICK list for \2%s\2"), uname, mc->name);
			else
				command_fail(si, fault_alreadyexists, _("\2%s\2 already has flags \2%s\2 on \2%s\2"), uname, bitmask_to_flags(ca->level), mc->name);
			return;
		}

		ca = chanacs_find_host(mc, uname, CA_AKICK);
		if (ca != NULL)
		{
			command_fail(si, fault_nochange, _("The more general mask \2%s\2 is already on the AKICK list for \2%s\2"), ca->host, mc->name);
			return;
		}

		// new entry
		ca2 = chanacs_open(mc, NULL, uname, true, entity(si->smu));
		if (chanacs_is_table_full(ca2))
		{
			command_fail(si, fault_toomany, _("Channel \2%s\2 access list is full."), mc->name);
			chanacs_close(ca2);
			return;
		}

		req.ca = ca2;
		req.oldlevel = ca2->level;

		chanacs_modify_simple(ca2, CA_AKICK, 0, si->smu);

		req.newlevel = ca2->level;

		if (reason[0])
			metadata_add(ca2, "reason", reason);

		if (duration > 0)
		{
			struct akick_timeout *timeout;
			time_t expireson = ca2->tmodified+duration;

			snprintf(expiry, sizeof expiry, "%ld", expireson);
			metadata_add(ca2, "expires", expiry);

			verbose(mc, "\2%s\2 added \2%s\2 to the AKICK list, expires in %s.", get_source_name(si), uname,timediff(duration));
			logcommand(si, CMDLOG_SET, "AKICK:ADD: \2%s\2 on \2%s\2, expires in %s.", uname, mc->name,timediff(duration));
			command_success_nodata(si, _("AKICK on \2%s\2 was successfully added for \2%s\2 and will expire in %s."), uname, mc->name,timediff(duration) );

			timeout = akick_add_timeout(mc, NULL, uname, expireson);

			if (akickdel_next == 0 || akickdel_next > timeout->expiration)
			{
				if (akickdel_next != 0)
					mowgli_timer_destroy(base_eventloop, akick_timeout_check_timer);

				akickdel_next = timeout->expiration;
				akick_timeout_check_timer = mowgli_timer_add_once(base_eventloop, "akick_timeout_check", akick_timeout_check, NULL, akickdel_next - CURRTIME);
			}
		}
		else
		{
			verbose(mc, "\2%s\2 added \2%s\2 to the AKICK list.", get_source_name(si), uname);
			logcommand(si, CMDLOG_SET, "AKICK:ADD: \2%s\2 on \2%s\2", uname, mc->name);

			command_success_nodata(si, _("AKICK on \2%s\2 was successfully added to the AKICK list for \2%s\2."), uname, mc->name);
		}

		hook_call_channel_acl_change(&req);
		chanacs_close(ca2);

		return;
	}
	else
	{
		if ((ca = chanacs_find_literal(mc, mt, 0x0)))
		{
			if (ca->level & CA_AKICK)
				command_fail(si, fault_nochange, _("\2%s\2 is already on the AKICK list for \2%s\2"), mt->name, mc->name);
			else
				command_fail(si, fault_alreadyexists, _("\2%s\2 already has flags \2%s\2 on \2%s\2"), mt->name, bitmask_to_flags(ca->level), mc->name);
			return;
		}

		// new entry
		ca2 = chanacs_open(mc, mt, NULL, true, entity(si->smu));
		if (chanacs_is_table_full(ca2))
		{
			command_fail(si, fault_toomany, _("Channel \2%s\2 access list is full."), mc->name);
			chanacs_close(ca2);
			return;
		}

		req.ca = ca2;
		req.oldlevel = ca2->level;

		chanacs_modify_simple(ca2, CA_AKICK, 0, si->smu);

		req.newlevel = ca2->level;

		if (reason[0])
			metadata_add(ca2, "reason", reason);

		if (duration > 0)
		{
			struct akick_timeout *timeout;
			time_t expireson = ca2->tmodified+duration;

			snprintf(expiry, sizeof expiry, "%ld", expireson);
			metadata_add(ca2, "expires", expiry);

			command_success_nodata(si, _("AKICK on \2%s\2 was successfully added for \2%s\2 and will expire in %s."), mt->name, mc->name, timediff(duration));
			verbose(mc, "\2%s\2 added \2%s\2 to the AKICK list, expires in %s.", get_source_name(si), mt->name, timediff(duration));
			logcommand(si, CMDLOG_SET, "AKICK:ADD: \2%s\2 on \2%s\2, expires in %s", mt->name, mc->name, timediff(duration));

			timeout = akick_add_timeout(mc, mt, mt->name, expireson);

			if (akickdel_next == 0 || akickdel_next > timeout->expiration)
			{
				if (akickdel_next != 0)
					mowgli_timer_destroy(base_eventloop, akick_timeout_check_timer);

				akickdel_next = timeout->expiration;
				akick_timeout_check_timer = mowgli_timer_add_once(base_eventloop, "akick_timeout_check", akick_timeout_check, NULL, akickdel_next - CURRTIME);
			}
		}
		else
		{
			command_success_nodata(si, _("AKICK on \2%s\2 was successfully added to the AKICK list for \2%s\2."), mt->name, mc->name);

			verbose(mc, "\2%s\2 added \2%s\2 to the AKICK list.", get_source_name(si), mt->name);
			logcommand(si, CMDLOG_SET, "AKICK:ADD: \2%s\2 on \2%s\2", mt->name, mc->name);
		}

		hook_call_channel_acl_change(&req);
		chanacs_close(ca2);
		return;
	}
}

static void
cs_cmd_akick_del(struct sourceinfo *si, int parc, char *parv[])
{
	struct myentity *mt;
	struct mychan *mc;
	struct hook_channel_acl_req req;
	struct chanacs *ca;
	mowgli_node_t *n, *tn;
	char *chan = parv[0];
	char *uname = parv[1];

	if (!chan || !uname)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "AKICK");
		command_fail(si, fault_needmoreparams, _("Syntax: AKICK <#channel> DEL <nickname|hostmask>"));
		return;
	}

	mc = mychan_find(chan);
	if (!mc)
	{
		command_fail(si, fault_nosuch_target, STR_IS_NOT_REGISTERED, chan);
		return;
	}

	struct akick_timeout *timeout;
	struct chanban *cb;

	if ((chanacs_source_flags(mc, si) & (CA_FLAGS | CA_REMOVE)) != (CA_FLAGS | CA_REMOVE))
	{
		command_fail(si, fault_noprivs, STR_NOT_AUTHORIZED);
		return;
	}

	if (metadata_find(mc, "private:close:closer"))
	{
		command_fail(si, fault_noprivs, STR_CHANNEL_IS_CLOSED, chan);
		return;
	}

	mt = myentity_find_ext(uname);
	if (!mt)
	{
		// we might be deleting a hostmask
		ca = chanacs_find_host_literal(mc, uname, CA_AKICK);
		if (ca == NULL)
		{
			ca = chanacs_find_host(mc, uname, CA_AKICK);
			if (ca != NULL)
				command_fail(si, fault_nosuch_key, _("\2%s\2 is not on the AKICK list for \2%s\2, however \2%s\2 is."), uname, mc->name, ca->host);
			else
				command_fail(si, fault_nosuch_key, _("\2%s\2 is not on the AKICK list for \2%s\2."), uname, mc->name);
			return;
		}

		req.ca = ca;
		req.oldlevel = ca->level;

		chanacs_modify_simple(ca, 0, CA_AKICK, si->smu);

		req.newlevel = ca->level;

		hook_call_channel_acl_change(&req);
		chanacs_close(ca);

		verbose(mc, "\2%s\2 removed \2%s\2 from the AKICK list.", get_source_name(si), uname);
		logcommand(si, CMDLOG_SET, "AKICK:DEL: \2%s\2 on \2%s\2", uname, mc->name);
		command_success_nodata(si, _("\2%s\2 has been removed from the AKICK list for \2%s\2."), uname, mc->name);

		MOWGLI_ITER_FOREACH_SAFE(n, tn, akickdel_list.head)
		{
			timeout = n->data;
			if (!match(timeout->host, uname) && timeout->chan == mc)
			{
				mowgli_node_delete(&timeout->node, &akickdel_list);
				mowgli_heap_free(akick_timeout_heap, timeout);
			}
		}

		if (mc->chan != NULL && (cb = chanban_find(mc->chan, uname, 'b')))
		{
			modestack_mode_param(chansvs.nick, mc->chan, MTYPE_DEL, cb->type, cb->mask);
			chanban_delete(cb);
		}

		return;
	}

	if (!(ca = chanacs_find_literal(mc, mt, CA_AKICK)))
	{
		command_fail(si, fault_nosuch_key, _("\2%s\2 is not on the AKICK list for \2%s\2."), mt->name, mc->name);
		return;
	}

	clear_bans_matching_entity(mc, mt);

	MOWGLI_ITER_FOREACH_SAFE(n, tn, akickdel_list.head)
	{
		timeout = n->data;
		if (timeout->entity == mt && timeout->chan == mc)
		{
			mowgli_node_delete(&timeout->node, &akickdel_list);
			mowgli_heap_free(akick_timeout_heap, timeout);
		}
	}

	req.ca = ca;
	req.oldlevel = ca->level;

	chanacs_modify_simple(ca, 0, CA_AKICK, si->smu);

	req.newlevel = ca->level;

	hook_call_channel_acl_change(&req);
	chanacs_close(ca);

	command_success_nodata(si, _("\2%s\2 has been removed from the AKICK list for \2%s\2."), mt->name, mc->name);
	logcommand(si, CMDLOG_SET, "AKICK:DEL: \2%s\2 on \2%s\2", mt->name, mc->name);
	verbose(mc, "\2%s\2 removed \2%s\2 from the AKICK list.", get_source_name(si), mt->name);

	return;
}

static void
cs_cmd_akick_list(struct sourceinfo *si, int parc, char *parv[])
{
	struct mychan *mc;
	struct chanacs *ca;
	struct metadata *md, *md2;
	mowgli_node_t *n, *tn;
	bool operoverride = false;
	char *chan = parv[0];
	char expiry[512];

	if (!chan)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "AKICK");
		command_fail(si, fault_needmoreparams, _("Syntax: AKICK <#channel> LIST"));
		return;
	}

	/* make sure they're registered, logged in
	 * and the founder of the channel before
	 * we go any further.
	 */
	if (!si->smu)
	{
		// if they're opers and just want to LIST, they don't have to log in
		if (!(has_priv(si, PRIV_CHAN_AUSPEX)))
		{
			command_fail(si, fault_noprivs, STR_NOT_LOGGED_IN);
			return;
		}
	}

	mc = mychan_find(chan);
	if (!mc)
	{
		command_fail(si, fault_nosuch_target, STR_IS_NOT_REGISTERED, chan);
		return;
	}

	if (metadata_find(mc, "private:close:closer"))
	{
		command_fail(si, fault_noprivs, STR_CHANNEL_IS_CLOSED, chan);
		return;
	}

	unsigned int i = 0;

	if (!chanacs_source_has_flag(mc, si, CA_ACLVIEW))
	{
		if (has_priv(si, PRIV_CHAN_AUSPEX))
			operoverride = true;
		else
		{
			command_fail(si, fault_noprivs, STR_NOT_AUTHORIZED);
			return;
		}
	}
	command_success_nodata(si, _("AKICK list for \2%s\2:"), mc->name);

	MOWGLI_ITER_FOREACH_SAFE(n, tn, mc->chanacs.head)
	{
		time_t expires_on = 0;
		char *ago;
		long time_left = 0;

		ca = (struct chanacs *)n->data;

		if (ca->level == CA_AKICK)
		{
			char buf[BUFSIZE], *buf_iter;
			struct myentity *setter = NULL;

			md = metadata_find(ca, "reason");

			// check if it's a temporary akick
			if ((md2 = metadata_find(ca, "expires")))
			{
				snprintf(expiry, sizeof expiry, "%s", md2->value);
				expires_on = (time_t)atol(expiry);
				time_left = difftime(expires_on, CURRTIME);
			}
			ago = ca->tmodified ? time_ago(ca->tmodified) : "?";

			buf_iter = buf;
			buf_iter += snprintf(buf_iter, sizeof(buf) - (buf_iter - buf), "%u: \2%s\2 (\2%s\2) [",
					     ++i, ca->entity != NULL ? ca->entity->name : ca->host,
					     md != NULL ? md->value : _("no AKICK reason specified"));

			if (*ca->setter_uid != '\0' && (setter = myentity_find_uid(ca->setter_uid)))
				buf_iter += snprintf(buf_iter, sizeof(buf) - (buf_iter - buf), _("setter: %s"),
						     setter->name);

			if (expires_on > 0)
				buf_iter += snprintf(buf_iter, sizeof(buf) - (buf_iter - buf), _("%sexpires: %s"),
						     setter != NULL ? ", " : "", timediff(time_left));

			if (ca->tmodified)
				buf_iter += snprintf(buf_iter, sizeof(buf) - (buf_iter - buf), _("%smodified: %s"),
						     expires_on > 0 || setter != NULL ? ", " : "", ago);

			mowgli_strlcat(buf, "]", sizeof buf);

			command_success_nodata(si, "%s", buf);
		}

	}

	command_success_nodata(si, _("Total of \2%u\2 entries in \2%s\2's AKICK list."), i, mc->name);

	if (operoverride)
		logcommand(si, CMDLOG_ADMIN, "AKICK:LIST: \2%s\2 (oper override)", mc->name);
	else
		logcommand(si, CMDLOG_GET, "AKICK:LIST: \2%s\2", mc->name);
}

static void
akickdel_list_create(void *arg)
{
	struct mychan *mc;
	mowgli_node_t *n, *tn;
	struct chanacs *ca;
	struct metadata *md;
	time_t expireson;

	mowgli_patricia_iteration_state_t state;

	MOWGLI_PATRICIA_FOREACH(mc, &state, mclist)
	{
		MOWGLI_ITER_FOREACH_SAFE(n, tn, mc->chanacs.head)
		{
			ca = (struct chanacs *)n->data;

			if (!(ca->level & CA_AKICK))
				continue;

			md = metadata_find(ca, "expires");

			if (!md)
				continue;

			expireson = atol(md->value);

			if (CURRTIME > expireson)
			{
				chanacs_modify_simple(ca, 0, CA_AKICK, NULL);
				chanacs_close(ca);
			}
			else
			{
				// overcomplicate the logic here a tiny bit
				if (ca->host == NULL && ca->entity != NULL)
					akick_add_timeout(mc, ca->entity, entity(ca->entity)->name, expireson);
				else if (ca->host != NULL && ca->entity == NULL)
					akick_add_timeout(mc, NULL, ca->host, expireson);
			}
		}
	}
}

static void
chanuser_sync(struct hook_chanuser_sync *hdata)
{
	struct chanuser *cu    = hdata->cu;
	unsigned int     flags = hdata->flags;

	if (!cu)
		return;

	struct channel *chan = cu->chan;
	struct user    *u    = cu->user;
	struct mychan  *mc   = chan->mychan;

	return_if_fail(mc != NULL);

	if (flags & CA_AKICK && !(flags & CA_EXEMPT))
	{
		// Stay on channel if this would empty it -- jilles
		if (chan->nummembers - chan->numsvcmembers == 1)
		{
			mc->flags |= MC_INHABIT;
			if (chan->numsvcmembers == 0)
				join(chan->name, chansvs.nick);
		}

		// use a user-given ban mask if possible -- jilles
		struct chanacs *ca = chanacs_find_host_by_user(mc, u, CA_AKICK);
		if (ca != NULL)
		{
			if (chanban_find(chan, ca->host, 'b') == NULL)
			{
				chanban_add(chan, ca->host, 'b');
				modestack_mode_param(chansvs.nick, chan, MTYPE_ADD, 'b', ca->host);
				modestack_flush_channel(chan);
			}
		}
		else
		{
			// XXX this could be done more efficiently
			ca = chanacs_find(mc, entity(u->myuser), CA_AKICK);
			ban(chansvs.me->me, chan, u);
		}
		remove_ban_exceptions(chansvs.me->me, chan, u);

		char akickreason[120] = "User is banned from this channel", *p;

		if (ca != NULL)
		{
			struct metadata *md = metadata_find(ca, "reason");
			if (md != NULL && *md->value != '|')
			{
				snprintf(akickreason, sizeof akickreason,
						"Banned: %s", md->value);
				p = strchr(akickreason, '|');
				if (p != NULL)
					*p = '\0';
				else
					p = akickreason + strlen(akickreason);
				/* strip trailing spaces, so as not to
				 * disclose the existence of an oper reason */
				p--;
				while (p > akickreason && *p == ' ')
					p--;
				p[1] = '\0';
			}
		}
		if (try_kick(chansvs.me->me, chan, u, akickreason))
		{
			hdata->cu = NULL;
			return;
		}
	}
}

static struct command cs_akick = {
	.name           = "AKICK",
	.desc           = N_("Manipulates a channel's AKICK list."),
	.access         = AC_NONE,
	.maxparc        = 4,
	.cmd            = &cs_cmd_akick,
	.help           = { .path = "cservice/akick" },
};

static struct command cs_akick_add = {
	.name           = "ADD",
	.desc           = N_("Adds a channel AKICK."),
	.access         = AC_NONE,
	.maxparc        = 4,
	.cmd            = &cs_cmd_akick_add,
	.help           = { .path = "" },
};

static struct command cs_akick_del = {
	.name           = "DEL",
	.desc           = N_("Deletes a channel AKICK."),
	.access         = AC_NONE,
	.maxparc        = 3,
	.cmd            = &cs_cmd_akick_del,
	.help           = { .path = "" },
};

static struct command cs_akick_list = {
	.name           = "LIST",
	.desc           = N_("Displays a channel's AKICK list."),
	.access         = AC_NONE,
	.maxparc        = 2,
	.cmd            = &cs_cmd_akick_list,
	.help           = { .path = "" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "chanserv/main")

	if (! (akick_timeout_heap = mowgli_heap_create(sizeof(struct akick_timeout), 512, BH_NOW)))
	{
		(void) slog(LG_ERROR, "%s: mowgli_heap_create() failed", m->name);

		m->mflags |= MODFLAG_FAIL;
		return;
	}

	if (! (cs_akick_cmds = mowgli_patricia_create(&strcasecanon)))
	{
		(void) slog(LG_ERROR, "%s: mowgli_patricia_create() failed", m->name);

		(void) mowgli_heap_destroy(akick_timeout_heap);

		m->mflags |= MODFLAG_FAIL;
		return;
	}

	(void) command_add(&cs_akick_add, cs_akick_cmds);
	(void) command_add(&cs_akick_del, cs_akick_cmds);
	(void) command_add(&cs_akick_list, cs_akick_cmds);

	(void) service_named_bind_command("chanserv", &cs_akick);

	(void) mowgli_timer_add_once(base_eventloop, "akickdel_list_create", &akickdel_list_create, NULL, 0);

	(void) hook_add_first_chanuser_sync(&chanuser_sync);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	if (akick_timeout_check_timer)
		(void) mowgli_timer_destroy(base_eventloop, akick_timeout_check_timer);

	(void) hook_del_chanuser_sync(&chanuser_sync);

	(void) service_named_unbind_command("chanserv", &cs_akick);

	(void) mowgli_patricia_destroy(cs_akick_cmds, &command_delete_trie_cb, cs_akick_cmds);

	(void) mowgli_heap_destroy(akick_timeout_heap);
}

SIMPLE_DECLARE_MODULE_V1("chanserv/akick", MODULE_UNLOAD_CAPABILITY_OK)
