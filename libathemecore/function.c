/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2014 Atheme Project (http://atheme.org/)
 * Copyright (C) 2018 Atheme Development Group (https://atheme.github.io/)
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * atheme-services: A collection of minimalist IRC services
 * function.c: Miscillaneous functions.
 */

#include <atheme.h>
#include "internal.h"

bool
string_in_list(const char *const restrict str, const char *restrict list)
{
	if (! str || ! *str || ! list || ! *list)
		return false;

	const size_t len = strlen(str);

	while (*list)
	{
		const char *const ptr = strpbrk(list, " \t\r\n");

		if (ptr && (size_t)(ptr - list) == len && strncasecmp(list, str, len) == 0)
			return true;

		if (! ptr && strcasecmp(list, str) == 0)
			return true;

		if (! ptr)
			return false;

		list = ptr;

		while (*list == ' ' || *list == '\t' || *list == '\r' || *list == '\n')
			list++;
	}

	return false;
}

/* This function uses smalloc() to allocate memory.
 * You MUST sfree() the result when you are done with it!
 */
char * ATHEME_FATTR_MALLOC ATHEME_FATTR_RETURNS_NONNULL
random_string(const size_t sz)
{
	char *const buf = smalloc(sz + 1); /* NULL terminator */

	(void) atheme_random_str(buf, sz);

	return buf;
}

const char *
create_weak_challenge(struct sourceinfo *const restrict si, const char *const restrict name)
{
	char key[BUFSIZE];
	char val[BUFSIZE];

	(void) memset(key, 0x00, sizeof key);
	(void) memset(val, 0x00, sizeof val);

	(void) mowgli_strlcpy(key, get_source_name(si), sizeof key);
	(void) mowgli_strlcpy(val, name, sizeof val);

	uint32_t out[4];

	if (! digest_oneshot_hmac(DIGALG_MD5, key, sizeof key, val, sizeof val, out, NULL))
		return NULL;

	static char result[BUFSIZE];

	if (snprintf(result, BUFSIZE, "%" PRIX32 ":%" PRIX32, out[0], out[1]) >= BUFSIZE)
	{
		(void) slog(LG_ERROR, "%s: snprintf(3) would have overflowed result buffer (BUG)", MOWGLI_FUNC_NAME);
		return NULL;
	}

	return result;
}

#ifdef HAVE_GETTIMEOFDAY
/* starts a timer */
void
s_time(struct timeval *sttime)
{
	gettimeofday(sttime, NULL);
}
#endif

#ifdef HAVE_GETTIMEOFDAY
/* ends a timer */
void
e_time(struct timeval sttime, struct timeval *ttime)
{
	struct timeval now;

	gettimeofday(&now, NULL);
	timersub(&now, &sttime, ttime);
}
#endif

#ifdef HAVE_GETTIMEOFDAY
/* translates microseconds into miliseconds */
int
tv2ms(struct timeval *tv)
{
	return (tv->tv_sec * 1000) + (int) (tv->tv_usec / 1000);
}
#endif

/* replaces tabs with a single ASCII 32 */
void
tb2sp(char *line)
{
	char *c;

	while ((c = strchr(line, '\t')))
		*c = ' ';
}

/* replace all occurances of 'old' with 'new' */
char *
replace(char *s, int size, const char *old, const char *new)
{
	char *ptr = s;
	int left = strlen(s);
	int avail = size - (left + 1);
	int oldlen = strlen(old);
	int newlen = strlen(new);
	int diff = newlen - oldlen;

	while (left >= oldlen)
	{
		if (strncmp(ptr, old, oldlen))
		{
			left--;
			ptr++;
			continue;
		}

		if (diff > avail)
			break;

		if (diff != 0)
			memmove(ptr + oldlen + diff, ptr + oldlen, left + 1 - oldlen);

		memcpy(ptr, new, newlen);
		ptr += newlen;
		left -= oldlen;
	}

	return s;
}

/* reverse of atoi() */
const char *
number_to_string(int num)
{
	static char ret[32];
	snprintf(ret, 32, "%d", num);
	return ret;
}

/* a better atoi() but detects errors and doesn't allow negative values */
bool ATHEME_FATTR_WUR
string_to_uint(const char *const restrict in, unsigned int *const restrict out)
{
	if (! in || ! *in || strchr(in, '-'))
		return false;

	errno = 0;
	char *endptr = NULL;
	const unsigned long long int ret = strtoull(in, &endptr, 10);

	if ((endptr != NULL && *endptr != 0x00) || (ret == ULLONG_MAX && errno != 0))
		return false;

#if (UINT_MAX < ULLONG_MAX)
	if (ret > UINT_MAX)
		return false;
#endif

	*out = (unsigned int) ret;
	return true;
}

/* return the time elapsed since an event */
char *
time_ago(time_t event)
{
	static char ret[128];
	int years, weeks, days, hours, minutes, seconds;

	event = CURRTIME - event;
	years = weeks = days = hours = minutes = 0;

	while (event >= (SECONDS_PER_DAY * 365))
	{
		event -= (SECONDS_PER_DAY * 365);
		years++;
	}
	while (event >= SECONDS_PER_WEEK)
	{
		event -= SECONDS_PER_WEEK;
		weeks++;
	}
	while (event >= SECONDS_PER_DAY)
	{
		event -= SECONDS_PER_DAY;
		days++;
	}
	while (event >= SECONDS_PER_HOUR)
	{
		event -= SECONDS_PER_HOUR;
		hours++;
	}
	while (event >= SECONDS_PER_MINUTE)
	{
		event -= SECONDS_PER_MINUTE;
		minutes++;
	}

	seconds = event;

	if (years)
		snprintf(ret, sizeof(ret), "%dy %dw %dd", years, weeks, days);
	else if (weeks)
		snprintf(ret, sizeof(ret), "%dw %dd %dh", weeks, days, hours);
	else if (days)
		snprintf(ret, sizeof(ret), "%dd %dh %dm %ds", days, hours, minutes, seconds);
	else if (hours)
		snprintf(ret, sizeof(ret), "%dh %dm %ds", hours, minutes, seconds);
	else if (minutes)
		snprintf(ret, sizeof(ret), "%dm %ds", minutes, seconds);
	else
		snprintf(ret, sizeof(ret), "%ds", seconds);

	return ret;
}

char *
timediff(time_t seconds)
{
	static char buf[BUFSIZE];
	long unsigned days, hours, minutes;

	days = seconds / SECONDS_PER_DAY;
	seconds %= SECONDS_PER_DAY;
	hours = seconds / SECONDS_PER_HOUR;
	hours %= SECONDS_PER_HOUR;
	minutes = seconds / SECONDS_PER_MINUTE;
	minutes %= SECONDS_PER_MINUTE;
	seconds %= SECONDS_PER_MINUTE;

	snprintf(buf, sizeof(buf), "%lu day%s, %lu:%02lu:%02lu", days, (days == 1) ? "" : "s", hours, minutes, (long unsigned) seconds);

	return buf;
}

/* generate a random number, for use as a key */
unsigned long
makekey(void)
{
	unsigned long k;

	k = atheme_random() & 0x7FFFFFFF;

	/* shorten or pad it to 9 digits */
	if (k > 1000000000)
		k = k - 1000000000;
	if (k < 100000000)
		k = k + 100000000;

	return k;
}

bool
is_internal_client(struct user *u)
{
	return (u && (!u->server || u->server == me.me));
}

bool
validhostmask(const char *host)
{
	char *p, *q;

	if (strchr(host, ' '))
		return false;

	/* make sure it has ! and @ in that order and only once */
	p = strchr(host, '!');
	q = strchr(host, '@');
	if (p == NULL || q == NULL || p > q || strchr(p + 1, '!') ||
			strchr(q + 1, '@'))
		return false;

	/* XXX this NICKLEN is too long */
	if (strlen(host) > NICKLEN + 1 + USERLEN + 1 + HOSTLEN + 1)
		return false;

	if (host[0] == ',' || host[0] == '-' || host[0] == '#' ||
			host[0] == '@' || host[0] == '!' || host[0] == ':')
		return false;

	return true;
}

/* char *
 * pretty_mask(char *mask);
 *
 * Input: A mask.
 * Output: A "user-friendly" version of the mask, in mask_buf.
 * Side-effects: mask_buf is appended to. mask_pos is incremented.
 * Notes: The following transitions are made:
 *  x!y@z =>  x!y@z
 *  y@z   =>  *!y@z
 *  x!y   =>  x!y@*
 *  x     =>  x!*@*
 *  z.d   =>  *!*@z.d
 *
 * If either nick/user/host are > than their respective limits, they are
 * chopped
 */
char *
pretty_mask(char *mask)
{
	static char mask_buf[BUFSIZE];
        char star[] = "*";
        char *nick = star, *user = star, *host = star;
	int mask_pos = 0;

        char *t, *at, *ex;
        char ne = 0, ue = 0, he = 0;    /* save values at nick[NICKLEN], et all */

	/* No point wasting CPU time if the mask is already valid */
	if (validhostmask(mask))
		return mask;

        if((size_t) (BUFSIZE - mask_pos) < strlen(mask) + 5)
                return (NULL);

        at = ex = NULL;
	if(is_valid_host(mask))
	{
		if (*mask != '\0')
			host = mask;
	}
	else if((t = strchr(mask, '@')) != NULL)
	{
                at = t;
                *t++ = '\0';
                if(*t != '\0')
                        host = t;

                if((t = strchr(mask, '!')) != NULL)
                {
                        ex = t;
                        *t++ = '\0';
                        if(*t != '\0')
                                user = t;
                        if(*mask != '\0')
                                nick = mask;
                }
                else
                {
                        if(*mask != '\0')
                                user = mask;
                }
        }
        else if((t = strchr(mask, '!')) != NULL)
        {
                ex = t;
                *t++ = '\0';
                if(*mask != '\0')
                        nick = mask;
                if(*t != '\0')
                        user = t;
        }
        else if(strchr(mask, '.') != NULL && strchr(mask, ':') != NULL)
        {
                if(*mask != '\0')
                        host = mask;
        }
        else
        {
                if(*mask != '\0')
                        nick = mask;
        }

        /* truncate values to max lengths */
        if(strlen(nick) > NICKLEN)
        {
                ne = nick[NICKLEN];
                nick[NICKLEN] = '\0';
        }
        if(strlen(user) > USERLEN)
        {
                ue = user[USERLEN];
                user[USERLEN] = '\0';
        }
        if(strlen(host) > HOSTLEN)
        {
                he = host[HOSTLEN];
                host[HOSTLEN] = '\0';
        }

	snprintf(mask_buf, sizeof mask_buf, "%s!%s@%s", nick, user, host);

        /* restore mask, since we may need to use it again later */
        if(at)
                *at = '@';
        if(ex)
                *ex = '!';
        if(ne)
                nick[NICKLEN] = ne;
        if(ue)
                user[USERLEN] = ue;
        if(he)
                host[HOSTLEN] = he;

	return mask_buf;
}

bool
validtopic(const char *topic)
{
	int i;

	/* Most server protocols support less than this (depending on
	 * the lengths of the identifiers), but this should catch the
	 * ludicrous stuff.
	 */
	if (strlen(topic) > 450)
		return false;
	for (i = 0; topic[i] != '\0'; i++)
	{
		switch (topic[i])
		{
			case '\r':
			case '\n':
				return false;
		}
	}
	return true;
}
bool
validtopic_ctrl_chars(const char *topic)
{
	int i;
	if (ircd->flags & IRCD_TOPIC_NOCOLOUR)
	{
		for (i = 0; topic[i] != '\0'; i++)
		{
			switch (topic[i])
			{
				case 2:
				case 3:
				case 6:
				case 7:
				case 22:
				case 23:
				case 27:
				case 29: //italics
				case 31:
					return false;
			}
		}
	}
	return true;
}

bool
has_ctrl_chars(const char *text)
{
	int i;

	for (i = 0; text[i] != '\0'; i++)
	{
		if (text[i] > 0 && text[i] < 32)
			return true;
	}
	return false;
}

/* various access level checkers */
bool
is_founder(struct mychan *mychan, struct myentity *mt)
{
	if (mt == NULL)
		return false;

	if (chanacs_entity_has_flag(mychan, mt, CA_FOUNDER))
		return true;

	return false;
}

bool
is_autokline_exempt(struct user *user)
{
	mowgli_node_t *n;
	char buf[BUFSIZE];

	snprintf(buf, sizeof(buf), "%s@%s", user->user, user->host);
	MOWGLI_ITER_FOREACH(n, config_options.exempts.head)
	{
		if (0 == match(n->data, buf))
			return true;
	}
	return false;
}

char *
sbytes(float x)
{
	if (x > 1073741824.0f)
		return "GB";

	else if (x > 1048576.0f)
		return "MB";

	else if (x > 1024.0f)
		return "KB";

	return "B";
}

float
bytes(float x)
{
	if (x > 1073741824.0f)
		return (x / 1073741824.0f);

	if (x > 1048576.0f)
		return (x / 1048576.0f);

	if (x > 1024.0f)
		return (x / 1024.0f);

	return x;
}

int
srename(const char *old_fn, const char *new_fn)
{
#ifdef MOWGLI_OS_WIN
	unlink(new_fn);
#endif

	return rename(old_fn, new_fn);
}

char *
combine_path(const char *parent, const char *child)
{
	char buf[BUFSIZE];

	return_val_if_fail(parent != NULL, NULL);
	return_val_if_fail(child != NULL, NULL);

	mowgli_strlcpy(buf, parent, sizeof buf);
	mowgli_strlcat(buf, "/", sizeof buf);
	mowgli_strlcat(buf, child, sizeof buf);

	return sstrdup(buf);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
