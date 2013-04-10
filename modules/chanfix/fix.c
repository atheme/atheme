/* chanfix - channel fixing service
 * Copyright (c) 2010 Atheme Development Group
 */

#include "atheme.h"
#include "chanfix.h"

static unsigned int count_ops(channel_t *c)
{
	unsigned int i = 0;
	mowgli_node_t *n;

	return_val_if_fail(c != NULL, 0);

	MOWGLI_ITER_FOREACH(n, c->members.head)
	{
		chanuser_t *cu = n->data;

		if (cu->modes & CSTATUS_OP)
			i++;
	}

	return i;
}

static bool chanfix_should_handle(chanfix_channel_t *cfchan, channel_t *c)
{
	mychan_t *mc;
	unsigned int n;

	return_val_if_fail(cfchan != NULL, false);

	if (c == NULL)
		return false;

	if ((mc = mychan_find(c->name)) != NULL)
		return false;

	n = count_ops(c);
	/* enough ops, don't touch it */
	if (n >= CHANFIX_OP_THRESHHOLD)
		return false;
	/* only start a fix for opless channels, and consider a fix done
	 * after CHANFIX_FIX_TIME if any ops were given
	 */
	if (n > 0 && (cfchan->fix_started == 0 ||
			CURRTIME - cfchan->fix_started > CHANFIX_FIX_TIME))
		return false;

	return true;
}

static unsigned int chanfix_calculate_score(chanfix_oprecord_t *orec)
{
	unsigned int base;

	return_val_if_fail(orec != NULL, 0);

	base = orec->age;
	if (orec->entity != NULL)
		base *= CHANFIX_ACCOUNT_WEIGHT;

	return base;
}

static void chanfix_lower_ts(chanfix_channel_t *chan)
{
	channel_t *ch;
	chanuser_t *cfu;
	mowgli_node_t *n;

	ch = chan->chan;
	if (ch == NULL)
		return;

	/* Apply mode change locally only, chan_lowerts() will propagate. */
	channel_mode_va(NULL, ch, 2, "-ilk", "*");

	chan->ts--;
	ch->ts = chan->ts;

	MOWGLI_ITER_FOREACH(n, ch->members.head)
	{
		chanuser_t *cu = n->data;
		cu->modes = 0;
	}

	chan_lowerts(ch, chanfix->me);
	cfu = chanuser_add(ch, CLIENT_NAME(chanfix->me));
	cfu->modes |= CSTATUS_OP;

	msg(chanfix->me->nick, chan->name, "I only joined to remove modes.");

	part(chan->name, chanfix->me->nick);
}

static unsigned int chanfix_get_highscore(chanfix_channel_t *chan)
{
	unsigned int highscore = 0;
	mowgli_node_t *n;

	MOWGLI_ITER_FOREACH(n, chan->oprecords.head)
	{
		unsigned int score;
		chanfix_oprecord_t *orec = n->data;

		score = chanfix_calculate_score(orec);
		if (score > highscore)
			highscore = score;
	}

	return highscore;
}

static unsigned int chanfix_get_threshold(chanfix_channel_t *chan)
{
	unsigned int highscore, t, threshold;

	highscore = chanfix_get_highscore(chan);

	t = CURRTIME - chan->fix_started;
	if (t > CHANFIX_FIX_TIME)
		t = CHANFIX_FIX_TIME;
	threshold = highscore * (CHANFIX_INITIAL_STEP +
			(CHANFIX_FINAL_STEP - CHANFIX_INITIAL_STEP) *
			t / CHANFIX_FIX_TIME);
	if (threshold == 0)
		threshold = 1;
	return threshold;
}

