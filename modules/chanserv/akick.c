/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005 William Pitcock, et al.
 *
 * This file contains code for the CService AKICK functions.
 */

#include <atheme.h>

#define DB_TYPE "AK"
#define PRIVDATA_NAME "akicks"

struct akick
{
	struct myentity *       entity;
	struct mychan *         mychan;
	char *                  host;
	char *                  reason;
	unsigned int            number;
	time_t                  tmodified;
	time_t                  expires;
	char                    setter_uid[IDLEN + 1];

	mowgli_node_t list_node;
	mowgli_node_t timeout_node;
};


static mowgli_heap_t *akick_heap = NULL;
static mowgli_patricia_t *cs_akick_cmds = NULL;

static time_t next_akick_timeout;
static mowgli_list_t akick_timeout_list;
static mowgli_eventloop_timer_t *akick_timeout_check_timer = NULL;

// Retrieves the list of akicks set on a channel, optionally allocating one
// if it does not yet exist.
static mowgli_list_t *
channel_get_akicks(struct mychan *mc, bool create)
{
	mowgli_list_t *l;

	l = privatedata_get(mc, PRIVDATA_NAME);
	if (l)
		return l;

	if (create)
	{
		l = mowgli_list_create();
		privatedata_set(mc, PRIVDATA_NAME, l);

		return l;
	}
	else
	{
		return NULL;
	}
}

// Free memory allocated for an akick and remove references to it from containing lists.
static void
akick_destroy(struct akick *ak)
{
	sfree(ak->reason);
	sfree(ak->host);

	mowgli_list_t *list = channel_get_akicks(ak->mychan, false);
	mowgli_node_delete(&ak->list_node, list);

	if (ak->expires)
		mowgli_node_delete(&ak->timeout_node, &akick_timeout_list);

	mowgli_heap_free(akick_heap, ak);
}

// Given a channel, find an akick against the given entity.
// If 'literal' is set, the akick must be set against the literal entity given;
// otherwise, any akick matching the given entity is returned.
static struct akick *
akick_find_entity(struct mychan *mc, struct myentity *mt, bool literal)
{
	mowgli_list_t *akick_list = channel_get_akicks(mc, false);

	if (akick_list)
	{
		mowgli_node_t *n;

		MOWGLI_ITER_FOREACH(n, akick_list->head)
		{
			struct akick *ak = n->data;

			if (!ak->entity)
				continue;

			if (literal)
			{
				if (ak->entity == mt)
					return ak;
			}
			else
			{
				const struct entity_vtable *vt = myentity_get_vtable(ak->entity);
				if (vt->match_entity(ak->entity, mt))
					return ak;
			}
		}
	}

	return NULL;
}

// Given a channel, find an akick against the given hostmask.
// If 'literal' is set, the akick must be set against the literal hostmask given;
// otherwise, any akick matching the given hostmask is returned.
static struct akick *
akick_find_host(struct mychan *mc, const char *host, bool literal)
{
	mowgli_list_t *akick_list = channel_get_akicks(mc, false);

	if (akick_list)
	{
		mowgli_node_t *n;

		MOWGLI_ITER_FOREACH(n, akick_list->head)
		{
			struct akick *ak = n->data;

			if (!ak->host)
				continue;

			if (literal)
			{
				if (!strcmp(ak->host, host))
					return ak;
			}
			else
			{
				if (!match(ak->host, host))
					return ak;
			}
		}
	}

	return NULL;
}

// Given a channel, find an akick against the given user. Tries to return
// a hostmask-based akick if possible.
static struct akick *
akick_find_by_user(struct mychan *mc, struct user *u)
{
	mowgli_list_t *akick_list = channel_get_akicks(mc, false);

	if (akick_list)
	{
		mowgli_node_t *n;
		// We try our best to return a hostmask-based akick to respect the user-specified
		// ban mask, especially since our auto-generated account ban masks are far from clever.
		// This reflects pre-7.3 akick behaviour as well.
		// Maybe entity vtables should include a way to indicate a preferred ban mask
		// so we can use e.g. account extbans on ircds that support them instead
		struct akick *best = NULL;

		MOWGLI_ITER_FOREACH(n, akick_list->head)
		{
			struct akick *ak = n->data;

			if (ak->host)
			{
				if (mask_matches_user(ak->host, u))
					return ak;
			}
			else // ak->entity
			{
				const struct entity_vtable *vt = myentity_get_vtable(ak->entity);

				if (vt->match_user && vt->match_user(ak->entity, u))
					best = ak;
				else if (u->myuser && vt->match_entity(ak->entity, entity(u->myuser)))
					best = ak;
			}
		}

		return best;
	}

	return NULL;
}

