/*
 * atheme-services: A collection of minimalist IRC services   
 * cmode.c: Channel mode change tracking.
 *
 * Copyright (c) 2005-2007 Atheme Project (http://www.atheme.org)           
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

/* convert mode flags to a text mode string */
char *flags_to_string(unsigned int flags)
{
	static char buf[32];
	char *s = buf;
	int i;

	for (i = 0; mode_list[i].mode != 0; i++)
		if (flags & mode_list[i].value)
			*s++ = mode_list[i].mode;

	*s = 0;

	return buf;
}

/* convert a mode character to a flag. */
int mode_to_flag(char c)
{
	int i;

	for (i = 0; mode_list[i].mode != 0 && mode_list[i].mode != c; i++);

	return mode_list[i].value;
}

static void reop_service(channel_t *chan, user_t *victim, user_t **pfirst_deopped_service)
{
	mowgli_node_t *n;

	if (*pfirst_deopped_service == NULL)
	{
		if (chan->nummembers > 1)
		{
			slog(LG_DEBUG, "channel_mode(): %s deopped on %s, rejoining", victim->nick, chan->name);
			part_sts(chan, victim);
			join_sts(chan, victim, false, channel_modes(chan, true));
		}
		else
		{
			slog(LG_DEBUG, "channel_mode(): %s deopped on %s, opping from other service", victim->nick, chan->name);
			MOWGLI_ITER_FOREACH(n, me.me->userlist.head)
			{
				if (n->data != victim)
				{
					modestack_mode_param(((user_t *)n->data)->nick, chan, MTYPE_ADD, 'o', CLIENT_NAME(victim));
					break;
				}
			}
		}
		*pfirst_deopped_service = victim;
	}
	else if (*pfirst_deopped_service != victim)
	{
		slog(LG_DEBUG, "channel_mode(): %s deopped on %s, opping from %s", victim->nick, chan->name, (*pfirst_deopped_service)->nick);
		modestack_mode_param((*pfirst_deopped_service)->nick, chan, MTYPE_ADD, 'o', CLIENT_NAME(victim));
	}
}

/* yeah, this should be fun. */
/* If source == NULL, apply a mode change from outside to our structures
 * If source != NULL, apply the mode change and send it out from that user
 */