static bool chanfix_fix_channel(chanfix_channel_t *chan)
{
	channel_t *ch;
	mowgli_node_t *n;
	unsigned int threshold, opped = 0;

	ch = chan->chan;
	if (ch == NULL)
		return false;

	/* op users who have X% of the highest score. */
	threshold = chanfix_get_threshold(chan);
	MOWGLI_ITER_FOREACH(n, ch->members.head)
	{
		chanuser_t *cu = n->data;
		chanfix_oprecord_t *orec;
		unsigned int score;

		if (cu->user == chanfix->me)
			continue;
		if (cu->modes & CSTATUS_OP)
			continue;

		orec = chanfix_oprecord_find(chan, cu->user);
		if (orec == NULL)
			continue;

		score = chanfix_calculate_score(orec);

		if (score >= threshold)
		{
			if (opped == 0)
				join(chan->name, chanfix->me->nick);
			modestack_mode_param(chanfix->me->nick, chan->chan, MTYPE_ADD, 'o', CLIENT_NAME(cu->user));
			cu->modes |= CSTATUS_OP;
			opped++;
		}
	}

	if (opped == 0)
		return false;

	/* flush the modestacker. */
	modestack_flush_channel(ch);

	/* now report the damage */
	msg(chanfix->me->nick, chan->name, "\2%d\2 client%s should have been opped.", opped, opped != 1 ? "s" : "");

	/* fix done, leave. */
	part(chan->name, chanfix->me->nick);

	return true;
}

static bool chanfix_can_start_fix(chanfix_channel_t *chan)
{
	channel_t *ch;
	mowgli_node_t *n;
	unsigned int threshold;

	ch = chan->chan;
	if (ch == NULL)
		return false;

	threshold = chanfix_get_highscore(chan) * CHANFIX_FINAL_STEP;
	MOWGLI_ITER_FOREACH(n, ch->members.head)
	{
		chanuser_t *cu = n->data;
		chanfix_oprecord_t *orec;
		unsigned int score;

		if (cu->user == chanfix->me)
			continue;
		if (cu->modes & CSTATUS_OP)
			return false;

		orec = chanfix_oprecord_find(chan, cu->user);
		if (orec == NULL)
			continue;

		score = chanfix_calculate_score(orec);
		if (score >= threshold)
			return true;
	}
	return false;
}

static void chanfix_clear_bans(channel_t *ch)
{
	bool joined = false;
	mowgli_node_t *n, *tn;

	return_if_fail(ch != NULL);

	if (ch->modes & CMODE_INVITE)
	{
		if (!joined)
		{
			joined = true;
			join(ch->name, chanfix->me->nick);
		}
		channel_mode_va(chanfix->me, ch, 1, "-i");
	}
	if (ch->limit > 0)
	{
		if (!joined)
		{
			joined = true;
			join(ch->name, chanfix->me->nick);
		}
		channel_mode_va(chanfix->me, ch, 1, "-l");
	}
	if (ch->key != NULL)
	{
		if (!joined)
		{
			joined = true;
			join(ch->name, chanfix->me->nick);
		}
		channel_mode_va(chanfix->me, ch, 2, "-k", "*");
	}
	MOWGLI_ITER_FOREACH_SAFE(n, tn, ch->bans.head)
	{
		chanban_t *cb = n->data;

		if (cb->type != 'b')
			continue;

		if (!joined)
		{
			joined = true;
			join(ch->name, chanfix->me->nick);
		}
		modestack_mode_param(chanfix->me->nick, ch, MTYPE_DEL,
				'b', cb->mask);
		chanban_delete(cb);
	}
	if (!joined)
		return;

	modestack_flush_channel(ch);
	msg(chanfix->me->nick, ch->name, "I only joined to remove modes.");
	part(ch->name, chanfix->me->nick);
}

/*************************************************************************************/

bool chanfix_do_autofix;

void chanfix_autofix_ev(void *unused)
{
	chanfix_channel_t *chan;
	mowgli_patricia_iteration_state_t state;

	MOWGLI_PATRICIA_FOREACH(chan, &state, chanfix_channels)
	{
		if (!chanfix_do_autofix && !chan->fix_requested)
			continue;

		if (chanfix_should_handle(chan, chan->chan))
		{
			if (chan->fix_started == 0)
			{
				if (chanfix_can_start_fix(chan))
				{
					slog(LG_INFO, "chanfix_autofix_ev(): fixing %s automatically.", chan->name);
					chan->fix_started = CURRTIME;
					/* If we are opping some users
					 * immediately, they can handle it.
					 * Otherwise, remove bans to allow
					 * users with higher scores to join.
					 */
					if (!chanfix_fix_channel(chan))
						chanfix_clear_bans(chan->chan);
				}
				else
				{
					/* No scored ops yet, remove bans
					 * to allow them to join.
					 */
					chanfix_clear_bans(chan->chan);
				}
			}
			else
			{
				/* Continue trying to give ops.
				 * If the channel is still or again opless,
				 * remove bans to allow ops to join.
				 */
				if (!chanfix_fix_channel(chan) &&
						count_ops(chan->chan) == 0)
					chanfix_clear_bans(chan->chan);
			}
		}
		else
		{
			chan->fix_requested = false;
			chan->fix_started = 0;
		}
	}
}

