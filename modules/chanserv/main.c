/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains the main() routine.
 *
 * $Id: main.c 8273 2007-05-19 05:31:46Z nenolod $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/main", FALSE, _modinit, _moddeinit,
	"$Id: main.c 8273 2007-05-19 05:31:46Z nenolod $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_join(hook_channel_joinpart_t *hdata);
static void cs_part(hook_channel_joinpart_t *hdata);
static void cs_register(mychan_t *mc);
static void cs_newchan(channel_t *c);
static void cs_keeptopic_topicset(channel_t *c);
static void cs_topiccheck(hook_channel_topic_check_t *data);
static void cs_tschange(channel_t *c);
static void cs_leave_empty(void *unused);

list_t cs_cmdtree;
list_t cs_helptree;

E list_t mychan;

static void join_registered(boolean_t all)
{
	mychan_t *mc;
	dictionary_iteration_state_t state;

	DICTIONARY_FOREACH(mc, &state, mclist)
	{
		if (!(mc->flags & MC_GUARD))
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

/* main services client routine */
static void chanserv(sourceinfo_t *si, int parc, char *parv[])
{
	mychan_t *mc = NULL;
	char orig[BUFSIZE];
	char newargs[BUFSIZE];
	char *cmd;
	char *args;

	/* this should never happen */
	if (parv[parc - 2][0] == '&')
	{
		slog(LG_ERROR, "services(): got parv with local channel: %s", parv[0]);
		return;
	}

	/* is this a fantasy command? */
	if (parv[parc - 2][0] == '#')
	{
		metadata_t *md;

		if (chansvs.fantasy == FALSE)
		{
			/* *all* fantasy disabled */
			return;
		}

		mc = mychan_find(parv[parc - 2]);
		if (!mc)
		{
			/* unregistered, NFI how we got this message, but let's leave it alone! */
			return;
		}

		md = metadata_find(mc, METADATA_CHANNEL, "disable_fantasy");
		if (md)
		{
			/* fantasy disabled on this channel. don't message them, just bail. */
			return;
		}
	}

	/* make a copy of the original for debugging */
	strlcpy(orig, parv[parc - 1], BUFSIZE);

	/* lets go through this to get the command */
	cmd = strtok(parv[parc - 1], " ");

	if (!cmd)
		return;
	if (*cmd == '\001')
	{
		handle_ctcp_common(si, cmd, strtok(NULL, ""));
		return;
	}

	/* take the command through the hash table */
	if (mc == NULL)
		command_exec_split(si->service, si, cmd, strtok(NULL, ""), &cs_cmdtree);
	else
	{
		if (strlen(cmd) > 2
		    && (strchr(chansvs.trigger, cmd[0]) != NULL && isalpha(*++cmd)) /* !foo */
		    || (strcasestr(chansvs.nick, cmd) != NULL) && (cmd = strtok(cmd, " ")) != NULL) /* ChanServ: foo */
		{
			/* XXX not really nice to look up the command twice
			 * -- jilles */
			if (command_find(&cs_cmdtree, cmd) == NULL)
				return;
			if (floodcheck(si->su, si->service->me))
				return;
			/* construct <channel> <args> */
			strlcpy(newargs, parv[parc - 2], sizeof newargs);
			args = strtok(NULL, "");
			if (args != NULL)
			{
				strlcat(newargs, " ", sizeof newargs);
				strlcat(newargs, args, sizeof newargs);
			}
			/* let the command know it's called as fantasy cmd */
			si->c = mc->chan;
			/* fantasy commands are always verbose
			 * (a little ugly but this way we can !set verbose)
			 */
			mc->flags |= MC_FORCEVERBOSE;
			command_exec_split(si->service, si, cmd, newargs, &cs_cmdtree);
			mc->flags &= ~MC_FORCEVERBOSE;
		}
	}
}

static void chanserv_config_ready(void *unused)
{
	if (chansvs.me)
		del_service(chansvs.me);

	chansvs.me = add_service(chansvs.nick, chansvs.user,
				 chansvs.host, chansvs.real,
				 chanserv, &cs_cmdtree);
	chansvs.disp = chansvs.me->disp;
	if (chansvs.fantasy)
		fcmd_agent = chansvs.me;

	if (me.connected)
		join_registered(!config_options.leave_chans);

	hook_del_hook("config_ready", chanserv_config_ready);
}

void _modinit(module_t *m)
{
	hook_add_event("config_ready");
	hook_add_hook("config_ready", chanserv_config_ready);

	if (!cold_start)
	{
		chansvs.me = add_service(chansvs.nick, chansvs.user, chansvs.host, chansvs.real, chanserv, &cs_cmdtree);
		chansvs.disp = chansvs.me->disp;
		if (chansvs.fantasy)
			fcmd_agent = chansvs.me;

		if (me.connected)
			join_registered(!config_options.leave_chans);
	}

	hook_add_event("channel_join");
	hook_add_event("channel_part");
	hook_add_event("channel_register");
	hook_add_event("channel_add");
	hook_add_event("channel_topic");
	hook_add_event("channel_can_change_topic");
	hook_add_event("channel_tschange");
	hook_add_hook("channel_join", (void (*)(void *)) cs_join);
	hook_add_hook("channel_part", (void (*)(void *)) cs_part);
	hook_add_hook("channel_register", (void (*)(void *)) cs_register);
	hook_add_hook("channel_add", (void (*)(void *)) cs_newchan);
	hook_add_hook("channel_topic", (void (*)(void *)) cs_keeptopic_topicset);
	hook_add_hook("channel_can_change_topic", (void (*)(void *)) cs_topiccheck);
	hook_add_hook("channel_tschange", (void (*)(void *)) cs_tschange);
	event_add("cs_leave_empty", cs_leave_empty, NULL, 300);
}

void _moddeinit(void)
{
	if (chansvs.me)
	{
		if (fcmd_agent == chansvs.me)
			fcmd_agent = NULL;
		del_service(chansvs.me);
		chansvs.me = NULL;
	}

	hook_del_hook("channel_join", (void (*)(void *)) cs_join);
	hook_del_hook("channel_part", (void (*)(void *)) cs_part);
	hook_del_hook("channel_register", (void (*)(void *)) cs_register);
	hook_del_hook("channel_add", (void (*)(void *)) cs_newchan);
	hook_del_hook("channel_topic", (void (*)(void *)) cs_keeptopic_topicset);
	hook_del_hook("channel_can_change_topic", (void (*)(void *)) cs_topiccheck);
	hook_del_hook("channel_tschange", (void (*)(void *)) cs_tschange);
	event_delete(cs_leave_empty, NULL);
}

static void cs_join(hook_channel_joinpart_t *hdata)
{
	chanuser_t *cu = hdata->cu;
	user_t *u;
	channel_t *chan;
	mychan_t *mc;
	unsigned int flags;
	metadata_t *md;
	boolean_t noop;
	chanacs_t *ca2;
	boolean_t secure = FALSE;
	char akickreason[120] = "User is banned from this channel", *p;

	if (cu == NULL || is_internal_client(cu->user))
		return;
	u = cu->user;
	chan = cu->chan;

	/* first check if this is a registered channel at all */
	mc = mychan_find(chan->name);
	if (mc == NULL)
		return;

	/* attempt to deop people recreating channels, if the more
	 * sophisticated mechanism is disabled */
	if (mc->flags & MC_SECURE || (!chansvs.changets && chan->nummembers == 1 && chan->ts > CURRTIME - 300))
		secure = TRUE;

	if (chan->nummembers == 1 && mc->flags & MC_GUARD)
		join(chan->name, chansvs.nick);

	flags = chanacs_user_flags(mc, u);
	noop = mc->flags & MC_NOOP || (u->myuser != NULL &&
			u->myuser->flags & MU_NOOP);

	/* auto stuff */
	if ((mc->flags & MC_STAFFONLY) && !has_priv_user(u, PRIV_JOIN_STAFFONLY))
	{
		/* Stay on channel if this would empty it -- jilles */
		if (chan->nummembers <= (mc->flags & MC_GUARD ? 2 : 1))
		{
			mc->flags |= MC_INHABIT;
			if (!(mc->flags & MC_GUARD))
				join(chan->name, chansvs.nick);
		}
		ban(chansvs.me->me, chan, u);
		remove_ban_exceptions(chansvs.me->me, chan, u);
		kick(chansvs.nick, chan->name, u->nick, "You are not authorized to be on this channel");
		hdata->cu = NULL;
		return;
	}

	if (flags & CA_AKICK && !(flags & CA_REMOVE))
	{
		/* Stay on channel if this would empty it -- jilles */
		if (chan->nummembers <= (mc->flags & MC_GUARD ? 2 : 1))
		{
			mc->flags |= MC_INHABIT;
			if (!(mc->flags & MC_GUARD))
				join(chan->name, chansvs.nick);
		}
		/* use a user-given ban mask if possible -- jilles */
		ca2 = chanacs_find_host_by_user(mc, u, CA_AKICK);
		if (ca2 != NULL)
		{
			if (chanban_find(chan, ca2->host, 'b') == NULL)
			{
				char str[512];

				chanban_add(chan, ca2->host, 'b');
				snprintf(str, sizeof str, "+b %s", ca2->host);
				/* ban immediately */
				mode_sts(chansvs.nick, chan, str);
			}
		}
		else
		{
			/* XXX this could be done more efficiently */
			ca2 = chanacs_find(mc, u->myuser, CA_AKICK);
			ban(chansvs.me->me, chan, u);
		}
		remove_ban_exceptions(chansvs.me->me, chan, u);
		if (ca2 != NULL)
		{
			md = metadata_find(ca2, METADATA_CHANACS, "reason");
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
		kick(chansvs.nick, chan->name, u->nick, akickreason);
		hdata->cu = NULL;
		return;
	}

	/* A user joined and was not kicked; we do not need
	 * to stay on the channel artificially. */
	if (mc->flags & MC_INHABIT)
	{
		mc->flags &= ~MC_INHABIT;
		if (!(mc->flags & MC_GUARD) && (!config_options.chan || irccasecmp(chan->name, config_options.chan)) && chanuser_find(chan, chansvs.me->me))
			part(chan->name, chansvs.nick);
	}

	if (ircd->uses_owner)
	{
		if (u->myuser != NULL && is_founder(mc, u->myuser))
		{
			if (flags & CA_AUTOOP && !(noop || cu->modes & ircd->owner_mode))
			{
				modestack_mode_param(chansvs.nick, chan, MTYPE_ADD, ircd->owner_mchar[1], CLIENT_NAME(u));
				cu->modes |= ircd->owner_mode;
			}
		}
		else if (secure && (cu->modes & ircd->owner_mode))
		{
			modestack_mode_param(chansvs.nick, chan, MTYPE_DEL, ircd->owner_mchar[1], CLIENT_NAME(u));
			cu->modes &= ~ircd->owner_mode;
		}
	}

	/* XXX still uses should_protect() */
	if (ircd->uses_protect)
	{
		if (u->myuser != NULL && should_protect(mc, u->myuser))
		{
			if (flags & CA_AUTOOP && !(noop || cu->modes & ircd->protect_mode))
			{
				modestack_mode_param(chansvs.nick, chan, MTYPE_ADD, ircd->protect_mchar[1], CLIENT_NAME(u));
				cu->modes |= ircd->protect_mode;
			}
		}
		else if (secure && (cu->modes & ircd->protect_mode))
		{
			modestack_mode_param(chansvs.nick, chan, MTYPE_DEL, ircd->protect_mchar[1], CLIENT_NAME(u));
			cu->modes &= ~ircd->protect_mode;
		}
	}

	if (flags & CA_AUTOOP)
	{
		if (!(noop || cu->modes & CMODE_OP))
		{
			modestack_mode_param(chansvs.nick, chan, MTYPE_ADD, 'o', CLIENT_NAME(u));
			cu->modes |= CMODE_OP;
		}
	}
	else if (secure && (cu->modes & CMODE_OP) && !(flags & CA_OP))
	{
		modestack_mode_param(chansvs.nick, chan, MTYPE_DEL, 'o', CLIENT_NAME(u));
		cu->modes &= ~CMODE_OP;
	}

	if (ircd->uses_halfops)
	{
		if (flags & CA_AUTOHALFOP)
		{
			if (!(noop || cu->modes & (CMODE_OP | ircd->halfops_mode)))
			{
				modestack_mode_param(chansvs.nick, chan, MTYPE_ADD, 'h', CLIENT_NAME(u));
				cu->modes |= ircd->halfops_mode;
			}
		}
		else if (secure && (cu->modes & ircd->halfops_mode) && !(flags & CA_HALFOP))
		{
			modestack_mode_param(chansvs.nick, chan, MTYPE_DEL, 'h', CLIENT_NAME(u));
			cu->modes &= ~ircd->halfops_mode;
		}
	}

	if (flags & CA_AUTOVOICE)
	{
		if (!(noop || cu->modes & (CMODE_OP | ircd->halfops_mode | CMODE_VOICE)))
		{
			modestack_mode_param(chansvs.nick, chan, MTYPE_ADD, 'v', CLIENT_NAME(u));
			cu->modes |= CMODE_VOICE;
		}
	}

	if (u->server->flags & SF_EOB && (md = metadata_find(mc, METADATA_CHANNEL, "private:entrymsg")))
		notice(chansvs.nick, cu->user->nick, "[%s] %s", mc->name, md->value);

	if (u->server->flags & SF_EOB && (md = metadata_find(mc, METADATA_CHANNEL, "url")))
		numeric_sts(me.name, 328, cu->user->nick, "%s :%s", mc->name, md->value);

	if (flags & CA_USEDUPDATE)
		mc->used = CURRTIME;
}

static void cs_part(hook_channel_joinpart_t *hdata)
{
	chanuser_t *cu;
	mychan_t *mc;

	cu = hdata->cu;
	if (cu == NULL)
		return;
	mc = mychan_find(cu->chan->name);
	if (mc == NULL)
		return;
	if (CURRTIME - mc->used >= 3600)
		if (chanacs_user_flags(mc, cu->user) & CA_USEDUPDATE)
			mc->used = CURRTIME;
	/*
	 * When channel_part is fired, we haven't yet removed the
	 * user from the room. So, the channel will have two members
	 * if ChanServ is joining channels: the triggering user and
	 * itself.
	 *
	 * Do not part if we're enforcing an akick/close in an otherwise
	 * empty channel (MC_INHABIT). -- jilles
	 */
	if ((mc->flags & MC_GUARD)
		&& config_options.leave_chans
		&& !(mc->flags & MC_INHABIT)
		&& (cu->chan->nummembers <= 2)
		&& !is_internal_client(cu->user))
		part(cu->chan->name, chansvs.nick);
}

static void cs_register(mychan_t *mc)
{
	if (mc->chan)
	{
		if (mc->flags & MC_GUARD)
			join(mc->name, chansvs.nick);

		check_modes(mc, TRUE);
	}
}

/* Called on every set of a topic, after updating our internal structures */
static void cs_keeptopic_topicset(channel_t *c)
{
	mychan_t *mc;
	metadata_t *md;

	mc = mychan_find(c->name);

	if (mc == NULL)
		return;

	md = metadata_find(mc, METADATA_CHANNEL, "private:topic:text");
	if (md != NULL)
	{
		if (c->topic != NULL && !strcmp(md->value, c->topic))
			return;
		metadata_delete(mc, METADATA_CHANNEL, "private:topic:text");
	}

	if (metadata_find(mc, METADATA_CHANNEL, "private:topic:setter"))
		metadata_delete(mc, METADATA_CHANNEL, "private:topic:setter");

	if (metadata_find(mc, METADATA_CHANNEL, "private:topic:ts"))
		metadata_delete(mc, METADATA_CHANNEL, "private:topic:ts");

	if (c->topic && c->topic_setter)
	{
		slog(LG_DEBUG, "KeepTopic: topic set for %s by %s: %s", c->name,
			c->topic_setter, c->topic);
		metadata_add(mc, METADATA_CHANNEL, "private:topic:setter",
			c->topic_setter);
		metadata_add(mc, METADATA_CHANNEL, "private:topic:text",
			c->topic);
		metadata_add(mc, METADATA_CHANNEL, "private:topic:ts",
			itoa(c->topicts));
	}
	else
		slog(LG_DEBUG, "KeepTopic: topic cleared for %s", c->name);
}

/* Called when a topic change is received from the network, before altering
 * our internal structures */
static void cs_topiccheck(hook_channel_topic_check_t *data)
{
	mychan_t *mc;
	unsigned int accessfl = 0;

	mc = mychan_find(data->c->name);
	if (mc == NULL)
		return;

	if ((mc->flags & (MC_KEEPTOPIC | MC_TOPICLOCK)) == (MC_KEEPTOPIC | MC_TOPICLOCK))
	{
		if (data->u == NULL || !((accessfl = chanacs_user_flags(mc, data->u)) & CA_TOPIC))
		{
			/* topic burst or unauthorized user, revert it */
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

/* Called on creation of a channel */
static void cs_newchan(channel_t *c)
{
	mychan_t *mc;
	chanuser_t *cu;
	metadata_t *md;
	char *setter;
	char *text;
	time_t channelts = 0;
	time_t topicts;
	char str[21];

	if (!(mc = mychan_find(c->name)))
		return;

	/* schedule a mode lock check when we know the current modes
	 * -- jilles */
	mc->flags |= MC_MLOCK_CHECK;

	md = metadata_find(mc, METADATA_CHANNEL, "private:channelts");
	if (md != NULL)
		channelts = atol(md->value);
	if (channelts == 0)
		channelts = mc->registered;

	if (chansvs.changets && c->ts > channelts && channelts > 0)
	{
		/* Stop the splitrider -- jilles */
		c->ts = channelts;
		clear_simple_modes(c);
		c->modes |= CMODE_NOEXT | CMODE_TOPIC;
		check_modes(mc, FALSE);
		/* No ops to clear */
		chan_lowerts(c, chansvs.me->me);
		cu = chanuser_add(c, CLIENT_NAME(chansvs.me->me));
		cu->modes |= CMODE_OP;
		/* make sure it parts again sometime (empty SJOIN etc) */
		mc->flags |= MC_INHABIT;
	}
	else if (c->ts != channelts)
	{
		snprintf(str, sizeof str, "%lu", (unsigned long)c->ts);
		metadata_add(mc, METADATA_CHANNEL, "private:channelts", str);
	}
	else if (!(MC_TOPICLOCK & mc->flags) && LIST_LENGTH(&c->members) == 0)
		/* Same channel, let's assume ircd has kept topic.
		 * However, if topiclock is enabled, we must change it back
		 * regardless.
		 * Also, if there is someone in this channel already, it is
		 * being created by a service and we must restore.
		 * -- jilles */
		return;

	if (!(MC_KEEPTOPIC & mc->flags))
		return;

	md = metadata_find(mc, METADATA_CHANNEL, "private:topic:setter");
	if (md == NULL)
		return;
	setter = md->value;

	md = metadata_find(mc, METADATA_CHANNEL, "private:topic:text");
	if (md == NULL)
		return;
	text = md->value;

	md = metadata_find(mc, METADATA_CHANNEL, "private:topic:ts");
	if (md == NULL)
		return;
	topicts = atol(md->value);

	handle_topic(c, setter, topicts, text);
	topic_sts(c, setter, topicts, 0, text);
}

static void cs_tschange(channel_t *c)
{
	mychan_t *mc;
	char str[21];

	if (!(mc = mychan_find(c->name)))
		return;

	/* store new TS */
	snprintf(str, sizeof str, "%lu", (unsigned long)c->ts);
	metadata_add(mc, METADATA_CHANNEL, "private:channelts", str);

	/* schedule a mode lock check when we know the new modes
	 * -- jilles */
	mc->flags |= MC_MLOCK_CHECK;
}

static void cs_leave_empty(void *unused)
{
	mychan_t *mc;
	dictionary_iteration_state_t state;

	(void)unused;
	DICTIONARY_FOREACH(mc, &state, mclist)
	{
		if (!(mc->flags & MC_INHABIT))
			continue;
		mc->flags &= ~MC_INHABIT;
		if (mc->chan != NULL &&
				(!config_options.chan || irccasecmp(mc->name, config_options.chan)) &&
				(!(mc->flags & MC_GUARD) ||
				 (config_options.leave_chans && mc->chan->nummembers == 1) ||
				 metadata_find(mc, METADATA_CHANNEL, "private:close:closer")) &&
				chanuser_find(mc->chan, chansvs.me->me))
		{
			slog(LG_DEBUG, "cs_leave_empty(): leaving %s", mc->chan->name);
			part(mc->chan->name, chansvs.nick);
		}
	}
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
