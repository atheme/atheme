/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005 Atheme Project (http://atheme.org/)
 *
 * This file contains the main() routine.
 */

#include <atheme.h>
#include "chanserv.h"

static mowgli_eventloop_timer_t *cs_leave_empty_timer = NULL;

static void
join_registered(bool all)
{
	struct mychan *mc;
	mowgli_patricia_iteration_state_t state;

	MOWGLI_PATRICIA_FOREACH(mc, &state, mclist)
	{
		if (!(mc->flags & MC_GUARD))
			continue;
		if (metadata_find(mc, "private:botserv:bot-assigned") != NULL)
			continue;

		if (all)
		{
			join(mc->name, chansvs.nick);
			continue;
		}
		else if (mc->chan != NULL && mc->chan->members.count != 0)
		{
			join(mc->name, chansvs.nick);
			continue;
		}
	}
}

// main services client routine
static void
chanserv(struct sourceinfo *si, int parc, char *parv[])
{
	struct mychan *mc = NULL;
	char orig[BUFSIZE];
	char newargs[BUFSIZE];
	char *cmd;
	char *args;

	// this should never happen
	if (parv[parc - 2][0] == '&')
	{
		slog(LG_ERROR, "services(): got parv with local channel: %s", parv[0]);
		return;
	}

	// is this a fantasy command?
	if (parv[parc - 2][0] == '#')
	{
		struct metadata *md;

		if (chansvs.fantasy == false)
		{
			// *all* fantasy disabled
			return;
		}

		mc = mychan_find(parv[parc - 2]);
		if (!mc)
		{
			// unregistered, NFI how we got this message, but let's leave it alone!
			return;
		}

		md = metadata_find(mc, "disable_fantasy");
		if (md)
		{
			// fantasy disabled on this channel. don't message them, just bail.
			return;
		}
	}

	// make a copy of the original for debugging
	mowgli_strlcpy(orig, parv[parc - 1], BUFSIZE);

	// lets go through this to get the command
	cmd = strtok(parv[parc - 1], " ");

	if (!cmd)
		return;
	if (*orig == '\001')
	{
		handle_ctcp_common(si, cmd, strtok(NULL, ""));
		return;
	}

	// take the command through the hash table
	if (mc == NULL)
		command_exec_split(si->service, si, cmd, strtok(NULL, ""), si->service->commands);
	else
	{
		struct metadata *md = metadata_find(mc, "private:prefix");
		const char *prefix = (md ? md->value : chansvs.trigger);

		if (strlen(cmd) >= 2 && strchr(prefix, cmd[0]) && isalpha((unsigned char)*++cmd))
		{
			const char *realcmd = service_resolve_alias(si->service, NULL, cmd);

			/* XXX not really nice to look up the command twice
			 * -- jilles */
			if (command_find(si->service->commands, realcmd) == NULL)
				return;
			if (floodcheck(si->su, si->service->me))
				return;
			// construct <channel> <args>
			mowgli_strlcpy(newargs, parv[parc - 2], sizeof newargs);
			args = strtok(NULL, "");
			if (args != NULL)
			{
				mowgli_strlcat(newargs, " ", sizeof newargs);
				mowgli_strlcat(newargs, args, sizeof newargs);
			}

			// let the command know it's called as fantasy cmd
			si->c = mc->chan;

			/* fantasy commands are always verbose
			 * (a little ugly but this way we can !set verbose)
			 */
			mc->flags |= MC_FORCEVERBOSE;
			command_exec_split(si->service, si, realcmd, newargs, si->service->commands);
			mc->flags &= ~MC_FORCEVERBOSE;
		}
		else if (!ircncasecmp(cmd, chansvs.nick, strlen(chansvs.nick)) && !isalnum((unsigned char)cmd[strlen(chansvs.nick)]) && (cmd = strtok(NULL, "")) != NULL)
		{
			const char *realcmd;
			char *pptr;

			mowgli_strlcpy(newargs, parv[parc - 2], sizeof newargs);
			while (*cmd == ' ')
				cmd++;
			if ((pptr = strchr(cmd, ' ')) != NULL)
			{
				mowgli_strlcat(newargs, pptr, sizeof newargs);
				*pptr = '\0';
			}

			realcmd = service_resolve_alias(si->service, NULL, cmd);

			if (command_find(si->service->commands, realcmd) == NULL)
				return;
			if (floodcheck(si->su, si->service->me))
				return;

			si->c = mc->chan;

			/* fantasy commands are always verbose
			 * (a little ugly but this way we can !set verbose)
			 */
			mc->flags |= MC_FORCEVERBOSE;
			command_exec_split(si->service, si, realcmd, newargs, si->service->commands);
			mc->flags &= ~MC_FORCEVERBOSE;
		}
	}
}

static void
chanserv_config_ready(void *unused)
{
	chansvs.nick = chansvs.me->nick;
	chansvs.user = chansvs.me->user;
	chansvs.host = chansvs.me->host;
	chansvs.real = chansvs.me->real;

	service_set_chanmsg(chansvs.me, true);

	if (me.connected)
		join_registered(false); // !config_options.leave_chans
}

static int
c_ci_vop(mowgli_config_file_entry_t *ce)
{
	if (ce->vardata == NULL)
	{
		conf_report_warning(ce, "no parameter for configuration option");
		return 0;
	}

	set_global_template_flags("VOP", flags_to_bitmask(ce->vardata, 0));

	return 0;
}

static int
c_ci_hop(mowgli_config_file_entry_t *ce)
{
	if (ce->vardata == NULL)
	{
		conf_report_warning(ce, "no parameter for configuration option");
		return 0;
	}

	set_global_template_flags("HOP", flags_to_bitmask(ce->vardata, 0));

	return 0;
}

static int
c_ci_aop(mowgli_config_file_entry_t *ce)
{
	if (ce->vardata == NULL)
	{
		conf_report_warning(ce, "no parameter for configuration option");
		return 0;
	}

	set_global_template_flags("AOP", flags_to_bitmask(ce->vardata, 0));

	return 0;
}

static int
c_ci_sop(mowgli_config_file_entry_t *ce)
{
	if (ce->vardata == NULL)
	{
		conf_report_warning(ce, "no parameter for configuration option");
		return 0;
	}

	set_global_template_flags("SOP", flags_to_bitmask(ce->vardata, 0));

	return 0;
}

static int
c_ci_templates(mowgli_config_file_entry_t *ce)
{
	mowgli_config_file_entry_t *flce;

	MOWGLI_ITER_FOREACH(flce, ce->entries)
	{
		if (flce->vardata == NULL)
		{
			conf_report_warning(ce, "no parameter for configuration option");
			return 0;
		}

		set_global_template_flags(flce->varname, flags_to_bitmask(flce->vardata, 0));
	}

	return 0;
}

static void
chanuser_sync(struct hook_chanuser_sync *hdata)
{
	struct chanuser *cu    = hdata->cu;
	bool             take  = hdata->take_prefixes;
	unsigned int     flags = hdata->flags;

	if (!cu)
		return;

	struct channel *chan = cu->chan;
	struct user    *u    = cu->user;
	struct mychan  *mc   = chan->mychan;

	return_if_fail(mc != NULL);

	bool noop = (mc->flags & MC_NOOP) || (u->myuser != NULL && u->myuser->flags & MU_NOOP);

	if (ircd->uses_owner)
	{
		if (flags & CA_USEOWNER)
		{
			if (flags & CA_AUTOOP && !(noop || cu->modes & ircd->owner_mode))
			{
				modestack_mode_param(chansvs.nick, chan, MTYPE_ADD, ircd->owner_mchar[1], CLIENT_NAME(u));
				cu->modes |= ircd->owner_mode;
			}
		}
		else if (take && (cu->modes & ircd->owner_mode))
		{
			modestack_mode_param(chansvs.nick, chan, MTYPE_DEL, ircd->owner_mchar[1], CLIENT_NAME(u));
			cu->modes &= ~ircd->owner_mode;
		}
	}

	if (ircd->uses_protect)
	{
		if (flags & CA_USEPROTECT)
		{
			if (flags & CA_AUTOOP && !(noop || cu->modes & ircd->protect_mode || (ircd->uses_owner && cu->modes & ircd->owner_mode)))
			{
				modestack_mode_param(chansvs.nick, chan, MTYPE_ADD, ircd->protect_mchar[1], CLIENT_NAME(u));
				cu->modes |= ircd->protect_mode;
			}
		}
		else if (take && (cu->modes & ircd->protect_mode))
		{
			modestack_mode_param(chansvs.nick, chan, MTYPE_DEL, ircd->protect_mchar[1], CLIENT_NAME(u));
			cu->modes &= ~ircd->protect_mode;
		}
	}

	if (flags & CA_AUTOOP)
	{
		if (!(noop || cu->modes & CSTATUS_OP))
		{
			modestack_mode_param(chansvs.nick, chan, MTYPE_ADD, 'o', CLIENT_NAME(u));
			cu->modes |= CSTATUS_OP;
		}
	}
	else if (take && (cu->modes & CSTATUS_OP) && !(flags & CA_OP) && !is_service(cu->user))
	{
		modestack_mode_param(chansvs.nick, chan, MTYPE_DEL, 'o', CLIENT_NAME(u));
		cu->modes &= ~CSTATUS_OP;
	}

	if (ircd->uses_halfops)
	{
		if (flags & CA_AUTOHALFOP)
		{
			if (!(noop || cu->modes & (CSTATUS_OP | ircd->halfops_mode)))
			{
				modestack_mode_param(chansvs.nick, chan, MTYPE_ADD, 'h', CLIENT_NAME(u));
				cu->modes |= ircd->halfops_mode;
			}
		}
		else if (take && (cu->modes & ircd->halfops_mode) && !(flags & CA_HALFOP))
		{
			modestack_mode_param(chansvs.nick, chan, MTYPE_DEL, 'h', CLIENT_NAME(u));
			cu->modes &= ~ircd->halfops_mode;
		}
	}

	if (flags & CA_AUTOVOICE)
	{
		if (!(noop || cu->modes & (CSTATUS_OP | ircd->halfops_mode | CSTATUS_VOICE)))
		{
			modestack_mode_param(chansvs.nick, chan, MTYPE_ADD, 'v', CLIENT_NAME(u));
			cu->modes |= CSTATUS_VOICE;
		}
	}
	else if (take && (cu->modes & CSTATUS_VOICE) && !(flags & CA_VOICE))
	{
		modestack_mode_param(chansvs.nick, chan, MTYPE_DEL, 'v', CLIENT_NAME(u));
		cu->modes &= ~CSTATUS_VOICE;
	}
}

static void
cs_join(struct hook_channel_joinpart *hdata)
{
	struct chanuser *cu = hdata->cu;
	struct user *u;
	struct channel *chan;
	struct mychan *mc;
	unsigned int flags;
	bool noop;
	bool secure;
	struct metadata *md;
	struct chanacs *ca2;

	if (cu == NULL || is_internal_client(cu->user))
		return;
	u = cu->user;
	chan = cu->chan;

	// first check if this is a registered channel at all
	mc = mychan_find(chan->name);
	if (mc == NULL)
		return;

	flags = chanacs_user_flags(mc, u);
	noop = mc->flags & MC_NOOP || (u->myuser != NULL &&
			u->myuser->flags & MU_NOOP);
	/* attempt to deop people recreating channels, if the more
	 * sophisticated mechanism is disabled */
	secure = mc->flags & MC_SECURE || (!chansvs.changets &&
			chan->nummembers == 1 && chan->ts > CURRTIME - 300);

	if (chan->nummembers == 1 && mc->flags & MC_GUARD &&
		metadata_find(mc, "private:botserv:bot-assigned") == NULL)
		join(chan->name, chansvs.nick);

	/* Kick out users who may be recreating channels mlocked +i.
	 * Users with +i flag are allowed to join, as are users matching
	 * an invite exception (the latter only works if the channel already
	 * exists because members are sent before invite exceptions).
	 * Because most networks do not use kick_on_split_riding or
	 * no_create_on_split, do not trust users coming back from splits;
	 * with the exception of users coming back after a services
	 * restart if the channel TS did not change.
	 * Unfortunately, this kicks users who have been invited by a channel
	 * operator, after a split.
	 */
	if (mc->mlock_on & CMODE_INVITE && !(flags & CA_INVITE) &&
			(!me.bursting || mc->flags & MC_RECREATED) &&
			(!(u->server->flags & SF_EOB) || (chan->nummembers - chan->numsvcmembers == 1)) &&
			(!ircd->invex_mchar || !next_matching_ban(chan, u, ircd->invex_mchar, chan->bans.head)))
	{
		if (chan->nummembers - chan->numsvcmembers == 1)
		{
			mc->flags |= MC_INHABIT;
			if (chan->numsvcmembers == 0)
				join(chan->name, chansvs.nick);
		}
		if (!(chan->modes & CMODE_INVITE))
			check_modes(mc, true);
		modestack_flush_channel(chan);
		if (try_kick(chansvs.me->me, chan, u, "Invite only channel"))
		{
			hdata->cu = NULL;
			return;
		}
	}

	struct hook_chanuser_sync sync_hdata = {
		.cu = cu,
		.flags = flags,
		.take_prefixes = secure,
	};
	hook_call_chanuser_sync(&sync_hdata);

	if (!sync_hdata.cu)
	{
		hdata->cu = NULL;
		return;
	}

	/* A second user joined and was not kicked; we do not need
	 * to stay on the channel artificially.
	 * If there is only one user, stay in the channel to avoid
	 * triggering autocycle-for-ops scripts and immediately
	 * destroying channels with kick on split riding.
	 */
	if (mc->flags & MC_INHABIT && chan->nummembers - chan->numsvcmembers >= 2)
	{
		mc->flags &= ~MC_INHABIT;
		if (!(mc->flags & MC_GUARD) && !(chan->flags & CHAN_LOG) && chanuser_find(chan, chansvs.me->me))
			part(chan->name, chansvs.nick);
	}

	if (flags & CA_USEDUPDATE)
		mc->used = CURRTIME;
}

static void
cs_part(struct hook_channel_joinpart *hdata)
{
	struct chanuser *cu;
	struct mychan *mc;

	cu = hdata->cu;
	if (cu == NULL)
		return;
	mc = mychan_find(cu->chan->name);
	if (mc == NULL)
		return;
	if (metadata_find(mc, "private:botserv:bot-assigned") != NULL)
		return;

	if ((CURRTIME - mc->used) >= SECONDS_PER_HOUR)
		if (chanacs_user_flags(mc, cu->user) & CA_USEDUPDATE)
			mc->used = CURRTIME;

	/*
	 * When channel_part is fired, we haven't yet removed the
	 * user from the room. So, the channel will have two members
	 * if ChanServ is joining channels: the triggering user and
	 * itself.
	 *
	 * This if block was utter nonsense.  Refactor it into multiple
	 * branches for better clarity and debugging ability. --nenolod
	 */

	// we're not parting if we've been told to never part.
	if (!config_options.leave_chans)
		return;

	// we're not parting if the channel has more than one person on it
	if (cu->chan->nummembers - cu->chan->numsvcmembers > 1)
		return;

	// internal clients parting a channel shouldn't cause chanserv to leave.
	if (is_internal_client(cu->user))
		return;

	// if we're enforcing an akick, we're MC_INHABIT.  do not part.
	if (mc->flags & MC_INHABIT)
	{
		slog(LG_DEBUG, "cs_part(): not leaving channel %s due to MC_INHABIT flag", mc->name);
		return;
	}

	// seems we've met all conditions to be parted from a channel.
	part(cu->chan->name, chansvs.nick);
}

static struct user *
get_changets_user(struct mychan *mc)
{
	struct metadata *md;

	return_val_if_fail(mc != NULL, chansvs.me->me);

	md = metadata_find(mc, "private:botserv:bot-assigned");
	if (md != NULL)
	{
		struct user *u = user_find(md->value);

		return_val_if_fail(is_internal_client(u), chansvs.me->me);

		return u;
	}

	return chansvs.me->me;
}

static void
cs_register(struct hook_channel_req *hdata)
{
	struct mychan *mc;

	mc = hdata->mc;
	if (mc->chan)
	{
		if (mc->flags & MC_GUARD)
			join(mc->name, chansvs.nick);
		if (metadata_find(mc, "private:botserv:bot-assigned") != NULL)
			return;

		mlock_sts(mc->chan);
		topiclock_sts(mc->chan);
		check_modes(mc, true);
	}
}

static void
cs_succession(struct hook_channel_succession_req *data)
{
	chanacs_change_simple(data->mc, entity(data->mu), NULL, custom_founder_check(), 0, NULL);
}

// Called on every set of a topic, after updating our internal structures
static void
cs_keeptopic_topicset(struct channel *c)
{
	struct mychan *mc;
	struct metadata *md;

	mc = mychan_find(c->name);

	if (mc == NULL)
		return;

	md = metadata_find(mc, "private:topic:text");
	if (md != NULL)
	{
		if (c->topic != NULL && !strcmp(md->value, c->topic))
			return;
		metadata_delete(mc, "private:topic:text");
	}

	if (metadata_find(mc, "private:topic:setter"))
		metadata_delete(mc, "private:topic:setter");

	if (metadata_find(mc, "private:topic:ts"))
		metadata_delete(mc, "private:topic:ts");

	if (c->topic && c->topic_setter)
	{
		slog(LG_DEBUG, "KeepTopic: topic set for %s by %s: %s", c->name,
			c->topic_setter, c->topic);
		metadata_add(mc, "private:topic:setter",
			c->topic_setter);
		metadata_add(mc, "private:topic:text",
			c->topic);
		metadata_add(mc, "private:topic:ts",
			number_to_string(c->topicts));
	}
	else
		slog(LG_DEBUG, "KeepTopic: topic cleared for %s", c->name);
}

/* Called when a topic change is received from the network, before altering
 * our internal structures */
static void
cs_topiccheck(struct hook_channel_topic_check *data)
{
	struct mychan *mc;
	unsigned int accessfl = 0;

	mc = mychan_find(data->c->name);
	if (mc == NULL)
		return;

	if ((mc->flags & (MC_KEEPTOPIC | MC_TOPICLOCK)) == (MC_KEEPTOPIC | MC_TOPICLOCK))
	{
		if (data->u == NULL || !((accessfl = chanacs_user_flags(mc, data->u)) & CA_TOPIC))
		{
			// topic burst or unauthorized user, revert it
			data->approved = 1;
			slog(LG_DEBUG, "cs_topiccheck(): reverting topic change on channel %s by %s",
					data->c->name,
					data->u != NULL ? data->u->nick : "<server>");

			if (data->u != NULL && !(mc->mlock_off & CMODE_TOPIC))
			{
				/* they do not have access to be opped either,
				 * deop them and set +t */
				/* note: channel_mode() takes nicks, not UIDs
				 * when used with a non-NULL source */
				if (ircd->uses_halfops && !(accessfl & (CA_OP | CA_AUTOOP | CA_HALFOP | CA_AUTOHALFOP)))
					channel_mode_va(chansvs.me->me, data->c,
							3, "+t-oh", data->u->nick, data->u->nick);
				else if (!ircd->uses_halfops && !(accessfl & (CA_OP | CA_AUTOOP)))
					channel_mode_va(chansvs.me->me, data->c,
							2, "+t-o", data->u->nick);
			}
		}
	}
}

// Called on creation of a channel
static void
cs_newchan(struct channel *c)
{
	struct mychan *mc;
	struct chanuser *cu;
	struct metadata *md;
	char *setter;
	char *text;
	time_t channelts = 0;
	time_t topicts;
	char str[21];

	if (!(mc = mychan_find(c->name)))
		return;

	// set struct channel -> mychan
	c->mychan = mc;

	/* schedule a mode lock check when we know the current modes
	 * -- jilles */
	mc->flags |= MC_MLOCK_CHECK;

	md = metadata_find(mc, "private:channelts");
	if (md != NULL)
		channelts = atol(md->value);
	if (channelts == 0)
		channelts = mc->registered;

	if (c->ts > channelts && channelts > 0)
		mc->flags |= MC_RECREATED;
	else
		mc->flags &= ~MC_RECREATED;

	if (chansvs.changets && c->ts > channelts && channelts > 0)
	{
		struct user *u;

		u = get_changets_user(mc);

		// Stop the splitrider -- jilles
		c->ts = channelts;
		clear_simple_modes(c);
		c->modes |= CMODE_NOEXT | CMODE_TOPIC;
		check_modes(mc, false);

		// No ops to clear
		chan_lowerts(c, u);
		cu = chanuser_add(c, CLIENT_NAME(u));
		cu->modes |= CSTATUS_OP;

		// make sure it parts again sometime (empty SJOIN etc)
		mc->flags |= MC_INHABIT;
	}
	else if (c->ts != channelts)
	{
		snprintf(str, sizeof str, "%lu", (unsigned long)c->ts);
		metadata_add(mc, "private:channelts", str);
	}
	else if (!(MC_TOPICLOCK & mc->flags) && MOWGLI_LIST_LENGTH(&c->members) == 0)
	{
		mlock_sts(c);

		/* Same channel, let's assume ircd has kept topic.
		 * However, if topiclock is enabled, we must change it back
		 * regardless.
		 * Also, if there is someone in this channel already, it is
		 * being created by a service and we must restore.
		 * -- jilles */
		return;
	}

	mlock_sts(c);
	topiclock_sts(c);

	if (!(MC_KEEPTOPIC & mc->flags))
		return;

	md = metadata_find(mc, "private:topic:setter");
	if (md == NULL)
		return;
	setter = md->value;

	md = metadata_find(mc, "private:topic:text");
	if (md == NULL)
		return;
	text = md->value;

	md = metadata_find(mc, "private:topic:ts");
	if (md == NULL)
		return;
	topicts = atol(md->value);

	handle_topic(c, setter, topicts, text);
	topic_sts(c, chansvs.me->me, setter, topicts, 0, text);
}

static void
cs_tschange(struct channel *c)
{
	struct mychan *mc;
	char str[21];

	if (!(mc = mychan_find(c->name)))
		return;

	// store new TS
	snprintf(str, sizeof str, "%lu", (unsigned long)c->ts);
	metadata_add(mc, "private:channelts", str);

	/* schedule a mode lock check when we know the new modes
	 * -- jilles */
	mc->flags |= MC_MLOCK_CHECK;

	// reset the mlock if needed
	mlock_sts(c);
	topiclock_sts(c);
}

static void
on_shutdown(void *unused)
{
	if (chansvs.me != NULL && chansvs.me->me != NULL)
		quit_sts(chansvs.me->me, "shutting down");
}

static void
cs_leave_empty(void *unused)
{
	struct mychan *mc;
	mowgli_patricia_iteration_state_t state;

	(void)unused;
	MOWGLI_PATRICIA_FOREACH(mc, &state, mclist)
	{
		if (!(mc->flags & MC_INHABIT))
			continue;

		// If there is only one user, stay indefinitely.
		if (mc->chan != NULL && mc->chan->nummembers - mc->chan->numsvcmembers == 1)
			continue;

		mc->flags &= ~MC_INHABIT;
		if (mc->chan != NULL &&
				!(mc->chan->flags & CHAN_LOG) &&
				(!(mc->flags & MC_GUARD) ||
				 (config_options.leave_chans && mc->chan->nummembers == mc->chan->numsvcmembers) ||
				 metadata_find(mc, "private:close:closer")) &&
				chanuser_find(mc->chan, chansvs.me->me))
		{
			slog(LG_DEBUG, "cs_leave_empty(): leaving %s", mc->chan->name);
			part(mc->chan->name, chansvs.nick);
		}
	}
}

static void
cs_bounce_mode_change(struct hook_channel_mode_change *data)
{
	struct mychan *mc;
	struct chanuser *cu;
	struct channel *chan;

	// if we have SECURE mode enabled, then we want to bounce any changes
	cu = data->cu;
	chan = cu->chan;
	mc = chan->mychan;

	if (mc == NULL || (mc != NULL && !(mc->flags & MC_SECURE)))
		return;

	if (ircd->uses_owner && data->mchar == ircd->owner_mchar[1] && !(chanacs_user_flags(mc, cu->user) & (CA_USEOWNER)))
	{
		modestack_mode_param(chansvs.nick, chan, MTYPE_DEL, ircd->owner_mchar[1], CLIENT_NAME(cu->user));
		cu->modes &= ~data->mvalue;
	}
	else if (ircd->uses_protect && data->mchar == ircd->protect_mchar[1] && !(chanacs_user_flags(mc, cu->user) & (CA_USEPROTECT)))
	{
		modestack_mode_param(chansvs.nick, chan, MTYPE_DEL, ircd->protect_mchar[1], CLIENT_NAME(cu->user));
		cu->modes &= ~data->mvalue;
	}
	else if (data->mchar == 'o' && !(chanacs_user_flags(mc, cu->user) & (CA_OP | CA_AUTOOP)) && !is_service(cu->user))
	{
		modestack_mode_param(chansvs.nick, chan, MTYPE_DEL, 'o', CLIENT_NAME(cu->user));
		cu->modes &= ~data->mvalue;
	}
	else if (ircd->uses_halfops && data->mchar == ircd->halfops_mchar[1] && !(chanacs_user_flags(mc, cu->user) & (CA_HALFOP | CA_AUTOHALFOP)))
	{
		modestack_mode_param(chansvs.nick, chan, MTYPE_DEL, ircd->halfops_mchar[1], CLIENT_NAME(cu->user));
		cu->modes &= ~data->mvalue;
	}
}

static void
mod_init(struct module ATHEME_VATTR_UNUSED *const restrict m)
{
	hook_add_config_ready(chanserv_config_ready);

	chansvs.me = service_add("chanserv", chanserv);

	hook_add_channel_join(cs_join);
	hook_add_channel_part(cs_part);
	hook_add_channel_register(cs_register);
	hook_add_channel_succession(cs_succession);
	hook_add_channel_add(cs_newchan);
	hook_add_channel_topic(cs_keeptopic_topicset);
	hook_add_channel_can_change_topic(cs_topiccheck);
	hook_add_channel_tschange(cs_tschange);
	hook_add_channel_mode_change(cs_bounce_mode_change);
	hook_add_chanuser_sync(chanuser_sync);
	hook_add_shutdown(on_shutdown);

	cs_leave_empty_timer = mowgli_timer_add(base_eventloop, "cs_leave_empty", cs_leave_empty, NULL, 5 * SECONDS_PER_MINUTE);

	// chanserv{} block
	add_bool_conf_item("FANTASY", &chansvs.me->conf_table, 0, &chansvs.fantasy, false);
	add_conf_item("VOP", &chansvs.me->conf_table, c_ci_vop);
	add_conf_item("HOP", &chansvs.me->conf_table, c_ci_hop);
	add_conf_item("AOP", &chansvs.me->conf_table, c_ci_aop);
	add_conf_item("SOP", &chansvs.me->conf_table, c_ci_sop);
	add_conf_item("TEMPLATES", &chansvs.me->conf_table, c_ci_templates);
	add_bool_conf_item("CHANGETS", &chansvs.me->conf_table, 0, &chansvs.changets, false);
	add_bool_conf_item("HIDE_XOP", &chansvs.me->conf_table, 0, &chansvs.hide_xop, false);
	add_bool_conf_item("HIDE_PUBACL_AKICKS", &chansvs.me->conf_table, 0, &chansvs.hide_pubacl_akicks, false);
	add_dupstr_conf_item("TRIGGER", &chansvs.me->conf_table, 0, &chansvs.trigger, "!");
	add_duration_conf_item("EXPIRE", &chansvs.me->conf_table, 0, &chansvs.expiry, "d", 0);
	add_uint_conf_item("MAXCHANS", &chansvs.me->conf_table, 0, &chansvs.maxchans, 1, INT_MAX, 5);
	add_uint_conf_item("MAXCHANACS", &chansvs.me->conf_table, 0, &chansvs.maxchanacs, 0, INT_MAX, 0);
	add_uint_conf_item("MAXFOUNDERS", &chansvs.me->conf_table, 0, &chansvs.maxfounders, 1, (512 - 60) / (9 + 2), 4); // fit on a line
	add_dupstr_conf_item("FOUNDER_FLAGS", &chansvs.me->conf_table, 0, &chansvs.founder_flags, NULL);
	add_dupstr_conf_item("DEFTEMPLATES", &chansvs.me->conf_table, 0, &chansvs.deftemplates, NULL);
	add_duration_conf_item("AKICK_TIME", &chansvs.me->conf_table, 0, &chansvs.akick_time, "m", 0);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{

}

SIMPLE_DECLARE_MODULE_V1("chanserv/main", MODULE_UNLOAD_CAPABILITY_NEVER)