/*************************************************************************************/

static void chanfix_cmd_fix(sourceinfo_t *si, int parc, char *parv[])
{
	chanfix_channel_t *chan;
	unsigned int highscore;

	if (parv[0] == NULL)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "CHANFIX");
		command_fail(si, fault_needmoreparams, _("To fix a channel: CHANFIX <#channel>"));
		return;
	}

	if (!channel_find(parv[0]))
	{
		command_fail(si, fault_nosuch_target, _("Channel \2%s\2 does not exist."), parv[0]);
		return;
	}

	if ((chan = chanfix_channel_find(parv[0])) == NULL)
	{
		command_fail(si, fault_nosuch_target, _("No CHANFIX record available for \2%s\2; try again later."),
			     parv[0]);
		return;
	}

	if (mychan_find(parv[0]))
	{
		command_fail(si, fault_nosuch_target, _("\2%s\2 is already registered."), parv[0]);
		return;
	}

	highscore = chanfix_get_highscore(chan);
	if (highscore < CHANFIX_MIN_FIX_SCORE)
	{
		command_fail(si, fault_nosuch_target, _("Scores for \2%s\2 are too low (< %u) for a fix."),
			     parv[0], CHANFIX_MIN_FIX_SCORE);
		return;
	}

	chanfix_lower_ts(chan);
	chan->fix_requested = true;

	logcommand(si, CMDLOG_ADMIN, "CHANFIX: \2%s\2", parv[0]);

	command_success_nodata(si, _("Fix request has been acknowledged for \2%s\2."), parv[0]);
}

command_t cmd_chanfix = { "CHANFIX", N_("Manually chanfix a channel."), PRIV_CHAN_ADMIN, 1, chanfix_cmd_fix, { .path = "chanfix/chanfix" } };

static int chanfix_compare_records(mowgli_node_t *a, mowgli_node_t *b, void *unused)
{
	chanfix_oprecord_t *ta = a->data;
	chanfix_oprecord_t *tb = b->data;

	return tb->age - ta->age;
}

static void chanfix_cmd_scores(sourceinfo_t *si, int parc, char *parv[])
{
	mowgli_node_t *n;
	chanfix_channel_t *chan;
	int i = 0;
	unsigned int count = 20;

	if (parv[0] == NULL)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SCORES");
		command_fail(si, fault_needmoreparams, _("To view CHANFIX scores for a channel: SCORES <#channel>"));
		return;
	}

	if ((chan = chanfix_channel_find(parv[0])) == NULL)
	{
		command_fail(si, fault_nosuch_target, _("No CHANFIX record available for \2%s\2; try again later."),
			     parv[0]);
		return;
	}

	/* sort records by score. */
	mowgli_list_sort(&chan->oprecords, chanfix_compare_records, NULL);

	if (count > MOWGLI_LIST_LENGTH(&chan->oprecords))
		count = MOWGLI_LIST_LENGTH(&chan->oprecords);

	if (count == 0)
	{
		command_success_nodata(si, _("There are no scores in the CHANFIX database for \2%s\2."), chan->name);
		return;
	}

	command_success_nodata(si, _("Top \2%d\2 scores for \2%s\2 in the database:"), count, chan->name);

	command_success_nodata(si, "%-3s %-50s %s", _("Num"), _("Account/Hostmask"), _("Score"));
	command_success_nodata(si, "%-3s %-50s %s", "---", "--------------------------------------------------", "-----");

	MOWGLI_ITER_FOREACH(n, chan->oprecords.head)
	{
		char buf[BUFSIZE];
		unsigned int score;
		chanfix_oprecord_t *orec = n->data;

		score = chanfix_calculate_score(orec);

		snprintf(buf, BUFSIZE, "%s@%s", orec->user, orec->host);

		command_success_nodata(si, "%-3d %-50s %d", ++i, orec->entity ? orec->entity->name : buf, score);
	}

	command_success_nodata(si, "%-3s %-50s %s", "---", "--------------------------------------------------", "-----");
	command_success_nodata(si, _("End of \2SCORES\2 listing for \2%s\2."), chan->name);
}