void channel_mode(user_t *source, channel_t *chan, int parc, char *parv[])
{
	bool matched = false;
	bool simple_modes_changed = false;
	int i, parpos = 0, whatt = MTYPE_NUL;
	unsigned int newlimit;
	const char *pos = parv[0];
	mychan_t *mc;
	chanuser_t *cu = NULL;
	user_t *first_deopped_service = NULL;

	if ((!pos) || (*pos == '\0'))
		return;

	if (!chan)
		return;

	/* SJOIN modes of 0 means no change */
	if (*pos == '0')
		return;

	for (; *pos != '\0'; pos++)
	{
		matched = false;

		if (*pos == '+')
		{
			whatt = MTYPE_ADD;
			continue;
		}
		if (*pos == '-')
		{
			whatt = MTYPE_DEL;
			continue;
		}

		for (i = 0; mode_list[i].mode != '\0'; i++)
		{
			if (*pos == mode_list[i].mode)
			{
				matched = true;

				if (whatt == MTYPE_ADD)
				{
					if (!(chan->modes & mode_list[i].value))
						simple_modes_changed = true;
					chan->modes |= mode_list[i].value;
				}
				else
				{
					if (chan->modes & mode_list[i].value)
						simple_modes_changed = true;
					chan->modes &= ~mode_list[i].value;
				}

				if (source)
					modestack_mode_simple(source->nick, chan, whatt, mode_list[i].value);

				break;
			}
		}
		if (matched)
			continue;

		for (i = 0; ignore_mode_list[i].mode != '\0'; i++)
		{
			if (*pos == ignore_mode_list[i].mode)
			{
				matched = true;
				if (whatt == MTYPE_ADD)
				{
					if (++parpos >= parc)
						break;
					if (source && !ignore_mode_list[i].check(parv[parpos], chan, NULL, NULL, NULL))
						break;
					if (chan->extmodes[i])
					{
						if (strcmp(chan->extmodes[i], parv[parpos]))
							simple_modes_changed = true;
						free(chan->extmodes[i]);
					}
					else
						simple_modes_changed = true;
					chan->extmodes[i] = sstrdup(parv[parpos]);
					if (source)
						modestack_mode_ext(source->nick, chan, MTYPE_ADD, i, chan->extmodes[i]);
				}
				else
				{
					if (chan->extmodes[i])
					{
						simple_modes_changed = true;
						free(chan->extmodes[i]);
						chan->extmodes[i] = NULL;
					}
					if (source)
						modestack_mode_ext(source->nick, chan, MTYPE_DEL, i, NULL);
				}
				break;
			}
		}
		if (matched)
			continue;

		if (*pos == 'l')
		{
			if (whatt == MTYPE_ADD)
			{
				if (++parpos >= parc)
					continue;
				chan->modes |= CMODE_LIMIT;
				newlimit = atoi(parv[parpos]);
				if (chan->limit != newlimit)
					simple_modes_changed = true;
				chan->limit = newlimit;
				if (source)
					modestack_mode_limit(source->nick, chan, MTYPE_ADD, chan->limit);
			}
			else
			{
				chan->modes &= ~CMODE_LIMIT;
				if (chan->limit != 0)
					simple_modes_changed = true;
				chan->limit = 0;
				if (source)
					modestack_mode_limit(source->nick, chan, MTYPE_DEL, 0);
			}
			continue;
		}

		if (*pos == 'k')
		{
			if (whatt == MTYPE_ADD)
			{
				if (++parpos >= parc)
					continue;
				chan->modes |= CMODE_KEY;
				if (chan->key)
				{
					if (strcmp(chan->key, parv[parpos]))
						simple_modes_changed = true;
					free(chan->key);
				}
				else
					simple_modes_changed = true;
				chan->key = sstrdup(parv[parpos]);
				if (source)
					modestack_mode_param(source->nick, chan, MTYPE_ADD, 'k', chan->key);
			}
			else
			{
				if (chan->key)
					simple_modes_changed = true;
				chan->modes &= ~CMODE_KEY;
				if (source)
					modestack_mode_param(source->nick, chan, MTYPE_DEL, 'k', chan->key ? chan->key : "*");
				free(chan->key);
				chan->key = NULL;
				/* ratbox typically sends either the key or a `*' on -k, so you
				 * should eat a parameter
				 */
				parpos++;
			}
			continue;
		}

		if (strchr(ircd->ban_like_modes, *pos))
		{
			if (++parpos >= parc)
				continue;
			if (whatt == MTYPE_ADD)
			{
				chanban_add(chan, parv[parpos], *pos);
				if (source)
					modestack_mode_param(source->nick, chan, MTYPE_ADD, *pos, parv[parpos]);
			}
			else
			{
				chanban_t *c;

				c = chanban_find(chan, parv[parpos], *pos);
				chanban_delete(c);
				if (source)
					modestack_mode_param(source->nick, chan, MTYPE_DEL, *pos, parv[parpos]);
			}
			continue;
		}

		for (i = 0; status_mode_list[i].mode != '\0'; i++)
		{
			if (*pos == status_mode_list[i].mode)
			{
				if (++parpos >= parc)
					break;
				cu = chanuser_find(chan, source ? user_find_named(parv[parpos]) : user_find(parv[parpos]));

				if (cu == NULL)
				{
					slog(LG_ERROR, "channel_mode(): MODE %s %c%c %s", chan->name, (whatt == MTYPE_ADD) ? '+' : '-', status_mode_list[i].mode, parv[parpos]);
					break;
				}

				matched = true;

				if (whatt == MTYPE_ADD)
				{
					cu->modes |= status_mode_list[i].value;

					if (source)
						modestack_mode_param(source->nick, chan, MTYPE_ADD, *pos, CLIENT_NAME(cu->user));

					/* see if they did something we have to undo */
					if (source == NULL && cu->user->server != me.me && chansvs.me != NULL && (mc = mychan_find(chan->name)) && mc->flags & MC_SECURE)
					{
						if (status_mode_list[i].mode == 'o' && !(chanacs_user_flags(mc, cu->user) & (CA_OP | CA_AUTOOP)))
						{
							/* they were opped and aren't on the list, deop them */
							modestack_mode_param(chansvs.nick, chan, MTYPE_DEL, 'o', CLIENT_NAME(cu->user));
							cu->modes &= ~status_mode_list[i].value;
						}
						else if (ircd->uses_halfops && status_mode_list[i].mode == ircd->halfops_mchar[1] && !(chanacs_user_flags(mc, cu->user) & (CA_HALFOP | CA_AUTOHALFOP)))
						{
							/* same for halfops -- jilles */
							modestack_mode_param(chansvs.nick, chan, MTYPE_DEL, ircd->halfops_mchar[1], CLIENT_NAME(cu->user));
							cu->modes &= ~status_mode_list[i].value;
						}
					}
				}
				else
				{
					if (cu->user->server == me.me && status_mode_list[i].value == CSTATUS_OP)
					{
						if (source == NULL)
							reop_service(chan, cu->user, &first_deopped_service);
						continue;
					}

					if (source)
						modestack_mode_param(source->nick, chan, MTYPE_DEL, *pos, CLIENT_NAME(cu->user));

					cu->modes &= ~status_mode_list[i].value;
				}

				break;
			}
		}
		if (matched)
			continue;

		slog(LG_DEBUG, "channel_mode(): mode %c not matched", *pos);
	}

	if (source == NULL && chansvs.me != NULL)
	{
		mc = mychan_find(chan->name);
		if (mc != NULL && (simple_modes_changed ||
					(mc->flags & MC_MLOCK_CHECK)))
			check_modes(mc, true);
	}
}

