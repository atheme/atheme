/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains channel mode tracking routines.
 *
 * $Id: cmode.c 2395 2005-09-26 23:01:54Z jilles $
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
					chan->modes |= mode_list[i].value;
				else
					chan->modes &= ~mode_list[i].value;

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
				parpos++;
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
				parpos++;
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
				chan->limit = atoi(parv[parpos]);
				if (source)
					cmode(source->nick, chan->name, "+l", parv[parpos]);
			}
			else
			{
				chan->modes &= ~CMODE_LIMIT;
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
					free(chan->key);
				chan->key = sstrdup(parv[parpos]);
				if (source)
					cmode(source->nick, chan->name, "+k", chan->key);
			}
			else
			{
				chan->modes &= ~CMODE_KEY;
				if (source)
					cmode(source->nick, chan->name, "-k", chan->key);
				free(chan->key);
				chan->key = NULL;
				/* ratbox typically sends either the key or a `*' on -k, so you
				 * should eat a parameter
				 */
				parpos++;
			}
			continue;
		}

		if (*pos == 'b')
		{
			if (++parpos >= parc)
				continue;
			if (whatt == MTYPE_ADD)
			{
				chanban_add(chan, parv[parpos]);
				if (source)
					cmode(source->nick, chan->name, "+b", parv[parpos]);
			}
			else
			{
				chanban_t *c;
				
				c = chanban_find(chan, parv[parpos]);
				chanban_delete(c);
				if (source)
					cmode(source->nick, chan->name, "-b", parv[parpos]);
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
					slog(LG_ERROR, "channel_mode(): MODE %s %c%c %s", chan->name, (whatt == MTYPE_ADD) ? '+' : '-', status_mode_list[i].value, parv[parpos]);

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
					if (source == NULL && (mc = mychan_find(cu->chan->name)))
					{
						myuser_t *mu = cu->user->myuser;

						if ((MC_SECURE & mc->flags) && (status_mode_list[i].mode == 'o'))
						{
							char hostbuf[BUFSIZE];

							strlcpy(hostbuf, cu->user->nick, BUFSIZE);
							strlcat(hostbuf, "!", BUFSIZE);
							strlcat(hostbuf, cu->user->user, BUFSIZE);
							strlcat(hostbuf, "@", BUFSIZE);
							strlcat(hostbuf, cu->user->host, BUFSIZE);

							if ((!is_founder(mc, mu)) && (cu->user != chansvs.me->me) && 
								(!is_xop(mc, mu, (CA_OP | CA_AUTOOP))) && (!chanacs_find_host(mc, hostbuf, (CA_OP | CA_AUTOOP))))
							{
								/* they were opped and aren't on the list, deop them */
								if (source)
									cmode(chansvs.nick, mc->name, "-o", cu->user->nick);
								cu->modes &= ~status_mode_list[i].value;
							}
						}
					}
				}
				else
				{
					if (cu->user->server == me.me &&
							status_mode_list[i].value == CMODE_OP)
					{
						if (source == NULL && (cu->user != chansvs.me->me || chanserv_reopped == FALSE))
						{
							slog(LG_DEBUG, "channel_mode(): deopped on %s, rejoining", cu->chan->name);

							part(cu->chan->name, cu->user->nick);
							join(cu->chan->name, cu->user->nick);

							if (cu->user == chansvs.me->me)
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

	if (source == NULL)
		check_modes(mychan_find(chan->name));
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
				  user->server->invis++;
			  else
				  user->server->invis--;
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

	if (!sender)
	{
		for (i = 0; i < 3; i++)
		{
			if (modedata[i].used)
				flush_cmode(&modedata[i]);
		}
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

		switch (c)
		{
		  case 'l':
		  case 'k':
		  case 'o':
		  case 'h':
		  case 'v':
		  case 'b':
		  case 'q':
		  case 'a':
		  case 'u':
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

			  if (!add && c == 'l')
				  break;

			  s = va_arg(args, char *);

			  md->paramslen += snprintf(md->params + md->paramslen, MAXPARAMSLEN + 1 - md->paramslen, "%s%s", md->paramslen ? " " : "", s);

			  md->nparams++;
			  break;

		  default:
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

	if (!md->event)
		md->event = event_add_once("flush_cmode_callback", flush_cmode_callback, md, 1);
}

void check_modes(mychan_t *mychan)
{
	char newmodes[40], *newkey = NULL;
	char *end = newmodes;
	int32_t newlimit = 0;
	int modes;

	if (!mychan || !mychan->chan)
		return;

	modes = ~mychan->chan->modes & mychan->mlock_on;

	end += snprintf(end, sizeof(newmodes) - (end - newmodes) - 2, "+%s", flags_to_string(modes));

	mychan->chan->modes |= modes;

	if (mychan->mlock_limit && mychan->mlock_limit != mychan->chan->limit)
	{
		*end++ = 'l';
		newlimit = mychan->mlock_limit;
		mychan->chan->limit = newlimit;
		cmode(chansvs.nick, mychan->name, "+l", itoa(newlimit));
	}

	if (mychan->mlock_key)
	{
		if (mychan->chan->key && strcmp(mychan->chan->key, mychan->mlock_key) && mychan->mlock_on & CMODE_KEY)
		{
			cmode(chansvs.nick, mychan->name, "-k", mychan->chan->key);
			free(mychan->chan->key);
			mychan->chan->key = NULL;
		}

		if (!mychan->chan->key && mychan->mlock_on & CMODE_KEY)
		{
			newkey = mychan->mlock_key;
			mychan->chan->key = sstrdup(newkey);
			cmode(chansvs.nick, mychan->name, "+k", newkey);
		}
	}
	else if (mychan->chan->key && mychan->mlock_off & CMODE_KEY)
	{
		free(mychan->chan->key);
		mychan->chan->key = NULL;
		cmode(chansvs.nick, mychan->name, "-k", mychan->chan->key);
	}

	if (end[-1] == '+')
		end--;

	modes = mychan->chan->modes & mychan->mlock_off;
	modes &= ~(CMODE_KEY | CMODE_LIMIT);

	end += snprintf(end, sizeof(newmodes) - (end - newmodes) - 1, "-%s", flags_to_string(modes));

	mychan->chan->modes &= ~modes;

	if (mychan->chan->limit && (mychan->mlock_off & CMODE_LIMIT))
	{
		cmode(chansvs.nick, mychan->name, "-l");
		mychan->chan->limit = 0;
	}

	if (mychan->chan->key && (mychan->mlock_off & CMODE_KEY))
	{
		cmode(chansvs.nick, mychan->name, "-k", mychan->chan->key);
		free(mychan->chan->key);
		mychan->chan->key = NULL;
	}

	if (end[-1] == '-')
		end--;

	if (end == newmodes)
		return;

	*end = 0;

	cmode(chansvs.nick, mychan->name, newmodes);
}
