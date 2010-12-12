/* chanfix - channel fixing service
 * Copyright (c) 2010 Atheme Development Group
 */

#include "atheme.h"
#include "chanfix.h"

#define CHANFIX_OP_THRESHHOLD	3
#define CHANFIX_ACCOUNT_WEIGHT	1.5

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

	if ((mc = mychan_find(c->name)) != NULL)
		return false;

	if (MOWGLI_LIST_LENGTH(&cfchan->oprecords) < CHANFIX_OP_THRESHHOLD)
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

void chanfix_fix_channel(chanfix_channel_t *chan)
{
	channel_t *ch;
	chanuser_t *cfu;
	mowgli_node_t *n;
	unsigned int highscore = 0, opped = 0;

	ch = chan->chan;
	if (ch == NULL)
		return;

	/* first, lower the TS by one to kill all ops. */
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

		if (score > (highscore * 0.30))
		{
			modestack_mode_param(chanfix->me->nick, chan->chan, MTYPE_ADD, 'o', CLIENT_NAME(cu->user));
			cu->modes |= CSTATUS_OP;
			opped++;
		}
	}

	/* flush the modestacker. */
	modestack_flush_channel(ch);

	/* now report the damage */
	msg(chanfix->me->nick, chan->name, "\2%d\2 client(s) should have been opped; minimum required score was \2%0.1f\2.", opped, (highscore * 0.30));

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
		}
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

	chanfix_fix_channel(chan);

	command_success_nodata(si, "\2%s\2 was fixed.", parv[0]);
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
