/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains channel mode tracking routines.
 *
 * $Id: cmode.c 5730 2006-07-04 17:31:44Z jilles $
 */

#include "atheme.h"

/* yeah, this should be fun. */
/* If source == NULL, apply a mode change from outside to our structures
 * If source != NULL, apply the mode change and send it out from that user
 */
void channel_mode(user_t *source, channel_t *chan, uint8_t parc, char *parv[])
{
	boolean_t matched = FALSE;
	boolean_t chanserv_reopped = FALSE;
	boolean_t simple_modes_changed = FALSE;
	int i, parpos = 0, whatt = MTYPE_NUL;
	const char *pos = parv[0];
	mychan_t *mc;
	chanuser_t *cu = NULL;

	if ((!pos) || (*pos == '\0'))
		return;

	if (!chan)
		return;

	/* SJOIN modes of 0 means no change */
	if (*pos == '0')
		return;

	for (; *pos != '\0'; pos++)
	{
		matched = FALSE;

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
				matched = TRUE;

				if (whatt == MTYPE_ADD)
				{
					if (!(chan->modes & mode_list[i].value))
						simple_modes_changed = TRUE;
					chan->modes |= mode_list[i].value;
				}
				else
				{
					if (chan->modes & mode_list[i].value)
						simple_modes_changed = TRUE;
					chan->modes &= ~mode_list[i].value;
				}

				if (source)
					modestack_mode_simple(source->nick, chan->name, whatt, mode_list[i].value);

				break;
			}
		}
		if (matched)
			continue;

		for (i = 0; ignore_mode_list[i].mode != '\0'; i++)
		{
			if (*pos == ignore_mode_list[i].mode)
			{
				matched = TRUE;
				if (whatt == MTYPE_ADD)
				{
					if (++parpos >= parc)
						break;
					if (source && !ignore_mode_list[i].check(parv[parpos], chan, NULL, NULL, NULL))
						break;
					if (chan->extmodes[i])
					{
						if (strcmp(chan->extmodes[i], parv[parpos]))
							simple_modes_changed = TRUE;
						free(chan->extmodes[i]);
					}
					else
						simple_modes_changed = TRUE;
					chan->extmodes[i] = sstrdup(parv[parpos]);
					if (source)
						modestack_mode_ext(source->nick, chan->name, MTYPE_ADD, i, chan->extmodes[i]);
				}
				else
				{
					if (chan->extmodes[i])
					{
						simple_modes_changed = TRUE;
						free(chan->extmodes[i]);
						chan->extmodes[i] = NULL;
					}
					if (source)
						modestack_mode_ext(source->nick, chan->name, MTYPE_DEL, i, NULL);
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
				i = atoi(parv[parpos]);
				if (chan->limit != i)
					simple_modes_changed = TRUE;
				chan->limit = i;
				if (source)
					modestack_mode_limit(source->nick, chan->name, MTYPE_ADD, chan->limit);
			}
			else
			{
				chan->modes &= ~CMODE_LIMIT;
				if (chan->limit != 0)
					simple_modes_changed = TRUE;
				chan->limit = 0;
				if (source)
					modestack_mode_limit(source->nick, chan->name, MTYPE_DEL, 0);
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
						simple_modes_changed = TRUE;
					free(chan->key);
				}
				else
					simple_modes_changed = TRUE;
				chan->key = sstrdup(parv[parpos]);
				if (source)
					modestack_mode_param(source->nick, chan->name, MTYPE_ADD, 'k', chan->key);
			}
			else
			{
				if (chan->key)
					simple_modes_changed = TRUE;
				chan->modes &= ~CMODE_KEY;
				if (source)
					modestack_mode_param(source->nick, chan->name, MTYPE_DEL, 'k', chan->key ? chan->key : "*");
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
					modestack_mode_param(source->nick, chan->name, MTYPE_ADD, *pos, parv[parpos]);
			}
			else
			{
				chanban_t *c;

				c = chanban_find(chan, parv[parpos], *pos);
				chanban_delete(c);
				if (source)
					modestack_mode_param(source->nick, chan->name, MTYPE_DEL, *pos, parv[parpos]);
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

				matched = TRUE;

				if (whatt == MTYPE_ADD)
				{
					cu->modes |= status_mode_list[i].value;

					if (source)
						modestack_mode_param(source->nick, chan->name, MTYPE_ADD, *pos, CLIENT_NAME(cu->user));

					/* see if they did something we have to undo */
					if (source == NULL && cu->user->server != me.me && chansvs.me != NULL && (mc = mychan_find(cu->chan->name)) && mc->flags & MC_SECURE)
					{
						if (status_mode_list[i].mode == 'o' && !(chanacs_user_flags(mc, cu->user) & (CA_OP | CA_AUTOOP)))
						{
							/* they were opped and aren't on the list, deop them */
							modestack_mode_param(chansvs.nick, mc->name, MTYPE_DEL, 'o', CLIENT_NAME(cu->user));
							cu->modes &= ~status_mode_list[i].value;
						}
						else if (ircd->uses_halfops && status_mode_list[i].mode == ircd->halfops_mchar[1] && !(chanacs_user_flags(mc, cu->user) & (CA_HALFOP | CA_AUTOHALFOP)))
						{
							/* same for halfops -- jilles */
							modestack_mode_param(chansvs.nick, mc->name, MTYPE_DEL, ircd->halfops_mchar[1], CLIENT_NAME(cu->user));
							cu->modes &= ~status_mode_list[i].value;
						}
					}
				}
				else
				{
					if (cu->user->server == me.me && status_mode_list[i].value == CMODE_OP)
					{
						if (source == NULL && (chansvs.me == NULL || cu->user != chansvs.me->me || chanserv_reopped == FALSE))
						{
							slog(LG_DEBUG, "channel_mode(): deopped on %s, rejoining", cu->chan->name);

							part(cu->chan->name, cu->user->nick);
							join(cu->chan->name, cu->user->nick);

							if (chansvs.me != NULL && cu->user == chansvs.me->me)
								chanserv_reopped = TRUE;
						}

						continue;
					}

					if (source)
						modestack_mode_param(source->nick, chan->name, MTYPE_DEL, *pos, CLIENT_NAME(cu->user));

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
			check_modes(mc, TRUE);
	}
}

/* like channel_mode() but parv array passed as varargs */
void channel_mode_va(user_t *source, channel_t *chan, uint8_t parc, const char *parv0, ...)
{
	const char *parv[255];
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
		parv[i] = va_arg(va, const char *);
	va_end(va);
	channel_mode(source, chan, parc, parv);
}

static struct modestackdata {
	char source[HOSTLEN]; /* name */
	char channel[CHANNELLEN];
	int32_t modes_on;
	int32_t modes_off;
	uint32_t limit;
	char extmodes[MAXEXTMODES][512];
	boolean_t limitused, extmodesused[MAXEXTMODES];
	char pmodes[2*MAXMODES+2];
	char params[512]; /* includes leading space */
	int totalparamslen; /* includes leading space */
	int totallen;
	int paramcount;

	uint32_t event;
} modestackdata;

static void modestack_calclen(struct modestackdata *md);

static void modestack_debugprint(struct modestackdata *md)
{
	int i;

	slog(LG_DEBUG, "modestack_debugprint(): %s MODE %s", md->source, md->channel);
	slog(LG_DEBUG, "simple %x/%x", md->modes_on, md->modes_off);
	if (md->limitused)
		slog(LG_DEBUG, "limit %u", (unsigned)md->limit);
	for (i = 0; i < MAXEXTMODES; i++)
		if (md->extmodesused[i])
			slog(LG_DEBUG, "ext %d %s", i, md->extmodes[i]);
	slog(LG_DEBUG, "pmodes %s%s", md->pmodes, md->params);
	modestack_calclen(md);
	slog(LG_DEBUG, "totallen %d/%d", md->totalparamslen, md->totallen);
}

/* calculates the length fields */
static void modestack_calclen(struct modestackdata *md)
{
	int i;
	const char *p;

	md->totallen = strlen(md->source) + USERLEN + HOSTLEN + 1 + 4 + 1 +
		10 + strlen(md->channel) + 1;
	md->totallen += 2 + 32 + strlen(md->pmodes);
	md->totalparamslen = 0;
	md->paramcount = (md->limitused != 0);
	if (md->limitused && md->limit != 0)
		md->totalparamslen += 11;
	for (i = 0; i < MAXEXTMODES; i++)
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
	int i;

	md->modes_on = 0;
	md->modes_off = 0;
	md->limitused = 0;
	for (i = 0; i < MAXEXTMODES; i++)
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
	int i;

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
	for (i = 0; i < MAXEXTMODES; i++)
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
	for (i = 0; i < MAXEXTMODES; i++)
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
	for (i = 0; i < MAXEXTMODES; i++)
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

static struct modestackdata *modestack_init(char *source, char *channel)
{
	if (irccasecmp(source, modestackdata.source) || irccasecmp(channel, modestackdata.channel))
	{
		/*slog(LG_DEBUG, "modestack_init(): new source/channel, flushing");*/
		modestack_flush(&modestackdata);
	}
	strlcpy(modestackdata.source, source, sizeof modestackdata.source);
	strlcpy(modestackdata.channel, channel, sizeof modestackdata.channel);
	return &modestackdata;
}

static void modestack_add_simple(struct modestackdata *md, int dir, int32_t flags)
{
	if (dir == MTYPE_ADD)
		md->modes_on |= flags, md->modes_off &= ~flags;
	else if (dir = MTYPE_DEL)
		md->modes_off |= flags, md->modes_on &= ~flags;
}

static void modestack_add_limit(struct modestackdata *md, int dir, uint32_t limit)
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
	else if (dir = MTYPE_DEL)
		md->limit = 0;
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
	else if (dir = MTYPE_DEL)
		md->extmodes[i][0] = '\0';
	md->extmodesused[i] = 1;
}

static void modestack_add_param(struct modestackdata *md, int dir, char type, const char *value)
{
	char *p;
	int n = 0, i;
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
	for (i = 0; i < MAXEXTMODES; i++)
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
void modestack_flush_channel(char *channel)
{
	if (channel == NULL || !irccasecmp(channel, modestackdata.channel))
		modestack_flush(&modestackdata);
}

/* forget pending modes for a certain channel */
void modestack_forget_channel(char *channel)
{
	if (channel == NULL || !irccasecmp(channel, modestackdata.channel))
		modestack_clear(&modestackdata);
}

/* stack simple modes without parameters */
void modestack_mode_simple(char *source, char *channel, int dir, int32_t flags)
{
	struct modestackdata *md;

	md = modestack_init(source, channel);
	modestack_add_simple(md, dir, flags);
	if (!md->event)
		md->event = event_add_once("flush_cmode_callback", modestack_flush_callback, md, 0);
}

/* stack a limit */
void modestack_mode_limit(char *source, char *channel, int dir, uint32_t limit)
{
	struct modestackdata *md;

	md = modestack_init(source, channel);
	modestack_add_limit(md, dir, limit);
	if (!md->event)
		md->event = event_add_once("flush_cmode_callback", modestack_flush_callback, md, 0);
}

/* stack a non-standard type C mode */
void modestack_mode_ext(char *source, char *channel, int dir, int i, const char *value)
{
	struct modestackdata *md;

	md = modestack_init(source, channel);
	if (i < 0 || i >= MAXEXTMODES)
	{
		slog(LG_ERROR, "modestack_mode_ext(): i=%d out of range (value=\"%s\")",
				i, value);
		return;
	}
	modestack_add_ext(md, dir, i, value);
	if (!md->event)
		md->event = event_add_once("flush_cmode_callback", modestack_flush_callback, md, 0);
}

/* stack a type A, B or E mode */
void modestack_mode_param(char *source, char *channel, int dir, char type, const char *value)
{
	struct modestackdata *md;

	md = modestack_init(source, channel);
	modestack_add_param(md, dir, type, value);
	if (!md->event)
		md->event = event_add_once("flush_cmode_callback", modestack_flush_callback, md, 0);
}

#define CMTYPE_UNKNOWN 0
#define CMTYPE_SIMPLE 1
#define CMTYPE_LIMIT 2
#define CMTYPE_EXT 3
#define CMTYPE_PARAM 4

/* legacy interface, blah */
void cmode(char *source, ...)
{
	va_list va;
	char *channel, *modes, m, *param;
	struct modestackdata *md;
	int dir = MTYPE_NUL;
	int type = CMTYPE_UNKNOWN;
	int i = 0;
	uint32_t flag = 0;

	if (!source)
	{
		/* yuck */
		modestack_flush(&modestackdata);
		return;
	}
	va_start(va, source);
	channel = va_arg(va, char *);
	md = modestack_init(source, channel);
	modes = va_arg(va, char *);
	slog(LG_DEBUG, "cmode(): %s MODE %s %s", source, channel, modes);
	while (*modes != '\0')
	{
		m = *modes++;
		if (m == '+')
		{
			dir = MTYPE_ADD;
			continue;
		}
		if (m == '-')
		{
			dir = MTYPE_DEL;
			continue;
		}
		if (m == 'k' || strchr(ircd->ban_like_modes, m))
			type = CMTYPE_PARAM;
		else if (m == 'l')
			type = CMTYPE_LIMIT;
		else
		{
			for (i = 0; mode_list[i].mode != '\0'; i++)
			{
				if (m == mode_list[i].mode)
					type = CMTYPE_SIMPLE, flag = mode_list[i].value;
			}
			for (i = 0; status_mode_list[i].mode != '\0'; i++)
			{
				if (m == status_mode_list[i].mode)
					type = CMTYPE_PARAM;
			}
			/* this one must be last */
			for (i = 0; ignore_mode_list[i].mode != '\0'; i++)
			{
				if (m == ignore_mode_list[i].mode)
				{
					type = CMTYPE_EXT;
					break;
				}
			}
		}
		if (type == CMTYPE_UNKNOWN)
		{
			slog(LG_INFO, "cmode(): unknown mode %c", m);
			continue;
		}
		if (type == CMTYPE_PARAM || (dir == MTYPE_ADD && type != CMTYPE_SIMPLE))
			param = va_arg(va, char *);
		else
			param = NULL;
		switch (type)
		{
			case CMTYPE_SIMPLE:
				modestack_add_simple(md, dir, flag);
				break;
			case CMTYPE_LIMIT:
				modestack_add_limit(md, dir, dir == MTYPE_ADD ? atoi(param) : 0);
				break;
			case CMTYPE_EXT:
				modestack_add_ext(md, dir, i, param);
				break;
			case CMTYPE_PARAM:
				modestack_add_param(md, dir, m, param);
		}
	}
	va_end(va);
	/*
	 * We now schedule the stack to occur as soon as we return to io_loop. This makes
	 * stacking 1:1 transactionally, but really, that's how it should work. Lag is
	 * bad, people! --nenolod
	 */
	if (!md->event)
		md->event = event_add_once("flush_cmode_callback", modestack_flush_callback, md, 0);
}

/* Clear all simple modes (+imnpstkl etc) on a channel */
void clear_simple_modes(channel_t *c)
{
	int i;

	if (c == NULL)
		return;
	c->modes = 0;
	c->limit = 0;
	if (c->key != NULL)
		free(c->key);
	c->key = NULL;
	for (i = 0; i < MAXEXTMODES; i++)
		if (c->extmodes[i] != NULL)
		{
			free(c->extmodes[i]);
			c->extmodes[i] = NULL;
		}
}

char *channel_modes(channel_t *c, boolean_t doparams)
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

/* i'm putting usermode in here too */
void user_mode(user_t *user, char *modes)
{
	int dir = MTYPE_ADD;

	if (!user)
	{
		slog(LG_DEBUG, "user_mode(): called for nonexistant user");
		return;
	}

	while (*modes != '\0')
	{
		switch (*modes)
		{
		  case '+':
			  dir = MTYPE_ADD;
			  break;
		  case '-':
			  dir = MTYPE_DEL;
			  break;
		  case 'i':
			  if (dir == MTYPE_ADD)
			  {
				  if (!(user->flags & UF_INVIS))
				  	user->server->invis++;
				  user->flags |= UF_INVIS;
			  }
			  else if (dir = MTYPE_DEL)
			  {
				  if (user->flags & UF_INVIS)
					  user->server->invis--;
				  user->flags &= ~UF_INVIS;
			  }
			  break;
		  case 'o':
			  if (dir == MTYPE_ADD)
			  {
				  if (!is_ircop(user))
				  {
					  user->flags |= UF_IRCOP;
					  slog(LG_DEBUG, "user_mode(): %s is now an IRCop", user->nick);
					  snoop("OPER: %s (%s)", user->nick, user->server->name);
					  user->server->opers++;
					  hook_call_event("user_oper", user);
				  }
			  }
			  else if (dir = MTYPE_DEL)
			  {
				  if (is_ircop(user))
				  {
					  user->flags &= ~UF_IRCOP;
					  slog(LG_DEBUG, "user_mode(): %s is no longer an IRCop", user->nick);
					  snoop("DEOPER: %s (%s)", user->nick, user->server->name);
					  user->server->opers--;
					  hook_call_event("user_deoper", user);
				  }
			  }
		  default:
			  break;
		}
		modes++;
	}
}

void check_modes(mychan_t *mychan, boolean_t sendnow)
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
		modestack_mode_simple(chansvs.nick, mychan->name, MTYPE_ADD, modes);
	mychan->chan->modes |= modes;

	if (mychan->mlock_limit && mychan->mlock_limit != mychan->chan->limit)
	{
		mychan->chan->limit = mychan->mlock_limit;
		if (sendnow)
			modestack_mode_limit(chansvs.nick, mychan->name, MTYPE_ADD, mychan->mlock_limit);
	}

	if (mychan->mlock_key)
	{
		if (mychan->chan->key && strcmp(mychan->chan->key, mychan->mlock_key))
		{
			/* some ircds still need this... :\ -- jilles */
			if (sendnow)
				modestack_mode_param(chansvs.nick, mychan->name, MTYPE_DEL, 'k', mychan->chan->key);
			free(mychan->chan->key);
			mychan->chan->key = NULL;
		}

		if (mychan->chan->key == NULL)
		{
			mychan->chan->key = sstrdup(mychan->mlock_key);
			if (sendnow)
				modestack_mode_param(chansvs.nick, mychan->name, MTYPE_ADD, 'k', mychan->mlock_key);
		}
	}

	/* check what's locked off */
	modes = mychan->chan->modes & mychan->mlock_off;
	modes &= ~(CMODE_KEY | CMODE_LIMIT);
	if (sendnow)
		modestack_mode_simple(chansvs.nick, mychan->name, MTYPE_DEL, modes);
	mychan->chan->modes &= ~modes;

	if (mychan->chan->limit && (mychan->mlock_off & CMODE_LIMIT))
	{
		if (sendnow)
			modestack_mode_limit(chansvs.nick, mychan->name, MTYPE_DEL, 0);
		mychan->chan->limit = 0;
	}

	if (mychan->chan->key && (mychan->mlock_off & CMODE_KEY))
	{
		if (sendnow)
			modestack_mode_param(chansvs.nick, mychan->name, MTYPE_DEL, 'k', mychan->chan->key);
		free(mychan->chan->key);
		mychan->chan->key = NULL;
	}

	/* non-standard type C modes separately */
	md = metadata_find(mychan, METADATA_CHANNEL, "private:mlockext");
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
							modestack_mode_ext(chansvs.nick, mychan->name, MTYPE_DEL, i, NULL);
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
								modestack_mode_ext(chansvs.nick, mychan->name, MTYPE_ADD, i, mychan->chan->extmodes[i]);
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