/* like channel_mode() but parv array passed as varargs */
void channel_mode_va(user_t *source, channel_t *chan, int parc, char *parv0, ...)
{
	char *parv[255];
	int i;
	va_list va;

	if (parc == 0)
		return;
#if 0
	if (parc > sizeof parv / sizeof *parv)
	{
		slog(LG_DEBUG, "channel_mode_va(): parc too big (%d), truncating", parc);
		parc = sizeof parv / sizeof *parv;
	}
#endif
	parv[0] = parv0;
	va_start(va, parv0);
	for (i = 1; i < parc; i++)
		parv[i] = va_arg(va, char *);
	va_end(va);
	channel_mode(source, chan, parc, parv);
}

static struct modestackdata {
	char source[HOSTLEN]; /* name */
	channel_t *channel;
	unsigned int modes_on;
	unsigned int modes_off;
	unsigned int limit;
	char extmodes[256][512];
	bool limitused, extmodesused[256];
	char pmodes[2*MAXMODES+2];
	char params[512]; /* includes leading space */
	int totalparamslen; /* includes leading space */
	int totallen;
	int paramcount;

	unsigned int event;
} modestackdata;

static void modestack_calclen(struct modestackdata *md);

static void modestack_debugprint(struct modestackdata *md)
{
	size_t i;

	slog(LG_DEBUG, "modestack_debugprint(): %s MODE %s", md->source, md->channel->name);
	slog(LG_DEBUG, "simple %x/%x", md->modes_on, md->modes_off);
	if (md->limitused)
		slog(LG_DEBUG, "limit %u", (unsigned)md->limit);
	for (i = 0; i < ignore_mode_list_size; i++)
		if (md->extmodesused[i])
			slog(LG_DEBUG, "ext %d %s", (int)i, md->extmodes[i]);
	slog(LG_DEBUG, "pmodes %s%s", md->pmodes, md->params);
	modestack_calclen(md);
	slog(LG_DEBUG, "totallen %d/%d", md->totalparamslen, md->totallen);
}

/* calculates the length fields */
static void modestack_calclen(struct modestackdata *md)
{
	size_t i;
	const char *p;

	md->totallen = strlen(md->source) + USERLEN + HOSTLEN + 1 + 4 + 1 +
		10 + strlen(md->channel->name) + 1;
	md->totallen += 2 + 32 + strlen(md->pmodes);
	md->totalparamslen = 0;
	md->paramcount = (md->limitused != 0);
	if (md->limitused && md->limit != 0)
		md->totalparamslen += 11;
	for (i = 0; i < ignore_mode_list_size; i++)
		if (md->extmodesused[i])
		{
			md->paramcount++;
			if (*md->extmodes[i] != '\0')
				md->totalparamslen += 1 + strlen(md->extmodes[i]);
		}
	md->totalparamslen += strlen(md->params);
	p = md->params;
	while (*p != '\0')
		if (*p++ == ' ')
			md->paramcount++;
	md->totallen += md->totalparamslen;
}