command_t cmd_scores = { "SCORES", N_("List channel scores."), PRIV_CHAN_AUSPEX, 1, chanfix_cmd_scores, { .path = "chanfix/scores" } };

static void chanfix_cmd_info(sourceinfo_t *si, int parc, char *parv[])
{
	chanfix_oprecord_t *orec;
	chanfix_channel_t *chan;
	struct tm tm;
	char strfbuf[BUFSIZE];
	unsigned int highscore = 0;
	metadata_t *md;

	if (parv[0] == NULL)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SCORES");
		command_fail(si, fault_needmoreparams, _("To view CHANFIX scores for a channel: SCORES <#channel>"));
		return;
	}

	if ((chan = chanfix_channel_find(parv[0])) == NULL)
	{
		command_fail(si, fault_nosuch_target, _("No CHANFIX record available for \2%s\2; try again later."),
			     parv[0]);
		return;
	}

	/* sort records by score. */
	mowgli_list_sort(&chan->oprecords, chanfix_compare_records, NULL);

	command_success_nodata(si, _("Information on \2%s\2:"), chan->name);

	tm = *localtime(&chan->ts);
	strftime(strfbuf, sizeof strfbuf, TIME_FORMAT, &tm);

	command_success_nodata(si, _("Creation time: %s"), strfbuf);

	if (chan->oprecords.head != NULL)
	{
		orec = chan->oprecords.head->data;
		highscore = chanfix_calculate_score(orec);
	}

	command_success_nodata(si, _("Highest score: \2%u\2"), highscore);
	command_success_nodata(si, _("Usercount    : \2%zu\2"), chan->chan ? MOWGLI_LIST_LENGTH(&chan->chan->members) : 0);
	command_success_nodata(si, _("Initial step : \2%.0f%%\2 of \2%u\2 (\2%0.1f\2)"), CHANFIX_INITIAL_STEP * 100, highscore, (highscore * CHANFIX_INITIAL_STEP));
	command_success_nodata(si, _("Current step : \2%u\2"), chanfix_get_threshold(chan));
	command_success_nodata(si, _("Final step   : \2%.0f%%\2 of \2%u\2 (\2%0.1f\2)"), CHANFIX_FINAL_STEP * 100, highscore, (highscore * CHANFIX_FINAL_STEP));
	command_success_nodata(si, _("Needs fixing : \2%s\2"), chanfix_should_handle(chan, chan->chan) ? "YES" : "NO");
	command_success_nodata(si, _("Now fixing   : \2%s\2"),
			chan->fix_started ? "YES" : "NO");

	if ((md = metadata_find(chan, "private:mark:setter")) != NULL)
	{
		const char *setter = md->value;
		const char *reason;
		time_t ts;

		md = metadata_find(chan, "private:mark:reason");
		reason = md != NULL ? md->value : "unknown";

		md = metadata_find(chan, "private:mark:timestamp");
		ts = md != NULL ? atoi(md->value) : 0;

		tm = *localtime(&ts);
		strftime(strfbuf, sizeof strfbuf, TIME_FORMAT, &tm);

		command_success_nodata(si, _("%s was \2MARKED\2 by %s on %s (%s)"), chan->name, setter, strfbuf, reason);
	}

	command_success_nodata(si, _("\2*** End of Info ***\2"));
}

command_t cmd_info = { "INFO", N_("List information on channel."), PRIV_CHAN_AUSPEX, 1, chanfix_cmd_info, { .path = "chanfix/info" } };

