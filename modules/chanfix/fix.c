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

bool chanfix_should_handle(chanfix_channel_t *cfchan, channel_t *c)
{
	mychan_t *mc;

	return_val_if_fail(cfchan != NULL, false);

	if (c == NULL)
		return false;

	if ((mc = mychan_find(c->name)) != NULL)
		return false;

	if (MOWGLI_LIST_LENGTH(&c->members) < CHANFIX_OP_THRESHHOLD)
		return false;

	if (count_ops(c) >= CHANFIX_OP_THRESHHOLD)
		return false;

	return true;
}

unsigned int chanfix_calculate_score(chanfix_oprecord_t *orec)
{
	unsigned int base;

	return_val_if_fail(orec != NULL, 0);

	base = orec->age;
	if (orec->entity != NULL)
		base *= CHANFIX_ACCOUNT_WEIGHT;

	return base;
}

void chanfix_lower_ts(chanfix_channel_t *chan)
{
	channel_t *ch;
	chanuser_t *cfu;
	mowgli_node_t *n;

	ch = chan->chan;
	if (ch == NULL)
		return;

	chan->ts--;
	ch->ts = chan->ts;

	MOWGLI_ITER_FOREACH(n, ch->members.head)
	{
		chanuser_t *cu = n->data;
		chanfix_oprecord_t *orec;
		unsigned int score;

		if (cu->modes & CSTATUS_OP)
			cu->modes &= ~CSTATUS_OP;
	}

	chan_lowerts(ch, chanfix->me);
	cfu = chanuser_add(ch, CLIENT_NAME(chanfix->me));
	cfu->modes |= CSTATUS_OP;

	msg(chanfix->me->nick, chan->name, "I only joined to remove modes.");

	part(chan->name, chanfix->me->nick);
}

void chanfix_fix_channel(chanfix_channel_t *chan)
{
	channel_t *ch;
	mowgli_node_t *n;
	unsigned int highscore = 0, opped = 0;

	ch = chan->chan;
	if (ch == NULL)
		return;

	/* join the channel */
	join(chan->name, chanfix->me->nick);

	/* find the highest score */
	MOWGLI_ITER_FOREACH(n, ch->members.head)
	{
		chanuser_t *cu = n->data;
		chanfix_oprecord_t *orec;
		unsigned int score;

		if (cu->user == chanfix->me)
			continue;

		orec = chanfix_oprecord_find(chan, cu->user);
		if (orec == NULL)
			continue;

		score = chanfix_calculate_score(orec);

		if (score > highscore)
			highscore = score;
	}

	/* now op users who have X% of that score. */
	MOWGLI_ITER_FOREACH(n, ch->members.head)
	{
		chanuser_t *cu = n->data;
		chanfix_oprecord_t *orec;
		unsigned int score;

		if (cu->user == chanfix->me)
			continue;

		orec = chanfix_oprecord_find(chan, cu->user);
		if (orec == NULL)
			continue;

		score = chanfix_calculate_score(orec);

		if (score > (highscore * chan->step))
		{
			modestack_mode_param(chanfix->me->nick, chan->chan, MTYPE_ADD, 'o', CLIENT_NAME(cu->user));
			cu->modes |= CSTATUS_OP;
			opped++;
		}
	}

	/* flush the modestacker. */
	modestack_flush_channel(ch);

	/* now report the damage */
	msg(chanfix->me->nick, chan->name, "\2%d\2 client%s should have been opped.", opped, opped != 1 ? "s" : "");

	/* fix done, leave. */
	part(chan->name, chanfix->me->nick);
}

/*************************************************************************************/

void chanfix_autofix_ev(void *unused)
{
	chanfix_channel_t *chan;
	mowgli_patricia_iteration_state_t state;

	MOWGLI_PATRICIA_FOREACH(chan, &state, chanfix_channels)
	{
		if (chanfix_should_handle(chan, chan->chan))
		{
			slog(LG_DEBUG, "chanfix_autofix_ev(): fixing %s automatically.", chan->name);
			chanfix_fix_channel(chan);

			chan->step -= CHANFIX_STEP_SIZE;
			if (chan->step < CHANFIX_FINAL_STEP)
				chan->step = CHANFIX_FINAL_STEP;
		}
		else
			chan->step = CHANFIX_INITIAL_STEP;
	}
}

/*************************************************************************************/

static void chanfix_cmd_fix(sourceinfo_t *si, int parc, char *parv[])
{
	chanfix_channel_t *chan;

	if (parv[0] == NULL)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "CHANFIX");
		command_fail(si, fault_needmoreparams, _("To fix a channel: CHANFIX <#channel>"));
		return;
	}

	if ((chan = chanfix_channel_find(parv[0])) == NULL)
	{
		command_fail(si, fault_nosuch_target, _("No CHANFIX record available for \2%s\2; try again later."),
			     parv[0]);
		return;
	}

	chanfix_lower_ts(chan);

	command_success_nodata(si, _("Fix request has been acknowledged for \2%s\2."), parv[0]);
}

command_t cmd_chanfix = { "CHANFIX", N_("Manually chanfix a channel."), AC_NONE, 1, chanfix_cmd_fix, { .path = "" } };

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
	int count = 20, i;

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

		command_success_nodata(si, "%-3d %-50s %d", i + 1, orec->entity ? orec->entity->name : buf, score);
	}

	command_success_nodata(si, "%-3s %-50s %s", "---", "--------------------------------------------------", "-----");
	command_success_nodata(si, _("End of \2SCORES\2 listing for \2%s\2."), chan->name);
}

command_t cmd_scores = { "SCORES", N_("List channel scores."), AC_NONE, 1, chanfix_cmd_scores, { .path = "" } };

static void chanfix_cmd_info(sourceinfo_t *si, int parc, char *parv[])
{
	mowgli_node_t *n;
	chanfix_oprecord_t *orec;
	chanfix_channel_t *chan;
	struct tm tm;
	char strfbuf[BUFSIZE];
	unsigned int highscore = 0;

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
	strftime(strfbuf, sizeof(strfbuf) - 1, config_options.time_format, &tm);

	command_success_nodata(si, _("Creation time: %s"), strfbuf);

	if (chan->oprecords.head != NULL)
	{
		orec = chan->oprecords.head->data;
		highscore = chanfix_calculate_score(orec);
	}

	command_success_nodata(si, _("Highest score: \2%u\2"), highscore);
	command_success_nodata(si, _("Usercount    : \2%zu\2"), chan->chan ? MOWGLI_LIST_LENGTH(&chan->chan->members) : 0);
	command_success_nodata(si, _("Initial step : \2%.0f%%\2 of \2%u\2 (\2%0.1f\2)"), CHANFIX_INITIAL_STEP * 100, highscore, (highscore * CHANFIX_INITIAL_STEP));
	command_success_nodata(si, _("Current step : \2%.0f%%\2 of \2%u\2 (\2%0.1f\2)"), chan->step * 100, highscore, (highscore * chan->step));
	command_success_nodata(si, _("Final step   : \2%.0f%%\2 of \2%u\2 (\2%0.1f\2)"), CHANFIX_FINAL_STEP * 100, highscore, (highscore * CHANFIX_FINAL_STEP));
	command_success_nodata(si, _("Needs fixing : \2%s\2"), chanfix_should_handle(chan, chan->chan) ? "YES" : "NO");

	command_success_nodata(si, _("\2*** End of Info ***\2"));
}

command_t cmd_info = { "INFO", N_("List information on channel."), AC_NONE, 1, chanfix_cmd_info, { .path = "" } };