/* clears the data */
static void modestack_clear(struct modestackdata *md)
{
	size_t i;

	md->modes_on = 0;
	md->modes_off = 0;
	md->limitused = 0;
	for (i = 0; i < ignore_mode_list_size; i++)
		md->extmodesused[i] = 0, *md->extmodes[i] = '\0';
	md->pmodes[0] = '\0';
	md->params[0] = '\0';
	md->totallen = 0;
	md->totalparamslen = 0;
	md->paramcount = 0;
}

/* sends a MODE and clears the data */
static void modestack_flush(struct modestackdata *md)
{
	char buf[512];
	char *end, *p;
	int dir = MTYPE_NUL;
	size_t i;

	p = buf;
	end = buf + sizeof buf;

	/* first do the mode letters */
	if (md->modes_off)
	{
		if (dir != MTYPE_DEL)
			dir = MTYPE_DEL, *p++ = '-';
		strlcpy(p, flags_to_string(md->modes_off), end - p);
		p += strlen(p);
	}
	if (md->limitused && md->limit == 0)
	{
		if (dir != MTYPE_DEL)
			dir = MTYPE_DEL, *p++ = '-';
		*p++ = 'l';
	}
	for (i = 0; i < ignore_mode_list_size; i++)
	{
		if (md->extmodesused[i] && *md->extmodes[i] == '\0')
		{
			if (dir != MTYPE_DEL)
				dir = MTYPE_DEL, *p++ = '-';
			*p++ = ignore_mode_list[i].mode;
		}
	}
	if (md->modes_on)
	{
		if (dir != MTYPE_ADD)
			dir = MTYPE_ADD, *p++ = '+';
		strlcpy(p, flags_to_string(md->modes_on), end - p);
		p += strlen(p);
	}
	if (md->limitused && md->limit != 0)
	{
		if (dir != MTYPE_ADD)
			dir = MTYPE_ADD, *p++ = '+';
		*p++ = 'l';
	}
	for (i = 0; i < ignore_mode_list_size; i++)
	{
		if (md->extmodesused[i] && *md->extmodes[i] != '\0')
		{
			if (dir != MTYPE_ADD)
				dir = MTYPE_ADD, *p++ = '+';
			*p++ = ignore_mode_list[i].mode;
		}
	}
	strlcpy(p, md->pmodes + ((dir == MTYPE_ADD && *md->pmodes == '+') || (dir == MTYPE_DEL && *md->pmodes == '-') ? 1 : 0), end - p);
	p += strlen(p);

	/* all mode letters done, now for some checks */
	if (p == buf)
	{
		/*slog(LG_DEBUG, "modestack_flush(): nothing to do");*/
		return;
	}
	if (p + md->totalparamslen >= end)
	{
		slog(LG_ERROR, "modestack_flush() overflow: %s", buf);
		modestack_debugprint(md);
		modestack_clear(md);
		return;
	}

	/* now the parameters, in the same order */
	if (md->limitused && md->limit != 0)
	{
		snprintf(p, end - p, " %u", (unsigned)md->limit);
		p += strlen(p);
	}
	for (i = 0; i < ignore_mode_list_size; i++)
	{
		if (md->extmodesused[i] && *md->extmodes[i] != '\0')
		{
			snprintf(p, end - p, " %s", md->extmodes[i]);
			p += strlen(p);
		}
	}
	if (*md->params)
	{
		strlcpy(p, md->params, end - p);
		p += strlen(p);
	}
	mode_sts(md->source, md->channel, buf);
	modestack_clear(md);
}

static struct modestackdata *modestack_init(const char *source, channel_t *channel)
{
	if (irccasecmp(source, modestackdata.source) || channel != modestackdata.channel)
	{
		/*slog(LG_DEBUG, "modestack_init(): new source/channel, flushing");*/
		modestack_flush(&modestackdata);
	}
	strlcpy(modestackdata.source, source, sizeof modestackdata.source);
	modestackdata.channel = channel;
	return &modestackdata;
}