/* MARK <channel> ON|OFF [reason] */
static void chanfix_cmd_mark(sourceinfo_t *si, int parc, char *parv[])
{
	char *target = parv[0];
	char *action = parv[1];
	char *info = parv[2];
	chanfix_channel_t *chan;

	if (!target || !action)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "MARK");
		command_fail(si, fault_needmoreparams, _("Usage: MARK <#channel> <ON|OFF> [note]"));
		return;
	}

	if (target[0] != '#')
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "MARK");
		return;
	}

	if ((chan = chanfix_channel_find(parv[0])) == NULL)
	{
		command_fail(si, fault_nosuch_target, _("No CHANFIX record available for \2%s\2; try again later."),
			     parv[0]);
		return;
	}

	if (!strcasecmp(action, "ON"))
	{
		if (!info)
		{
			command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "MARK");
			command_fail(si, fault_needmoreparams, _("Usage: MARK <#channel> ON <note>"));
			return;
		}

		if (metadata_find(chan, "private:mark:setter"))
		{
			command_fail(si, fault_nochange, _("\2%s\2 is already marked."), target);
			return;
		}

		metadata_add(chan, "private:mark:setter", get_oper_name(si));
		metadata_add(chan, "private:mark:reason", info);
		metadata_add(chan, "private:mark:timestamp", number_to_string(CURRTIME));

		logcommand(si, CMDLOG_ADMIN, "MARK:ON: \2%s\2 (reason: \2%s\2)", chan->name, info);
		command_success_nodata(si, _("\2%s\2 is now marked."), target);
	}
	else if (!strcasecmp(action, "OFF"))
	{
		if (!metadata_find(chan, "private:mark:setter"))
		{
			command_fail(si, fault_nochange, _("\2%s\2 is not marked."), target);
			return;
		}

		metadata_delete(chan, "private:mark:setter");
		metadata_delete(chan, "private:mark:reason");
		metadata_delete(chan, "private:mark:timestamp");

		logcommand(si, CMDLOG_ADMIN, "MARK:OFF: \2%s\2", chan->name);
		command_success_nodata(si, _("\2%s\2 is now unmarked."), target);
	}
	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "MARK");
		command_fail(si, fault_badparams, _("Usage: MARK <#channel> <ON|OFF> [note]"));
	}
}

command_t cmd_mark = { "MARK", N_("Adds a note to a channel."), PRIV_MARK, 3, chanfix_cmd_mark, { .path = "chanfix/mark" } };

/* HELP <command> [params] */
static void chanfix_cmd_help(sourceinfo_t *si, int parc, char *parv[])
{
	char *command = parv[0];

	if (!command)
	{
		command_success_nodata(si, _("***** \2%s Help\2 *****"), si->service->nick);
		command_success_nodata(si, _("\2%s\2 allows for simple channel operator management."), si->service->nick);
		command_success_nodata(si, " ");
		command_success_nodata(si, _("For more information on a command, type:"));
		command_success_nodata(si, "\2/%s%s help <command>\2", (ircd->uses_rcommand == false) ? "msg " : "", si->service->disp);
		command_success_nodata(si, " ");

		command_help(si, si->service->commands);

		command_success_nodata(si, _("***** \2End of Help\2 *****"));
		return;
	}

	/* take the command through the hash table */
	help_display(si, si->service, command, si->service->commands);
}

command_t cmd_help = { "HELP", N_(N_("Displays contextual help information.")), AC_NONE, 1, chanfix_cmd_help, { .path = "help" } };

void chanfix_can_register(hook_channel_register_check_t *req)
{
	chanfix_channel_t *chan;
	chanfix_oprecord_t *orec;
	unsigned int highscore, score;

	return_if_fail(req != NULL);

	if (req->approved)
		return;

	chan = chanfix_channel_find(req->name);
	if (chan == NULL)
		return;
	highscore = chanfix_get_highscore(chan);
	if (highscore < CHANFIX_MIN_FIX_SCORE)
		return;

	if (req->si->su != NULL)
		orec = chanfix_oprecord_find(chan, req->si->su);
	else
		orec = NULL;
	score = orec != NULL ? chanfix_calculate_score(orec) : 0;

	if (score < highscore * CHANFIX_FINAL_STEP)
	{
		/* If /msg chanfix chanfix would work, only allow users it
		 * could possibly op to register the channel.
		 */
		if (has_priv(req->si, PRIV_CHAN_ADMIN))
		{
			slog(LG_INFO, "chanfix_can_register(): forced registration of %s by %s",
					req->name,
					req->si->smu != NULL ?
					entity(req->si->smu)->name : "??");
			return;
		}
		req->approved = 1;
		command_fail(req->si, fault_noprivs, _("Your chanfix score is too low to register \2%s\2."), req->name);
	}
}
