/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2010 Atheme Project (http://atheme.org/)
 * Copyright (C) 2014-2016 Austin Ellis <siniStar@IRC4Fun.net>
 * Copyright (C) 2018 Atheme Development Group (https://atheme.github.io/)
 *
 * chanfix - channel fixing service
 */

#include <atheme.h>
#include "chanfix.h"

bool chanfix_do_autofix;

static unsigned int
count_ops(struct channel *c)
{
	unsigned int i = 0;
	mowgli_node_t *n;

	return_val_if_fail(c != NULL, 0);

	MOWGLI_ITER_FOREACH(n, c->members.head)
	{
		struct chanuser *cu = n->data;

		if (cu->modes & CSTATUS_OP)
			i++;
	}

	return i;
}

static bool
chanfix_should_handle(struct chanfix_channel *cfchan, struct channel *c)
{
	struct mychan *mc;
	unsigned int n;

	return_val_if_fail(cfchan != NULL, false);

	if (c == NULL)
		return false;

	if ((mc = mychan_find(c->name)) != NULL)
		return false;

	n = count_ops(c);
	// enough ops, don't touch it
	if (n >= CHANFIX_OP_THRESHHOLD)
		return false;

	// Do not fix NOFIX'd channels
	if ((metadata_find(cfchan, "private:nofix:setter")) != NULL)
		return false;

	/* only start a fix for opless channels, and consider a fix done
	 * after CHANFIX_FIX_TIME if any ops were given
	 */
	if (n > 0 && (cfchan->fix_started == 0 ||
			CURRTIME - cfchan->fix_started > CHANFIX_FIX_TIME))
		return false;

	return true;
}

static unsigned int
chanfix_calculate_score(struct chanfix_oprecord *orec)
{
	unsigned int base;

	return_val_if_fail(orec != NULL, 0);

	base = orec->age;
	if (orec->entity != NULL)
		base *= CHANFIX_ACCOUNT_WEIGHT;

	return base;
}

static void
chanfix_lower_ts(struct chanfix_channel *chan)
{
	struct channel *ch;
	struct chanuser *cfu;
	mowgli_node_t *n;

	ch = chan->chan;
	if (ch == NULL)
		return;

	// Apply mode change locally only, chan_lowerts() will propagate.
	channel_mode_va(NULL, ch, 2, "-ilk", "*");

	chan->ts--;
	ch->ts = chan->ts;

	MOWGLI_ITER_FOREACH(n, ch->members.head)
	{
		struct chanuser *cu = n->data;
		cu->modes = 0;
	}

	chan_lowerts(ch, chanfix->me);
	cfu = chanuser_add(ch, CLIENT_NAME(chanfix->me));
	cfu->modes |= CSTATUS_OP;

	msg(chanfix->me->nick, chan->name, "I only joined to remove modes.");

	part(chan->name, chanfix->me->nick);
}

static unsigned int
chanfix_get_highscore(struct chanfix_channel *chan)
{
	unsigned int highscore = 0;
	mowgli_node_t *n;

	MOWGLI_ITER_FOREACH(n, chan->oprecords.head)
	{
		unsigned int score;
		struct chanfix_oprecord *orec = n->data;

		score = chanfix_calculate_score(orec);
		if (score > highscore)
			highscore = score;
	}

	return highscore;
}

static unsigned int
chanfix_get_threshold(struct chanfix_channel *chan)
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

static bool
chanfix_fix_channel(struct chanfix_channel *chan)
{
	struct channel *ch;
	mowgli_node_t *n;
	unsigned int threshold, opped = 0;

	ch = chan->chan;
	if (ch == NULL)
		return false;

	// op users who have X% of the highest score.
	threshold = chanfix_get_threshold(chan);
	MOWGLI_ITER_FOREACH(n, ch->members.head)
	{
		struct chanuser *cu = n->data;
		struct chanfix_oprecord *orec;
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

	// flush the modestacker.
	modestack_flush_channel(ch);

	// now report the damage
	msg(chanfix->me->nick, chan->name, "\2%u\2 clients should have been opped.", opped);

	// if this is the services log channel, continue to occupy it after the fix
	if (ch->flags & CHAN_LOG)
	return true;

	// fix done, leave.
	part(chan->name, chanfix->me->nick);

	return true;
}

static bool
chanfix_can_start_fix(struct chanfix_channel *chan)
{
	struct channel *ch;
	mowgli_node_t *n;
	unsigned int threshold;

	ch = chan->chan;
	if (ch == NULL)
		return false;

	threshold = chanfix_get_highscore(chan) * CHANFIX_FINAL_STEP;
	MOWGLI_ITER_FOREACH(n, ch->members.head)
	{
		struct chanuser *cu = n->data;
		struct chanfix_oprecord *orec;
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

static void
chanfix_clear_bans(struct channel *ch)
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
		struct chanban *cb = n->data;

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

void
chanfix_autofix_ev(void *unused)
{
	struct chanfix_channel *chan;
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

static void
chanfix_cmd_list(struct sourceinfo *si, int parc, char *parv[])
{
	struct chanfix_channel *chan;
	struct metadata *md, *mdnofix;
	char *markpattern = NULL, *nofixpattern = NULL;
	char buf[BUFSIZE];
	mowgli_patricia_iteration_state_t state;
	unsigned int matches = 0;
	bool marked = false, nofix = false, markmatch = false, nofixmatch = false;
	char *chanpattern = NULL;

	if (parv[0] != NULL)
		chanpattern = parv[0];

	MOWGLI_PATRICIA_FOREACH(chan, &state, chanfix_channels)
	{
		if (chanpattern != NULL && match(chanpattern, chan->name))
			continue;

		if (markpattern)
		{
			markmatch = false;
			md = metadata_find(chan, "private:mark:reason");
			if (md != NULL && !match(markpattern, md->value))
				markmatch = true;

			if (!markmatch)
				continue;
		}

		if (markmatch && !metadata_find(chan, "private:mark:setter"))
			continue;

		if (nofixpattern)
		{
			nofixmatch = false;
			md = metadata_find(chan, "private:nofix:reason");
			if (md != NULL && !match(nofixpattern, md->value))
				nofixmatch = true;

			if (!nofixmatch)
				continue;
		}

		if (nofixmatch && !metadata_find(chan, "private:nofix:setter"))
			continue;

		// in the future we could add a LIMIT parameter
		*buf = '\0';

		if (metadata_find(chan, "private:mark:setter")) {
			mowgli_strlcat(buf, "\2[marked]\2", BUFSIZE);
		}

		if (metadata_find(chan, "private:nofix:setter")) {
			mowgli_strlcat(buf, "\2[nofix]\2", BUFSIZE);
		}

		command_success_nodata(si, "- %s %s", chan->name, buf);
		matches++;
	}

	if (chanpattern == NULL)
		chanpattern = "*";
	logcommand(si, CMDLOG_ADMIN, "LIST: \2%s\2 (\2%u\2 matches)", chanpattern, matches);
	if (matches == 0)
		command_success_nodata(si, _("No channels matched criteria \2%s\2"), chanpattern);
	else
		command_success_nodata(si, ngettext(N_("\2%u\2 match for criteria \2%s\2."),
		                                    N_("\2%u\2 matches for criteria \2%s\2."), matches),
		                                    matches, chanpattern);
}

static void
chanfix_cmd_fix(struct sourceinfo *si, int parc, char *parv[])
{
	struct chanfix_channel *chan;
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

        // Do not fix NOFIX'd channels
        if ((metadata_find(chan, "private:nofix:setter")) != NULL)
	{
		command_fail(si, fault_nochange, _("\2%s\2 has NOFIX enabled."), parv[0]);
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

static int
chanfix_compare_records(mowgli_node_t *a, mowgli_node_t *b, void *unused)
{
	struct chanfix_oprecord *ta = a->data;
	struct chanfix_oprecord *tb = b->data;

	return tb->age - ta->age;
}

static void
chanfix_cmd_scores(struct sourceinfo *si, int parc, char *parv[])
{
	mowgli_node_t *n;
	struct chanfix_channel *chan;
	unsigned int i = 0;
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

	// sort records by score.
	mowgli_list_sort(&chan->oprecords, chanfix_compare_records, NULL);

	if (count > MOWGLI_LIST_LENGTH(&chan->oprecords))
		count = MOWGLI_LIST_LENGTH(&chan->oprecords);

	if (count == 0)
	{
		command_success_nodata(si, _("There are no scores in the CHANFIX database for \2%s\2."), chan->name);
		return;
	}

	command_success_nodata(si, _("Top \2%u\2 scores for \2%s\2 in the database:"), count, chan->name);

	/* TRANSLATORS: Adjust these numbers only if the translated column
	 * headers would exceed that length. Pay particular attention to
	 * also changing the numbers in the format string inside the loop
	 * below to match them, and beware that these format strings are
	 * shared across multiple files!
	 */
	command_success_nodata(si, _("%-8s %-50s %s"), _("Num"), _("Account/Hostmask"), _("Score"));
	command_success_nodata(si, "----------------------------------------------------------------");

	MOWGLI_ITER_FOREACH(n, chan->oprecords.head)
	{
		char buf[BUFSIZE];
		unsigned int score;
		struct chanfix_oprecord *orec = n->data;

		score = chanfix_calculate_score(orec);

		snprintf(buf, BUFSIZE, "%s@%s", orec->user, orec->host);

		command_success_nodata(si, _("%-8u %-50s %u"), ++i, orec->entity ? orec->entity->name : buf, score);
	}

	command_success_nodata(si, "----------------------------------------------------------------");
	command_success_nodata(si, _("End of \2SCORES\2 listing for \2%s\2."), chan->name);
}

static void
chanfix_cmd_info(struct sourceinfo *si, int parc, char *parv[])
{
	struct chanfix_oprecord *orec;
	struct chanfix_channel *chan;
	struct tm *tm;
	char strfbuf[BUFSIZE];
	unsigned int highscore = 0;
	struct metadata *md;

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

	// sort records by score.
	mowgli_list_sort(&chan->oprecords, chanfix_compare_records, NULL);

	command_success_nodata(si, _("Information on \2%s\2:"), chan->name);

	tm = localtime(&chan->ts);
	strftime(strfbuf, sizeof strfbuf, TIME_FORMAT, tm);

	command_success_nodata(si, _("Creation time: %s"), strfbuf);

	if (chan->oprecords.head != NULL)
	{
		orec = chan->oprecords.head->data;
		highscore = chanfix_calculate_score(orec);
	}

	command_success_nodata(si, _("Highest score: \2%u\2"), highscore);
	command_success_nodata(si, _("Usercount    : \2%zu\2"),
	                       chan->chan ? MOWGLI_LIST_LENGTH(&chan->chan->members) : 0);
	command_success_nodata(si, _("Initial step : \2%.0f%%\2 of \2%u\2 (\2%0.1f\2)"),
	                       (double) (CHANFIX_INITIAL_STEP * 100), highscore,
	                       (double) (highscore * CHANFIX_INITIAL_STEP));
	command_success_nodata(si, _("Current step : \2%u\2"), chanfix_get_threshold(chan));
	command_success_nodata(si, _("Final step   : \2%.0f%%\2 of \2%u\2 (\2%0.1f\2)"),
	                       (double) (CHANFIX_FINAL_STEP * 100), highscore,
	                       (double) (highscore * CHANFIX_FINAL_STEP));
	command_success_nodata(si, _("Needs fixing : \2%s\2"), chanfix_should_handle(chan, chan->chan) ? _("Yes") : _("No"));
	command_success_nodata(si, _("Now fixing   : \2%s\2"), chan->fix_started ? _("Yes") : _("No"));

	if ((md = metadata_find(chan, "private:mark:setter")) != NULL)
	{
		const char *setter = md->value;
		const char *reason;
		time_t ts;

		md = metadata_find(chan, "private:mark:reason");
		reason = md != NULL ? md->value : "unknown";

		md = metadata_find(chan, "private:mark:timestamp");
		ts = md != NULL ? atoi(md->value) : 0;

		tm = localtime(&ts);
		strftime(strfbuf, sizeof strfbuf, TIME_FORMAT, tm);

		command_success_nodata(si, _("%s was \2MARKED\2 by \2%s\2 on \2%s\2 (%s)."), chan->name, setter, strfbuf, reason);
	}

        if ((md = metadata_find(chan, "private:nofix:setter")) != NULL)
        {
                const char *setter = md->value;
                const char *reason;
                time_t ts;

                md = metadata_find(chan, "private:nofix:reason");
                reason = md != NULL ? md->value : "unknown";

                md = metadata_find(chan, "private:mark:timestamp");
                ts = md != NULL ? atoi(md->value) : 0;

                tm = localtime(&ts);
                strftime(strfbuf, sizeof strfbuf, TIME_FORMAT, tm);

                command_success_nodata(si, _("%s had \2NOFIX\2 set by %s on %s (%s)"), chan->name, setter, strfbuf, reason);
        }

	command_success_nodata(si, _("\2*** End of Info ***\2"));
}

// MARK <channel> ON|OFF [reason]
static void
chanfix_cmd_mark(struct sourceinfo *si, int parc, char *parv[])
{
	char *target = parv[0];
	char *action = parv[1];
	char *info = parv[2];
	struct chanfix_channel *chan;

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

// NOFIX <channel> ON|OFF [reason]
static void
chanfix_cmd_nofix(struct sourceinfo *si, int parc, char *parv[])
{
	char *target = parv[0];
	char *action = parv[1];
	char *info = parv[2];
	struct chanfix_channel *chan;

	if (!target || !action)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "NOFIX");
		command_fail(si, fault_needmoreparams, _("Usage: NOFIX <#channel> <ON|OFF> [reason]"));
		return;
	}

	if (target[0] != '#')
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "NOFIX");
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
			command_fail(si, fault_needmoreparams, _("Usage: NOFIX <#channel> ON <reason>"));
			return;
		}

		if (metadata_find(chan, "private:nofix:setter"))
		{
			command_fail(si, fault_nochange, _("\2%s\2 already has NOFIX set."), target);
			return;
		}

		metadata_add(chan, "private:nofix:setter", get_oper_name(si));
		metadata_add(chan, "private:nofix:reason", info);
		metadata_add(chan, "private:nofix:timestamp", number_to_string(CURRTIME));

		logcommand(si, CMDLOG_ADMIN, "NOFIX:ON: \2%s\2 (reason: \2%s\2)", chan->name, info);
		command_success_nodata(si, _("\2%s\2 is now set to NOFIX."), target);
	}
	else if (!strcasecmp(action, "OFF"))
	{
		if (!metadata_find(chan, "private:nofix:setter"))
		{
			command_fail(si, fault_nochange, _("\2%s\2 is not set to NOFIX."), target);
			return;
		}

		metadata_delete(chan, "private:nofix:setter");
		metadata_delete(chan, "private:nofix:reason");
		metadata_delete(chan, "private:nofix:timestamp");

		logcommand(si, CMDLOG_ADMIN, "NOFIX:OFF: \2%s\2", chan->name);
		command_success_nodata(si, _("\2%s\2 is no longer set to NOFIX."), target);
	}
	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "NOFIX");
		command_fail(si, fault_badparams, _("Usage: NOFIX <#channel> <ON|OFF> <reason>"));
	}
}

// HELP <command> [params]
static void
chanfix_cmd_help(struct sourceinfo *si, int parc, char *parv[])
{
	if (parv[0])
	{
		(void) help_display(si, si->service, parv[0], si->service->commands);
		return;
	}

	(void) help_display_prefix(si, si->service);

	(void) command_success_nodata(si, _("\2%s\2 allows for simple channel operator management."),
	                              si->service->nick);

	(void) help_display_newline(si);
	(void) command_help(si, si->service->commands);
	(void) help_display_moreinfo(si, si->service, NULL);
	(void) help_display_locations(si);
	(void) help_display_suffix(si);
}

void
chanfix_can_register(struct hook_channel_register_check *req)
{
	struct chanfix_channel *chan;
	struct chanfix_oprecord *orec;
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

struct command cmd_list = {
	.name           = "LIST",
	.desc           = N_("List all channels with CHANFIX records."),
	.access         = PRIV_CHAN_AUSPEX,
	.maxparc        = 1,
	.cmd            = &chanfix_cmd_list,
	.help           = { .path = "chanfix/list" },
};

struct command cmd_chanfix = {
	.name           = "CHANFIX",
	.desc           = N_("Manually chanfix a channel."),
	.access         = PRIV_CHAN_ADMIN,
	.maxparc        = 1,
	.cmd            = &chanfix_cmd_fix,
	.help           = { .path = "chanfix/chanfix" },
};

struct command cmd_scores = {
	.name           = "SCORES",
	.desc           = N_("List channel scores."),
	.access         = PRIV_CHAN_AUSPEX,
	.maxparc        = 1,
	.cmd            = &chanfix_cmd_scores,
	.help           = { .path = "chanfix/scores" },
};

struct command cmd_info = {
	.name           = "INFO",
	.desc           = N_("List information on channel."),
	.access         = PRIV_CHAN_AUSPEX,
	.maxparc        = 1,
	.cmd            = &chanfix_cmd_info,
	.help           = { .path = "chanfix/info" },
};

struct command cmd_mark = {
	.name           = "MARK",
	.desc           = N_("Adds a note to a channel."),
	.access         = PRIV_MARK,
	.maxparc        = 3,
	.cmd            = &chanfix_cmd_mark,
	.help           = { .path = "chanfix/mark" },
};

struct command cmd_nofix = {
	.name           = "NOFIX",
	.desc           = N_("Adds NOFIX to a channel."),
	.access         = PRIV_CHAN_ADMIN,
	.maxparc        = 3,
	.cmd            = &chanfix_cmd_nofix,
	.help           = { .path = "chanfix/nofix" },
};

struct command cmd_help = {
	.name           = "HELP",
	.desc           = STR_HELP_DESCRIPTION,
	.access         = AC_NONE,
	.maxparc        = 1,
	.cmd            = &chanfix_cmd_help,
	.help           = { .path = "help" },
};