static void modestack_add_simple(struct modestackdata *md, int dir, int flags)
{
	if (dir == MTYPE_ADD)
		md->modes_on |= flags, md->modes_off &= ~flags;
	else if (dir == MTYPE_DEL)
		md->modes_off |= flags, md->modes_on &= ~flags;
	else
		slog(LG_ERROR, "modestack_add_simple(): invalid direction");
}

static void modestack_add_limit(struct modestackdata *md, int dir, unsigned int limit)
{
	md->limitused = 0;
	modestack_calclen(md);
	if (md->paramcount >= MAXMODES)
		modestack_flush(md);
	if (dir == MTYPE_ADD)
	{
		if (md->totallen + 11 > 512)
			modestack_flush(md);
		md->limit = limit;
	}
	else if (dir == MTYPE_DEL)
		md->limit = 0;
	else
		slog(LG_ERROR, "modestack_add_limit(): invalid direction");
	md->limitused = 1;
}

static void modestack_add_ext(struct modestackdata *md, int dir, int i, const char *value)
{
	md->extmodesused[i] = 0;
	modestack_calclen(md);
	if (md->paramcount >= MAXMODES)
		modestack_flush(md);
	if (dir == MTYPE_ADD)
	{
		if (md->totallen + 1 + strlen(value) > 512)
			modestack_flush(md);
		strlcpy(md->extmodes[i], value, sizeof md->extmodes[i]);
	}
	else if (dir == MTYPE_DEL)
		md->extmodes[i][0] = '\0';
	else
		slog(LG_ERROR, "modestack_add_ext(): invalid direction");
	md->extmodesused[i] = 1;
}

static void modestack_add_param(struct modestackdata *md, int dir, char type, const char *value)
{
	char *p;
	int n = 0;
	size_t i;
	char dir2 = MTYPE_NUL;
	char str[3];

	p = md->pmodes;
	while (*p != '\0')
	{
		if (*p == '+')
			dir2 = MTYPE_ADD;
		else if (*p == '-')
			dir2 = MTYPE_DEL;
		else
			n++;
		p++;
	}
	n += (md->limitused != 0);
	for (i = 0; i < ignore_mode_list_size; i++)
		n += (md->extmodesused[i] != 0);
	modestack_calclen(md);
	if (n >= MAXMODES || md->totallen + (dir != dir2) + 2 + strlen(value) > 512 || (type == 'k' && strchr(md->pmodes, 'k')))
	{
		modestack_flush(md);
		dir2 = MTYPE_NUL;
	}
	if (dir != dir2)
	{
		str[0] = dir == MTYPE_ADD ? '+' : '-';
		str[1] = type;
		str[2] = '\0';
	}
	else
	{
		str[0] = type;
		str[1] = '\0';
	}
	strlcat(md->pmodes, str, sizeof md->pmodes);
	strlcat(md->params, " ", sizeof md->params);
	strlcat(md->params, value, sizeof md->params);
}

static void modestack_flush_callback(void *arg)
{
	modestack_flush((struct modestackdata *)arg);
	((struct modestackdata *)arg)->event = 0;
}

/* flush pending modes for a certain channel */
void modestack_flush_channel(channel_t *channel)
{
	if (channel == NULL || channel == modestackdata.channel)
		modestack_flush(&modestackdata);
}

/* forget pending modes for a certain channel */
void modestack_forget_channel(channel_t *channel)
{
	if (channel == NULL || channel == modestackdata.channel)
		modestack_clear(&modestackdata);
}

/* handle a channel that is going to be destroyed */
void modestack_finalize_channel(channel_t *channel)
{
	user_t *u;

	if (channel == modestackdata.channel)
	{
		if (modestackdata.modes_off & ircd->perm_mode)
		{
			/* A mode change is not a good way to destroy a channel */
			slog(LG_DEBUG, "modestack_finalize_channel(): flushing modes for %s to clear perm mode", channel->name);
			u = user_find_named(modestackdata.source);
			if (u != NULL)
				join_sts(channel, u, false, channel_modes(channel, true));
			modestack_flush(&modestackdata);
			if (u != NULL)
				part_sts(channel, u);
		}
		else
			modestack_clear(&modestackdata);
	}
}