// Enforce an akick against a given chanuser, typically placing a ban
// and trying to kick the chanuser.
// Returns whether the user was actually kicked.
// This is only responsible for handling the actual kick/ban. Whether
// the user should be affected, including checking if the akick matches
// and testing for CA_EXEMPT, is handled by the caller.
static bool
akick_enforce(struct akick *ak, struct chanuser *cu)
{
	struct channel *chan = cu->chan;
	struct user    *u    = cu->user;

	return_val_if_fail(chan->mychan != NULL, false);

	// Stay on channel if this would empty it -- jilles
	if (chan->nummembers - chan->numsvcmembers == 1)
	{
		chan->mychan->flags |= MC_INHABIT;
		if (chan->numsvcmembers == 0)
			join(chan->name, chansvs.nick);
	}

	if (ak->host)
	{
		if (chanban_find(chan, ak->host, 'b') == NULL)
		{
			chanban_add(chan, ak->host, 'b');
			modestack_mode_param(chansvs.nick, chan, MTYPE_ADD, 'b', ak->host);
			modestack_flush_channel(chan);
		}
	}
	else
	{
		ban(chansvs.me->me, chan, u);
	}
	remove_ban_exceptions(chansvs.me->me, chan, u);

	char akickreason[120] = "User is banned from this channel";

	// if reason starts with a | it only has the private reason;
	// don't disclose its existence
	if (ak->reason && ak->reason[0] != '|')
	{
		snprintf(akickreason, sizeof akickreason,
				"Banned: %s", ak->reason);
		char *p = strchr(akickreason, '|');
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

	return try_kick(chansvs.me->me, chan, u, akickreason);
}

// Hook function; given a chanuser and applicable flags, enforces a matching akick
// (if any) unless CA_EXEMPT is held.
static void
chanuser_sync(struct hook_chanuser_sync *hdata)
{
	struct chanuser *cu    = hdata->cu;
	unsigned int     flags = hdata->flags;

	if (!cu)
		return;

	if (flags & CA_EXEMPT)
		return;

	struct channel *chan = cu->chan;
	struct user    *u    = cu->user;
	struct mychan  *mc   = chan->mychan;

	return_if_fail(mc != NULL);

	struct akick *ak = akick_find_by_user(mc, u);

	if (ak)
	{
		if (akick_enforce(ak, cu))
			hdata->cu = NULL;
	}
}

// Given an akick, enforces it against any matching chanusers given
// they don't hold CA_EXEMPT.
static void
akick_sync(struct akick *ak)
{
	struct mychan *mc = ak->mychan;
	if (mc->chan == NULL)
		return;

	mowgli_node_t *n, *tn;
	MOWGLI_ITER_FOREACH_SAFE(n, tn, mc->chan->members.head)
	{
		struct chanuser *cu = (struct chanuser *)n->data;

		if (chanacs_user_flags(cu->chan->mychan, cu->user) & CA_EXEMPT)
			continue;

		if (ak->host)
		{
			if (mask_matches_user(ak->host, cu->user))
				akick_enforce(ak, cu);
		}
		else // ak->entity
		{
			const struct entity_vtable *vt = myentity_get_vtable(ak->entity);

			if (vt->match_user && vt->match_user(ak->entity, cu->user))
				akick_enforce(ak, cu);
			else if (cu->user->myuser && vt->match_entity(ak->entity, entity(cu->user->myuser)))
				akick_enforce(ak, cu);
		}
	}
}

// Utility function to remove bans set against an entity.
// Works on a best-effort basis, removing bans matching any user logged in
// and only processing account-type entities.
static void
clear_bans_matching_entity(struct channel *chan, struct myentity *mt)
{
	return_if_fail(chan != NULL);

	if (!isuser(mt))
		return;

	struct myuser *tmu = user(mt);

	mowgli_node_t *n;

	MOWGLI_ITER_FOREACH(n, tmu->logins.head)
	{
		struct user *tu = n->data;

		char hostbuf2[BUFSIZE];
		snprintf(hostbuf2, BUFSIZE, "%s!%s@%s", tu->nick, tu->user, tu->vhost);

		mowgli_node_t *it, *itn;
		for (it = next_matching_ban(chan, tu, 'b', chan->bans.head); it != NULL; it = next_matching_ban(chan, tu, 'b', itn))
		{
			itn = it->next;
			struct chanban *cb = it->data;

			modestack_mode_param(chansvs.nick, chan, MTYPE_DEL, cb->type, cb->mask);
			chanban_delete(cb);
		}
	}

	modestack_flush_channel(chan);
}

// Called by the akick timer to process expired akicks, if any,
// and update the timer as necessary.
static void
akick_timeout_check(void *arg)
{
	next_akick_timeout = 0;

	slog(LG_DEBUG, "akick_timeout_check(): processing expiries at instant %llu", (unsigned long long)CURRTIME);

	mowgli_node_t *n, *tn;

	MOWGLI_ITER_FOREACH_SAFE(n, tn, akick_timeout_list.head)
	{
		struct akick *ak = n->data;

		if (ak->expires > CURRTIME)
		{
			slog(LG_DEBUG, "akick_timeout_check(): next akick expires at instant %llu, updating timer", (unsigned long long)ak->expires);
			next_akick_timeout = ak->expires;
			akick_timeout_check_timer = mowgli_timer_add_once(base_eventloop, "akick_timeout_check", akick_timeout_check, NULL, next_akick_timeout - CURRTIME);
			break;
		}

		slog(LG_DEBUG, "akick_timeout_check(): akick expired at instant %llu, removing", (unsigned long long)ak->expires);

		struct channel *chan = ak->mychan->chan;

		if (chan)
		{
			if (ak->host)
			{
				struct chanban *cb = chanban_find(chan, ak->host, 'b');
				if (cb)
				{
					modestack_mode_param(chansvs.nick, chan, MTYPE_DEL, cb->type, cb->mask);
					chanban_delete(cb);
				}
			}
			else
			{
				clear_bans_matching_entity(chan, ak->entity);
			}
		}

		akick_destroy(ak);
	}
}

// Given a timed akick, this function takes care of any necessary bookkeeping
// with the expiry list and timer.
static void
akick_add_timeout(struct akick *ak)
{
	return_if_fail(ak->expires != 0);

	mowgli_node_t *n;

	MOWGLI_ITER_FOREACH_PREV(n, akick_timeout_list.tail)
	{
		struct akick *next = n->data;
		if (next->expires <= ak->expires)
			break;
	}

	if (n == NULL)
		mowgli_node_add_head(ak, &ak->timeout_node, &akick_timeout_list);
	else if (n->next == NULL)
		mowgli_node_add(ak, &ak->timeout_node, &akick_timeout_list);
	else
		mowgli_node_add_before(ak, &ak->timeout_node, &akick_timeout_list, n->next);

	if (next_akick_timeout == 0 || next_akick_timeout > ak->expires)
	{
		if (next_akick_timeout != 0)
			mowgli_timer_destroy(base_eventloop, akick_timeout_check_timer);

		next_akick_timeout = ak->expires;
		akick_timeout_check_timer = mowgli_timer_add_once(base_eventloop, "akick_timeout_check", akick_timeout_check, NULL, next_akick_timeout - CURRTIME);
	}
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

static void
cs_cmd_akick_add(struct sourceinfo *si, int parc, char *parv[])
{
	struct hook_channel_acl_req req;
	struct chanacs *ca, *ca2;

	char *chan = parv[0];
	char *target = parv[1];
	char *token = strtok(parv[2], " ");

	if (!target)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "AKICK");
		command_fail(si, fault_needmoreparams, _("Syntax: AKICK <#channel> ADD <nickname|hostmask> [!P|!T <minutes>] [reason]"));
		return;
	}

	struct mychan *mc = mychan_find(chan);
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

	long duration;
	char reason[BUFSIZE];

	// A duration, reason or duration and reason.
	if (token)
	{
		if (!strcasecmp(token, "!P")) // A duration [permanent]
		{
			duration = 0;
			const char *treason = strtok(NULL, "");

			if (treason)
				mowgli_strlcpy(reason, treason, BUFSIZE);
			else
				reason[0] = 0;
		}
		else if (!strcasecmp(token, "!T")) // A duration [temporary]
		{
			const char *s = strtok(NULL, " ");
			const char *treason = strtok(NULL, "");

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
			const char *treason = strtok(NULL, "");

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

	// Check if we're adding an entity or a hostmask
	struct myentity *mt = myentity_find_ext(target);
	char *hostmask = NULL;
	if (mt)
	{
		if (akick_find_entity(mc, mt, true))
		{
			command_fail(si, fault_nochange, _("\2%s\2 is already on the AKICK list for \2%s\2"), mt->name, mc->name);
			return;
		}
	}
	else
	{
		// The hostmask case is more complicated than it should be, unfortunately
		hostmask = pretty_mask(target);
		if (hostmask == NULL)
			hostmask = target;

		if (!validhostmask(hostmask))
		{
			command_fail(si, fault_badparams, _("\2%s\2 is neither a nickname nor a hostmask."), hostmask);
			return;
		}

		hostmask = collapse(hostmask);

		struct akick *existing = akick_find_host(mc, hostmask, true);

		if (existing)
		{
			command_fail(si, fault_nochange, _("\2%s\2 is already on the AKICK list for \2%s\2"), hostmask, mc->name);
			return;
		}

		existing = akick_find_host(mc, hostmask, false);
		if (existing)
		{
			command_fail(si, fault_nochange, _("The more general mask \2%s\2 is already on the AKICK list for \2%s\2"), existing->host, mc->name);
			return;
		}
	}

	// Once we got here, we know we're all set to add an akick on either an entity or a hostmask
	// TODO: check if list is full

	mowgli_list_t *akick_list = channel_get_akicks(mc, true);

	unsigned int next_number = 1;

	// For assigning akick IDs, the following rules apply:
	// - The first akick gets an ID of 1.
	// - A given akick maintains its ID over its entire lifetime, until removed.
	// - IDs reflect the order of akick creation.
	//
	// With the current scheme, we do not guarantee that an akick ID will not be reused;
	// in fact, removing the highest-numbered akick will guarantee reuse of IDs.
	if (akick_list && akick_list->tail)
		next_number = ((struct akick*)akick_list->tail->data)->number + 1;

	struct akick *ak = mowgli_heap_alloc(akick_heap);
	ak->mychan    = mc;
	ak->number    = next_number;
	ak->tmodified = CURRTIME;
	mowgli_strlcpy(ak->setter_uid, entity(si->smu)->id, sizeof ak->setter_uid);

	if (mt)
		ak->entity = mt;
	else
		ak->host   = sstrdup(hostmask);

	if (duration > 0)
		ak->expires = CURRTIME + duration;

	if (*reason)
		ak->reason = sstrdup(reason);

	mowgli_node_add(ak, &ak->list_node, akick_list);

	const char *display_target = mt ? mt->name : hostmask;

	if (duration > 0)
	{
		command_success_nodata(si, _("AKICK on \2%s\2 was successfully added for \2%s\2 and will expire in %s."), display_target, mc->name, timediff(duration));
		verbose(mc, "\2%s\2 added \2%s\2 to the AKICK list, expires in %s.", get_source_name(si), display_target, timediff(duration));
		logcommand(si, CMDLOG_SET, "AKICK:ADD: \2%s\2 on \2%s\2, expires in %s", display_target, mc->name, timediff(duration));

		akick_add_timeout(ak);
	}
	else
	{
		command_success_nodata(si, _("AKICK on \2%s\2 was successfully added to the AKICK list for \2%s\2."), display_target, mc->name);
		verbose(mc, "\2%s\2 added \2%s\2 to the AKICK list.", get_source_name(si), display_target);
		logcommand(si, CMDLOG_SET, "AKICK:ADD: \2%s\2 on \2%s\2", display_target, mc->name);
	}

	akick_sync(ak);
}

static void
cs_cmd_akick_del(struct sourceinfo *si, int parc, char *parv[])
{
	char *chan = parv[0];
	char *uname = parv[1];

	if (!chan || !uname)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "AKICK");
		command_fail(si, fault_needmoreparams, _("Syntax: AKICK <#channel> DEL <nickname|hostmask>"));
		return;
	}

	struct mychan *mc = mychan_find(chan);
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

	if ((chanacs_source_flags(mc, si) & (CA_FLAGS | CA_REMOVE)) != (CA_FLAGS | CA_REMOVE))
	{
		command_fail(si, fault_noprivs, STR_NOT_AUTHORIZED);
		return;
	}

	struct myentity *mt = myentity_find_ext(uname);

	mowgli_list_t *akick_list = channel_get_akicks(mc, false);

	const char *removed = mt ? mt->name : uname;

	if (akick_list)
	{
		struct akick *ak = NULL;

		if (mt)
			ak = akick_find_entity(mc, mt, true);
		else
			ak = akick_find_host(mc, uname, true);

		if (ak)
		{
			command_success_nodata(si, _("\2%s\2 has been removed from the AKICK list for \2%s\2."), removed, mc->name);
			logcommand(si, CMDLOG_SET, "AKICK:DEL: \2%s\2 on \2%s\2", removed, mc->name);
			verbose(mc, "\2%s\2 removed \2%s\2 from the AKICK list.", get_source_name(si), removed);

			struct chanban *cb;
			if (ak->host && mc->chan != NULL && (cb = chanban_find(mc->chan, ak->host, 'b')))
			{
				modestack_mode_param(chansvs.nick, mc->chan, MTYPE_DEL, cb->type, cb->mask);
				chanban_delete(cb);
			}
			else if (ak->entity && mc->chan != NULL)
			{
				clear_bans_matching_entity(mc->chan, ak->entity);
			}

			akick_destroy(ak);
			return;
		}
	}

	command_fail(si, fault_nosuch_key, _("\2%s\2 is not on the AKICK list for \2%s\2."), removed, mc->name);
}

