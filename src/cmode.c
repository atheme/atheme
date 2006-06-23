/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains channel mode tracking routines.
 *
 * $Id: cmode.c 5520 2006-06-23 16:03:29Z jilles $
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
	char *pos = parv[0];
	mychan_t *mc;
	chanuser_t *cu = NULL;
	char str[3];

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

				str[0] = whatt == MTYPE_ADD ? '+' : '-';
				str[1] = *pos;
				str[2] = '\0';
				if (source)
					cmode(source->nick, chan->name, str);

				break;
			}
		}

		if (matched == TRUE)
			continue;

		for (i = 0; ignore_mode_list[i].mode != '\0'; i++)
		{
			if (*pos == ignore_mode_list[i].mode)
			{
				matched = TRUE;
				str[0] = whatt == MTYPE_ADD ? '+' : '-';
				str[1] = *pos;
				str[2] = '\0';
				if (whatt == MTYPE_ADD)
				{
					if (++parpos >= parc)
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
						cmode(source->nick, chan->name, str, chan->extmodes[i]);
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
						cmode(source->nick, chan->name, str, ".");
				}
				break;
			}
		}

		if (matched == TRUE)
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
					cmode(source->nick, chan->name, "+l", parv[parpos]);
			}
			else
			{
				chan->modes &= ~CMODE_LIMIT;
				if (chan->limit != 0)
					simple_modes_changed = TRUE;
				chan->limit = 0;
				if (source)
					cmode(source->nick, chan->name, "-l");
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
					cmode(source->nick, chan->name, "+k", chan->key);
			}
			else
			{
				if (chan->key)
					simple_modes_changed = TRUE;
				chan->modes &= ~CMODE_KEY;
				if (source)
					cmode(source->nick, chan->name, "-k", chan->key ? chan->key : "*");
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
			char mchar[3];

			if (++parpos >= parc)
				continue;
			if (whatt == MTYPE_ADD)
			{
				chanban_add(chan, parv[parpos], *pos);
				mchar[0] = '+';
				mchar[1] = *pos;
				mchar[2] = '\0';
				if (source)
					cmode(source->nick, chan->name, mchar, parv[parpos]);
			}
			else
			{
				chanban_t *c;

				c = chanban_find(chan, parv[parpos], *pos);
				chanban_delete(c);
				mchar[0] = '-';
				mchar[1] = *pos;
				mchar[2] = '\0';
				if (source)
					cmode(source->nick, chan->name, mchar, parv[parpos]);
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

					matched = TRUE;
				}

				if (matched == TRUE)
					break;

				matched = TRUE;

				if (whatt == MTYPE_ADD)
				{
					cu->modes |= status_mode_list[i].value;

					str[0] = '+';
					str[1] = *pos;
					str[2] = '\0';
					if (source)
						cmode(source->nick, chan->name, str, CLIENT_NAME(cu->user));

					/* see if they did something we have to undo */
					if (source == NULL && cu->user->server != me.me && chansvs.me != NULL && (mc = mychan_find(cu->chan->name)) && mc->flags & MC_SECURE)
					{
						if (status_mode_list[i].mode == 'o' && !(chanacs_user_flags(mc, cu->user) & (CA_OP | CA_AUTOOP)))
						{
							/* they were opped and aren't on the list, deop them */
							cmode(chansvs.nick, mc->name, "-o", CLIENT_NAME(cu->user));
							cu->modes &= ~status_mode_list[i].value;
						}
						else if (ircd->uses_halfops && status_mode_list[i].mode == ircd->halfops_mchar[1] && !(chanacs_user_flags(mc, cu->user) & (CA_HALFOP | CA_AUTOHALFOP)))
						{
							/* same for halfops -- jilles */
							char mchar[3];

							strlcpy(mchar, ircd->halfops_mchar, sizeof mchar);
							mchar[0] = '-';
							cmode(chansvs.nick, mc->name, mchar, CLIENT_NAME(cu->user));
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

					str[0] = '-';
					str[1] = *pos;
					str[2] = '\0';
					if (source)
						cmode(source->nick, chan->name, str, CLIENT_NAME(cu->user));

					cu->modes &= ~status_mode_list[i].value;
				}

				break;
			}
		}
		if (matched == TRUE)
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
void channel_mode_va(user_t *source, channel_t *chan, uint8_t parc, char *parv0, ...)
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
	boolean_t toadd = FALSE;

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
			  toadd = TRUE;
			  break;
		  case '-':
			  toadd = FALSE;
			  break;
		  case 'i':
			  if (toadd)
			  {
				  if (!(user->flags & UF_INVIS))
				  	user->server->invis++;
				  user->flags |= UF_INVIS;
			  }
			  else
			  {
				  if (user->flags & UF_INVIS)
					  user->server->invis--;
				  user->flags &= ~UF_INVIS;
			  }
			  break;
		  case 'o':
			  if (toadd)
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
			  else
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

/* mode stacking code was borrowed from cygnus.
 * credit to darcy grexton and andrew church.
 */

/* mode stacking struct */
struct modedata_
{
	time_t used;
	int last_add;
	char channel[64];
	char sender[32];
	int32_t binmodes_on;
	int32_t binmodes_off;
	char opmodes[MAXMODES * 2 + 1];
	char params[BUFSIZE];
	int nparams;
	int paramslen;
	uint32_t event;
} modedata[3];

/* flush stacked and waiting cmodes */
static void flush_cmode(struct modedata_ *md)
{
	char buf[BUFSIZE];
	int len = 0;
	char lastc = 0;

	if (!md->binmodes_on && !md->binmodes_off && !*md->opmodes)
	{
		memset(md, 0, sizeof(*md));
		md->last_add = -1;
		return;
	}

	if (md->binmodes_off)
	{
		len += snprintf(buf + len, sizeof(buf) - len, "-%s", flags_to_string(md->binmodes_off));
		lastc = '-';
	}

	if (md->binmodes_on)
	{
		len += snprintf(buf + len, sizeof(buf) - len, "+%s", flags_to_string(md->binmodes_on));
		lastc = '+';
	}

	if (*md->opmodes)
	{
		if (*md->opmodes == lastc)
			memmove(md->opmodes, md->opmodes + 1, strlen(md->opmodes + 1) + 1);
		len += snprintf(buf + len, sizeof(buf) - len, "%s", md->opmodes);
	}

	if (md->paramslen)
		snprintf(buf + len, sizeof(buf) - len, " %s", md->params);

	mode_sts(md->sender, md->channel, buf);

	memset(md, 0, sizeof(*md));
	md->last_add = -1;
}

void flush_cmode_callback(void *arg)
{
	flush_cmode((struct modedata_ *)arg);
}

/* stacks channel modes to be applied to a channel */
void cmode(char *sender, ...)
{
	va_list args;
	char *channel, *modes;
	struct modedata_ *md;
	int which = -1, add;
	int32_t flag;
	int i;
	char c, *s;
	int takesparams; /* 0->no, 1->only when set, 2->always */

	if (!sender)
	{
		for (i = 0; i < 3; i++)
		{
			if (modedata[i].used)
				flush_cmode(&modedata[i]);
		}
		return;
	}

	va_start(args, sender);
	channel = va_arg(args, char *);
	modes = va_arg(args, char *);

	for (i = 0; i < 3; i++)
	{
		if ((modedata[i].used) && (!strcasecmp(modedata[i].channel, channel)))
		{
			if (strcasecmp(modedata[i].sender, sender))
				flush_cmode(&modedata[i]);

			which = i;
			break;
		}
	}

	if (which < 0)
	{
		for (i = 0; i < 3; i++)
		{
			if (!modedata[i].used)
			{
				which = i;
				modedata[which].last_add = -1;
				break;
			}
		}
	}

	if (which < 0)
	{
		int oldest = 0;
		time_t oldest_time = modedata[0].used;

		for (i = 1; i < 3; i++)
		{
			if (modedata[i].used < oldest_time)
			{
				oldest_time = modedata[i].used;
				oldest = i;
			}
		}

		flush_cmode(&modedata[oldest]);
		which = oldest;
		modedata[which].last_add = -1;
	}

	md = &modedata[which];
	strscpy(md->sender, sender, 32);
	strscpy(md->channel, channel, 64);

	add = -1;

	while ((c = *modes++))
	{
		if (c == '+')
		{
			add = 1;
			continue;
		}
		else if (c == '-')
		{
			add = 0;
			continue;
		}
		else if (add < 0)
			continue;

		if (c == 'k' || strchr(ircd->ban_like_modes, c))
			takesparams = 2;
		else if (c == 'l')
			takesparams = 1;
		else
		{
			takesparams = 0;
			for (i = 0; status_mode_list[i].mode != '\0'; i++)
			{
				if (c == status_mode_list[i].mode)
					takesparams = 2;
			}
			for (i = 0; ignore_mode_list[i].mode != '\0'; i++)
			{
				if (c == ignore_mode_list[i].mode)
					takesparams = 1; /* may not be true */
			}
		}
		if (takesparams)
		{
			if (md->nparams >= MAXMODES || md->paramslen >= MAXPARAMSLEN)
			{
				flush_cmode(&modedata[which]);
				strscpy(md->sender, sender, 32);
				strscpy(md->channel, channel, 64);
				md->used = CURRTIME;
			}

			s = md->opmodes + strlen(md->opmodes);

			if (add != md->last_add)
			{
				*s++ = add ? '+' : '-';
				md->last_add = add;
			}

			*s++ = c;

			if (!add && takesparams == 1)
				break;

			s = va_arg(args, char *);

			md->paramslen += snprintf(md->params + md->paramslen, MAXPARAMSLEN + 1 - md->paramslen, "%s%s", md->paramslen ? " " : "", s);

			md->nparams++;

		}
		else
		{
			flag = mode_to_flag(c);

			if (add)
			{
				md->binmodes_on |= flag;
				md->binmodes_off &= ~flag;
			}
			else
			{
				md->binmodes_off |= flag;
				md->binmodes_on &= ~flag;
			}
		}
	}

	va_end(args);
	md->used = CURRTIME;

	/*
	 * We now schedule the stack to occur as soon as we return to io_loop. This makes
	 * stacking 1:1 transactionally, but really, that's how it should work. Lag is
	 * bad, people! --nenolod
	 */
	if (!md->event)
		md->event = event_add_once("flush_cmode_callback", flush_cmode_callback, md, 0);
}

void check_modes(mychan_t *mychan, boolean_t sendnow)
{
	char newmodes[40], *newkey = NULL;
	char *end = newmodes;
	int32_t newlimit = 0;
	int modes;

	if (!mychan || !mychan->chan)
		return;
	mychan->flags &= ~MC_MLOCK_CHECK;

	modes = ~mychan->chan->modes & mychan->mlock_on;
	modes &= ~(CMODE_KEY | CMODE_LIMIT);

	end += snprintf(end, sizeof(newmodes) - (end - newmodes) - 2, "+%s", flags_to_string(modes));

	mychan->chan->modes |= modes;

	if (mychan->mlock_limit && mychan->mlock_limit != mychan->chan->limit)
	{
		newlimit = mychan->mlock_limit;
		mychan->chan->limit = newlimit;
		if (sendnow)
			cmode(chansvs.nick, mychan->name, "+l", itoa(newlimit));
	}

	if (mychan->mlock_key)
	{
		if (mychan->chan->key && strcmp(mychan->chan->key, mychan->mlock_key))
		{
			/* some ircds still need this... :\ -- jilles */
			cmode(chansvs.nick, mychan->name, "-k", mychan->chan->key);
			free(mychan->chan->key);
			mychan->chan->key = NULL;
		}

		if (mychan->chan->key == NULL)
		{
			newkey = mychan->mlock_key;
			mychan->chan->key = sstrdup(newkey);
			if (sendnow)
				cmode(chansvs.nick, mychan->name, "+k", newkey);
		}
	}

	if (end[-1] == '+')
		end--;

	modes = mychan->chan->modes & mychan->mlock_off;
	modes &= ~(CMODE_KEY | CMODE_LIMIT);

	end += snprintf(end, sizeof(newmodes) - (end - newmodes) - 1, "-%s", flags_to_string(modes));

	mychan->chan->modes &= ~modes;

	if (mychan->chan->limit && (mychan->mlock_off & CMODE_LIMIT))
	{
		if (sendnow)
			cmode(chansvs.nick, mychan->name, "-l");
		mychan->chan->limit = 0;
	}

	if (mychan->chan->key && (mychan->mlock_off & CMODE_KEY))
	{
		if (sendnow)
			cmode(chansvs.nick, mychan->name, "-k", mychan->chan->key);
		free(mychan->chan->key);
		mychan->chan->key = NULL;
	}

	if (end[-1] == '-')
		end--;

	if (end == newmodes)
		return;

	*end = 0;

	if (sendnow)
		cmode(chansvs.nick, mychan->name, newmodes);
}