/* stack simple modes without parameters */
void modestack_mode_simple_real(const char *source, channel_t *channel, int dir, int flags)
{
	struct modestackdata *md;

	if (flags == 0)
		return;
	md = modestack_init(source, channel);
	modestack_add_simple(md, dir, flags);
	if (!md->event)
		md->event = event_add_once("flush_cmode_callback", modestack_flush_callback, md, 0);
}
void (*modestack_mode_simple)(const char *source, channel_t *channel, int dir, int flags) = modestack_mode_simple_real;

/* stack a limit */
void modestack_mode_limit_real(const char *source, channel_t *channel, int dir, unsigned int limit)
{
	struct modestackdata *md;

	md = modestack_init(source, channel);
	modestack_add_limit(md, dir, limit);
	if (!md->event)
		md->event = event_add_once("flush_cmode_callback", modestack_flush_callback, md, 0);
}
void (*modestack_mode_limit)(const char *source, channel_t *channel, int dir, unsigned int limit) = modestack_mode_limit_real;

/* stack a non-standard type C mode */
void modestack_mode_ext_real(const char *source, channel_t *channel, int dir, unsigned int i, const char *value)
{
	struct modestackdata *md;

	md = modestack_init(source, channel);
	if (i >= ignore_mode_list_size)
	{
		slog(LG_ERROR, "modestack_mode_ext(): i=%d out of range (value=\"%s\")",
				i, value);
		return;
	}
	modestack_add_ext(md, dir, i, value);
	if (!md->event)
		md->event = event_add_once("flush_cmode_callback", modestack_flush_callback, md, 0);
}
void (*modestack_mode_ext)(const char *source, channel_t *channel, int dir, unsigned int i, const char *value) = modestack_mode_ext_real;

/* stack a type A, B or E mode */
void modestack_mode_param_real(const char *source, channel_t *channel, int dir, char type, const char *value)
{
	struct modestackdata *md;

	md = modestack_init(source, channel);
	modestack_add_param(md, dir, type, value);
	if (!md->event)
		md->event = event_add_once("flush_cmode_callback", modestack_flush_callback, md, 0);
}
void (*modestack_mode_param)(const char *source, channel_t *channel, int dir, char type, const char *value) = modestack_mode_param_real;

/* Clear all simple modes (+imnpstkl etc) on a channel */
void clear_simple_modes(channel_t *c)
{
	size_t i;

	if (c == NULL)
		return;
	c->modes = 0;
	c->limit = 0;
	if (c->key != NULL)
		free(c->key);
	c->key = NULL;
	for (i = 0; i < ignore_mode_list_size; i++)
		if (c->extmodes[i] != NULL)
		{
			free(c->extmodes[i]);
			c->extmodes[i] = NULL;
		}
}

char *channel_modes(channel_t *c, bool doparams)
{
	static char fullmode[512];
	char params[512];
	int i;
	char *p;
	char *q;

	if (c == NULL)
		return NULL;

	p = fullmode;
	q = params;
	*p++ = '+';
	*q = '\0';
	for (i = 0; mode_list[i].mode != '\0'; i++)
	{
		if (c->modes & mode_list[i].value)
			*p++ = mode_list[i].mode;
	}
	if (c->limit)
	{
		*p++ = 'l';
		if (doparams)
		{
			snprintf(q, params + sizeof params - q, " %d", c->limit);
			q += strlen(q);
		}
	}
	if (c->key)
	{
		*p++ = 'k';
		if (doparams)
		{
			*q++ = ' ';
			strlcpy(q, c->key, params + sizeof params - q);
			q += strlen(q);
		}
	}
	for (i = 0; ignore_mode_list[i].mode != '\0'; i++)
	{
		if (c->extmodes[i] != NULL)
		{
			*p++ = ignore_mode_list[i].mode;
			if (doparams)
			{
				*q++ = ' ';
				strlcpy(q, c->extmodes[i], params + sizeof params - q);
				q += strlen(q);
			}
		}
	}
	strlcpy(p, params, fullmode + sizeof fullmode - p);
	return fullmode;
}