static void
cs_cmd_akick_list(struct sourceinfo *si, int parc, char *parv[])
{
	char *chan = parv[0];

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

	struct mychan *mc = mychan_find(chan);
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

	bool operoverride = false;
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

	unsigned int i = 0;

	mowgli_list_t *akick_list = channel_get_akicks(mc, false);

	if (akick_list)
	{
		mowgli_node_t *n;
		MOWGLI_ITER_FOREACH(n, akick_list->head)
		{
			struct akick *ak = n->data;

			// check if it's a temporary akick
			long time_left = 0;
			if (ak->expires)
				time_left = difftime(ak->expires, CURRTIME);

			char *ago = time_ago(ak->tmodified);

			char buf[BUFSIZE];
			char *buf_iter = buf;
			buf_iter += snprintf(buf_iter, sizeof(buf) - (buf_iter - buf), "%u: \2%s\2 (\2%s\2) [",
					ak->number, ak->entity != NULL ? ak->entity->name : ak->host,
					ak->reason ? ak->reason : _("no AKICK reason specified"));

			struct myentity *setter = NULL;

			if (*ak->setter_uid != '\0' && (setter = myentity_find_uid(ak->setter_uid)))
				buf_iter += snprintf(buf_iter, sizeof(buf) - (buf_iter - buf), _("setter: %s"),
						setter->name);

			if (ak->expires > 0)
				buf_iter += snprintf(buf_iter, sizeof(buf) - (buf_iter - buf), _("%sexpires: %s"),
						setter != NULL ? ", " : "", timediff(time_left));

			buf_iter += snprintf(buf_iter, sizeof(buf) - (buf_iter - buf), _("%smodified: %s"),
					ak->expires > 0 || setter != NULL ? ", " : "", ago);

			mowgli_strlcat(buf, "]", sizeof buf);

			command_success_nodata(si, "%s", buf);

			i++;
		}
	}

	command_success_nodata(si, _("Total of \2%u\2 entries in \2%s\2's AKICK list."), i, mc->name);

	if (operoverride)
		logcommand(si, CMDLOG_ADMIN, "AKICK:LIST: \2%s\2 (oper override)", mc->name);
	else
		logcommand(si, CMDLOG_GET, "AKICK:LIST: \2%s\2", mc->name);
}