void check_modes(mychan_t *mychan, bool sendnow)
{
	int modes;
	int i;
	metadata_t *md;
	char *p, *q;
	char str2[512];

	if (!mychan || !mychan->chan)
		return;
	mychan->flags &= ~MC_MLOCK_CHECK;

	/* check what's locked on */
	modes = ~mychan->chan->modes & mychan->mlock_on;
	modes &= ~(CMODE_KEY | CMODE_LIMIT);
	if (sendnow)
		modestack_mode_simple(chansvs.nick, mychan->chan, MTYPE_ADD, modes);
	mychan->chan->modes |= modes;

	if (mychan->mlock_limit && mychan->mlock_limit != mychan->chan->limit)
	{
		mychan->chan->limit = mychan->mlock_limit;
		if (sendnow)
			modestack_mode_limit(chansvs.nick, mychan->chan, MTYPE_ADD, mychan->mlock_limit);
	}

	if (mychan->mlock_key)
	{
		if (mychan->chan->key && strcmp(mychan->chan->key, mychan->mlock_key))
		{
			/* some ircds still need this... :\ -- jilles */
			if (sendnow)
				modestack_mode_param(chansvs.nick, mychan->chan, MTYPE_DEL, 'k', mychan->chan->key);
			free(mychan->chan->key);
			mychan->chan->key = NULL;
		}

		if (mychan->chan->key == NULL)
		{
			mychan->chan->key = sstrdup(mychan->mlock_key);
			if (sendnow)
				modestack_mode_param(chansvs.nick, mychan->chan, MTYPE_ADD, 'k', mychan->mlock_key);
		}
	}

	/* check what's locked off */
	modes = mychan->chan->modes & mychan->mlock_off;
	modes &= ~(CMODE_KEY | CMODE_LIMIT);
	if (sendnow)
		modestack_mode_simple(chansvs.nick, mychan->chan, MTYPE_DEL, modes);
	mychan->chan->modes &= ~modes;

	if (mychan->chan->limit && (mychan->mlock_off & CMODE_LIMIT))
	{
		if (sendnow)
			modestack_mode_limit(chansvs.nick, mychan->chan, MTYPE_DEL, 0);
		mychan->chan->limit = 0;
	}

	if (mychan->chan->key && (mychan->mlock_off & CMODE_KEY))
	{
		if (sendnow)
			modestack_mode_param(chansvs.nick, mychan->chan, MTYPE_DEL, 'k', mychan->chan->key);
		free(mychan->chan->key);
		mychan->chan->key = NULL;
	}

	/* non-standard type C modes separately */
	md = metadata_find(mychan, "private:mlockext");
	if (md != NULL)
	{
		p = md->value;
		while (*p != '\0')
		{
			for (i = 0; ignore_mode_list[i].mode != '\0'; i++)
			{
				if (ignore_mode_list[i].mode == *p)
				{
					if ((p[1] == ' ' || p[1] == '\0') && mychan->chan->extmodes[i] != NULL)
					{
						free(mychan->chan->extmodes[i]);
						mychan->chan->extmodes[i] = NULL;
						if (sendnow)
							modestack_mode_ext(chansvs.nick, mychan->chan, MTYPE_DEL, i, NULL);
					}
					else if (p[1] != ' ' && p[1] != '\0')
					{
						strlcpy(str2, p + 1, sizeof str2);
						q = strchr(str2, ' ');
						if (q != NULL)
							*q = '\0';
						if ((mychan->chan->extmodes[i] == NULL || strcmp(mychan->chan->extmodes[i], str2)) && ignore_mode_list[i].check(str2, mychan->chan, mychan, NULL, NULL))
						{
							if (mychan->chan->extmodes[i] != NULL)
								free(mychan->chan->extmodes[i]);
							mychan->chan->extmodes[i] = sstrdup(str2);
							if (sendnow)
								modestack_mode_ext(chansvs.nick, mychan->chan, MTYPE_ADD, i, mychan->chan->extmodes[i]);
						}
					}
				}
			}
			while (*p != ' ' && *p != '\0')
				p++;
			while (*p == ' ')
				p++;
		}
	}

}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