static struct command cs_akick = {
	.name           = "AKICK",
	.desc           = N_("Manipulates a channel's AKICK list."),
	.access         = AC_AUTHENTICATED,
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
write_akicks_db(struct database_handle *db)
{
	mowgli_patricia_iteration_state_t state;
	struct mychan *mc;

	MOWGLI_PATRICIA_FOREACH(mc, &state, mclist)
	{
		mowgli_list_t *akick_list = channel_get_akicks(mc, false);
		mowgli_node_t *n;

		if (!akick_list)
			continue;

		MOWGLI_ITER_FOREACH(n, akick_list->head)
		{
			struct akick *ak = n->data;

			db_start_row(db, DB_TYPE);
			db_write_word(db, ak->mychan->name);
			db_write_uint(db, ak->number);
			if (ak->entity)
				db_write_word(db, ak->entity->name);
			else
				db_write_word(db, ak->host);
			db_write_word(db, ak->setter_uid);
			db_write_time(db, ak->tmodified);
			db_write_time(db, ak->expires);
			if (ak->reason)
				db_write_str(db, ak->reason);
			else
				db_write_str(db, "");
			db_commit_row(db);
		}
	}
}

static void
db_h_ak(struct database_handle *db, const char *type)
{
	const char *chan_name  = db_sread_word(db);
	unsigned int number    = db_sread_uint(db);
	const char *target     = db_sread_word(db);
	const char *setter_uid = db_sread_word(db);
	time_t tmodified       = db_sread_time(db);
	time_t expires         = db_sread_time(db);
	const char *reason     = db_sread_str(db);

	struct mychan *mc = mychan_find(chan_name);

	if (!mc)
	{
		slog(LG_ERROR, "chanserv/akick: DB line %u: akick for nonexistent channel %s - exiting to avoid data loss", db->line, chan_name);
		exit(EXIT_FAILURE);
	}

	struct akick *ak = mowgli_heap_alloc(akick_heap);
	ak->mychan    = mc;
	ak->number    = number;
	ak->tmodified = tmodified;
	ak->expires   = expires;

	if (reason[0])
		ak->reason = sstrdup(reason);
	else
		ak->reason = NULL;

	mowgli_strlcpy(ak->setter_uid, setter_uid, sizeof ak->setter_uid);

	struct myentity *mt = myentity_find(target);
	if (mt)
	{
		ak->entity = mt;
	}
	else if (validhostmask(target))
	{
		ak->host = sstrdup(target);
	}
	else
	{
		slog(LG_ERROR, "chanserv/akick: DB line %u: akick for nonexistent target %s - exiting to avoid data loss", db->line, target);
		exit(EXIT_FAILURE);
	}

	mowgli_list_t *mc_akick_list = channel_get_akicks(mc, true);
	mowgli_node_add(ak, &ak->list_node, mc_akick_list);

	if (ak->expires)
		akick_add_timeout(ak);
}

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "chanserv/main")

	//if (! (akick_heap = mowgli_heap_create(sizeof(struct akick_timeout), 512, BH_NOW)))
	if (! (akick_heap = mowgli_heap_create(sizeof(struct akick), 512, BH_NOW)))
	{
		(void) slog(LG_ERROR, "%s: mowgli_heap_create() failed", m->name);

		m->mflags |= MODFLAG_FAIL;
		return;
	}

	if (! (cs_akick_cmds = mowgli_patricia_create(&strcasecanon)))
	{
		(void) slog(LG_ERROR, "%s: mowgli_patricia_create() failed", m->name);

		(void) mowgli_heap_destroy(akick_heap);

		m->mflags |= MODFLAG_FAIL;
		return;
	}

	(void) command_add(&cs_akick_add, cs_akick_cmds);
	(void) command_add(&cs_akick_del, cs_akick_cmds);
	(void) command_add(&cs_akick_list, cs_akick_cmds);

	(void) service_named_bind_command("chanserv", &cs_akick);

	(void) hook_add_first_chanuser_sync(&chanuser_sync);
	(void) hook_add_db_write(&write_akicks_db);

	(void) db_register_type_handler(DB_TYPE, db_h_ak);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	(void) hook_del_chanuser_sync(&chanuser_sync);
	(void) hook_del_db_write(&write_akicks_db);

	(void) db_unregister_type_handler(DB_TYPE);

	(void) service_named_unbind_command("chanserv", &cs_akick);

	(void) mowgli_patricia_destroy(cs_akick_cmds, &command_delete_trie_cb, cs_akick_cmds);

	(void) mowgli_heap_destroy(akick_heap);
}

SIMPLE_DECLARE_MODULE_V1("chanserv/akick", MODULE_UNLOAD_CAPABILITY_NEVER)
