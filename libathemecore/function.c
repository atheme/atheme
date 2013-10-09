/*
 * atheme-services: A collection of minimalist IRC services   
 * function.c: Miscillaneous functions.
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

char ch[] = "abcdefghijklmnopqrstuvwxyz";

/* This function uses smalloc() to allocate memory.
 * You MUST free the result when you are done with it!
 */
char *random_string(int sz)
{
	int i;
	char *buf = smalloc(sz + 1); /* padding */

	for (i = 0; i < sz; i++)
	{
		buf[i] = ch[arc4random() % 26];
	}

	buf[sz] = 0;

	return buf;
}

void create_challenge(sourceinfo_t *si, const char *name, int v, char *dest)
{
	char buf[256];
	int digest[4];
	md5_state_t ctx;

	snprintf(buf, sizeof buf, "%lu:%s:%s",
			(unsigned long)(CURRTIME / 300) - v,
			get_source_name(si),
			name);
	md5_init(&ctx);
	md5_append(&ctx, (unsigned char *)buf, strlen(buf));
	md5_finish(&ctx, (unsigned char *)digest);
	/* note: this depends on byte order, but that's ok because
	 * it's only going to work in the same atheme instance anyway
	 */
	snprintf(dest, 80, "%x:%x", digest[0], digest[1]);
}

#ifdef HAVE_GETTIMEOFDAY
/* starts a timer */
void s_time(struct timeval *sttime)
{
	gettimeofday(sttime, NULL);
}
#endif

#ifdef HAVE_GETTIMEOFDAY
/* ends a timer */
void e_time(struct timeval sttime, struct timeval *ttime)
{
	struct timeval now;

	gettimeofday(&now, NULL);
	timersub(&now, &sttime, ttime);
}
#endif

#ifdef HAVE_GETTIMEOFDAY
/* translates microseconds into miliseconds */
int tv2ms(struct timeval *tv)
{
	return (tv->tv_sec * 1000) + (int) (tv->tv_usec / 1000);
}
#endif

/* replaces tabs with a single ASCII 32 */
void tb2sp(char *line)
{
	char *c;

	while ((c = strchr(line, '\t')))
		*c = ' ';
}

/* replace all occurances of 'old' with 'new' */
char *replace(char *s, int size, const char *old, const char *new)
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
const char *number_to_string(int num)
{
	static char ret[32];
	snprintf(ret, 32, "%d", num);
	return ret;
}

/* return the time elapsed since an event */
char *time_ago(time_t event)
{
	static char ret[128];
	int years, weeks, days, hours, minutes, seconds;

	event = CURRTIME - event;
	years = weeks = days = hours = minutes = 0;

	while (event >= 60 * 60 * 24 * 365)
	{
		event -= 60 * 60 * 24 * 365;
		years++;
	}
	while (event >= 60 * 60 * 24 * 7)
	{
		event -= 60 * 60 * 24 * 7;
		weeks++;
	}
	while (event >= 60 * 60 * 24)
	{
		event -= 60 * 60 * 24;
		days++;
	}
	while (event >= 60 * 60)
	{
		event -= 60 * 60;
		hours++;
	}
	while (event >= 60)
	{
		event -= 60;
		minutes++;
	}

	seconds = event;

	if (years)
		snprintf(ret, sizeof(ret),
			 "%d year%s, %d week%s, %d day%s, %02d:%02d:%02d", years, years == 1 ? "" : "s", weeks, weeks == 1 ? "" : "s", days, days == 1 ? "" : "s", hours, minutes, seconds);
	else if (weeks)
		snprintf(ret, sizeof(ret), "%d week%s, %d day%s, %02d:%02d:%02d", weeks, weeks == 1 ? "" : "s", days, days == 1 ? "" : "s", hours, minutes, seconds);
	else if (days)
		snprintf(ret, sizeof(ret), "%d day%s, %02d:%02d:%02d", days, days == 1 ? "" : "s", hours, minutes, seconds);
	else if (hours)
		snprintf(ret, sizeof(ret), "%d hour%s, %d minute%s, %d second%s", hours, hours == 1 ? "" : "s", minutes, minutes == 1 ? "" : "s", seconds, seconds == 1 ? "" : "s");
	else if (minutes)
		snprintf(ret, sizeof(ret), "%d minute%s, %d second%s", minutes, minutes == 1 ? "" : "s", seconds, seconds == 1 ? "" : "s");
	else
		snprintf(ret, sizeof(ret), "%d second%s", seconds, seconds == 1 ? "" : "s");

	return ret;
}

char *timediff(time_t seconds)
{
	static char buf[BUFSIZE];
	long unsigned days, hours, minutes;

	days = seconds / 86400;
	seconds %= 86400;
	hours = seconds / 3600;
	hours %= 3600;
	minutes = seconds / 60;
	minutes %= 60;
	seconds %= 60;

	snprintf(buf, sizeof(buf), "%lu day%s, %lu:%02lu:%02lu", days, (days == 1) ? "" : "s", hours, minutes, (long unsigned) seconds);

	return buf;
}

/* generate a random number, for use as a key */
unsigned long makekey(void)
{
	unsigned long k;

	k = arc4random() & 0x7FFFFFFF;

	/* shorten or pad it to 9 digits */
	if (k > 1000000000)
		k = k - 1000000000;
	if (k < 100000000)
		k = k + 100000000;

	return k;
}

bool is_internal_client(user_t *u)
{
	return (u && (!u->server || u->server == me.me));
}

int validemail(const char *email)
{
	int i, valid = 1, chars = 0, atcnt = 0, dotcnt1 = 0;
	char c;
	const char *lastdot = NULL;

	/* sane length */
	if (strlen(email) >= EMAILLEN)
		return 0;

#if 0
	/* RFC2822 */
#define EXTRA_ATEXTCHARS "!#$%&'*+-/=?^_`{|}~"
#else
	/* commonly used subset */
#define EXTRA_ATEXTCHARS "%+-=^_"
#endif
	/* note that we do not allow domain literals or quoted strings */
	for (i = 0; email[i] != '\0'; i++)
	{
		c = email[i];
		if (c == '.')
		{
			dotcnt1++;
			lastdot = &email[i];
			/* dot may not be first or last, no consecutive dots */
			if (i == 0 || email[i - 1] == '.' ||
					email[i - 1] == '@' ||
					email[i + 1] == '\0' ||
					email[i + 1] == '@')
				return 0;
		}
		else if (c == '@')
			atcnt++, dotcnt1 = 0;
		else if ((c >= 'a' && c <= 'z') ||
				(c >= 'A' && c <= 'Z') ||
				(c >= '0' && c <= '9') ||
				strchr(EXTRA_ATEXTCHARS, c))
			chars++;
		else
			return 0;
	}

	/* must have exactly one @, and at least one . after the @ */
	if (atcnt != 1 || dotcnt1 == 0)
		return 0;

	/* no mail to IP addresses, this should be done using [10.2.3.4]
	 * like syntax but we do not allow that either
	 */
	if (isdigit(lastdot[1]))
		return 0;

	/* make sure there are at least 4 characters besides the above
	 * mentioned @ and .
	 */
	if (chars < 4)
		return 0;

	return valid;
}

static mowgli_list_t email_canonicalizers;

/* Re-canonicalize email addresses.
 * Call this after adding or removing an email_canonicalize hook.
 */
static void canonicalize_emails(void)
{
	myentity_iteration_state_t state;
	myentity_t *mt;

	MYENTITY_FOREACH_T(mt, &state, ENT_USER)
	{
		myuser_t *mu = user(mt);

		strshare_unref(mu->email_canonical);
		mu->email_canonical = canonicalize_email(mu->email);
	}
}

void
register_email_canonicalizer(email_canonicalizer_t func, void *user_data)
{
	email_canonicalizer_item_t *item;

	item = smalloc(sizeof(email_canonicalizer_item_t));
	item->func = func;
	item->user_data = user_data;

	mowgli_node_add(item, &item->node, &email_canonicalizers);

	canonicalize_emails();
}

void
unregister_email_canonicalizer(email_canonicalizer_t func, void *user_data)
{
	mowgli_node_t *n, *tn;

	MOWGLI_LIST_FOREACH_SAFE(n, tn, email_canonicalizers.head)
	{
		email_canonicalizer_item_t *item = n->data;

		if (item->func == func && item->user_data == user_data)
		{
			mowgli_node_delete(&item->node, &email_canonicalizers);
			free(item);

			canonicalize_emails();

			return;
		}
	}
}

stringref canonicalize_email(const char *email)
{
	mowgli_node_t *n, *tn;
	char buf[EMAILLEN + 1];

	if (email == NULL)
		return NULL;

	mowgli_strlcpy(buf, email, sizeof buf);

	MOWGLI_LIST_FOREACH_SAFE(n, tn, email_canonicalizers.head)
	{
		email_canonicalizer_item_t *item = n->data;

		item->func(buf, item->user_data);
	}

	return strshare_get(buf);
}

void canonicalize_email_case(char email[EMAILLEN + 1], void *user_data)
{
	strcasecanon(email);
}

bool email_within_limits(const char *email)
{
	mowgli_node_t *n;
	myentity_iteration_state_t state;
	myentity_t *mt;
	unsigned int tcnt = 0;
	stringref email_canonical;
	bool result = true;

	if (me.maxusers <= 0)
		return true;

	MOWGLI_ITER_FOREACH(n, nicksvs.emailexempts.head)
	{
		if (0 == match(n->data, email))
			return true;
	}

	email_canonical = canonicalize_email(email);

	MYENTITY_FOREACH_T(mt, &state, ENT_USER)
	{
		myuser_t *mu = user(mt);

		if (mu->email_canonical == email_canonical)
			tcnt++;

		/* optimization: if tcnt >= me.maxusers, quit iterating. -nenolod */
		if (tcnt >= me.maxusers) {
			result = false;
			break;
		}
	}

	strshare_unref(email_canonical);
	return result;
}

bool validhostmask(const char *host)
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
	if (strlen(host) > NICKLEN + USERLEN + HOSTLEN + 1)
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
char *pretty_mask(char *mask)
{
	static char mask_buf[BUFSIZE];
        int old_mask_pos;
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

        old_mask_pos = mask_pos;

        at = ex = NULL;
        if((t = strchr(mask, '@')) != NULL)
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
        if(strlen(nick) > NICKLEN - 1)
        {
                ne = nick[NICKLEN - 1];
                nick[NICKLEN - 1] = '\0';
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
                nick[NICKLEN - 1] = ne;
        if(ue)
                user[USERLEN] = ue;
        if(he)
                host[HOSTLEN] = he;

	return mask_buf;
}

bool validtopic(const char *topic)
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
				case 31:
					return false;
			}
		}
	}
	return true;
}

bool has_ctrl_chars(const char *text)
{
	int i;

	for (i = 0; text[i] != '\0'; i++)
	{
		if (text[i] > 0 && text[i] < 32)
			return true;
	}
	return false;
}

#ifndef MOWGLI_OS_WIN
static void sendemail_waited(pid_t pid, int status, void *data)
{
	char *email;

	email = data;
	if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
		slog(LG_INFO, "sendemail_waited(): email for %s failed", email);
	free(email);
}
#endif

/* send the specified type of email.
 *
 * u is whoever caused this to be called, the corresponding service
 *   in case of xmlrpc
 * type is EMAIL_*, see include/tools.h
 * mu is the recipient user
 * param depends on type, also see include/tools.h
 */
int sendemail(user_t *u, myuser_t *mu, const char *type, const char *email, const char *param)
{
#ifndef MOWGLI_OS_WIN
	char *date = NULL;
	char timebuf[BUFSIZE], to[BUFSIZE], from[BUFSIZE], buf[BUFSIZE], pathbuf[BUFSIZE], sourceinfo[BUFSIZE];
	FILE *in, *out;
	time_t t;
	struct tm tm;
	int pipfds[2];
	pid_t pid;
	int rc;
	static time_t period_start = 0, lastwallops = 0;
	static unsigned int emailcount = 0;
	service_t *svs;

	if (u == NULL || mu == NULL)
		return 0;

	if (me.mta == NULL)
	{
		if (strcmp(type, EMAIL_MEMO) && !is_internal_client(u))
		{
			svs = service_find("operserv");
			notice(svs ? svs->nick : me.name, u->nick, "Sending email is administratively disabled.");
		}
		return 0;
	}

	if (!validemail(email))
	{
		if (strcmp(type, EMAIL_MEMO) && !is_internal_client(u))
		{
			svs = service_find("operserv");
			notice(svs ? svs->nick : me.name, u->nick, "The email address is considered invalid.");
		}
		return 0;
	}

	if ((unsigned int)(CURRTIME - period_start) > me.emailtime)
	{
		emailcount = 0;
		period_start = CURRTIME;
	}
	emailcount++;
	if (emailcount > me.emaillimit)
	{
		if (CURRTIME - lastwallops > 60)
		{
			wallops(_("Rejecting email for %s[%s@%s] due to too high load (type '%s' to %s <%s>)"),
					u->nick, u->user, u->vhost,
					type, entity(mu)->name, email);
			slog(LG_ERROR, "sendemail(): rejecting email for %s[%s@%s] (%s) due to too high load (type '%s' to %s <%s>)",
					u->nick, u->user, u->vhost,
					u->ip ? u->ip : u->host,
					type, entity(mu)->name, email);
			lastwallops = CURRTIME;
		}
		return 0;
	}

	snprintf(pathbuf, sizeof pathbuf, "%s/%s", SHAREDIR "/email", type);
	if ((in = fopen(pathbuf, "r")) == NULL)
	{
		slog(LG_ERROR, "sendemail(): rejecting email for %s[%s@%s] (%s), due to unknown type '%s'",
			       u->nick, u->user, u->vhost, email, type);
		return 0;
	}

	slog(LG_INFO, "sendemail(): email for %s[%s@%s] (%s) type %s to %s <%s>",
			u->nick, u->user, u->vhost, u->ip ? u->ip : u->host,
			type, entity(mu)->name, email);

	/* set up the email headers */
	time(&t);
	tm = *localtime(&t);
	strftime(timebuf, sizeof timebuf, "%a, %d %b %Y %H:%M:%S %z", &tm);

	date = timebuf;

	snprintf(from, sizeof from, "\"%s\" <%s>",
			me.netname, me.register_email);
	snprintf(to, sizeof to, "\"%s\" <%s>", entity(mu)->name, email);
	/* \ is special here; escape it */
	replace(to, sizeof to, "\\", "\\\\");
	snprintf(sourceinfo, sizeof sourceinfo, "%s[%s@%s]", u->nick, u->user, u->vhost);

	/* now set up the email */
	if (pipe(pipfds) < 0)
		return 0;
	switch (pid = fork())
	{
		case -1:
			return 0;
		case 0:
			connection_close_all_fds();
			close(pipfds[1]);
			dup2(pipfds[0], 0);
			execl(me.mta, me.mta, "-t", "-f", me.register_email, NULL);
			_exit(255);
	}
	close(pipfds[0]);
	childproc_add(pid, "email", sendemail_waited, sstrdup(email));
	out = fdopen(pipfds[1], "w");

	while (fgets(buf, BUFSIZE, in))
	{
		strip(buf);

		replace(buf, sizeof buf, "&from&", from);
		replace(buf, sizeof buf, "&to&", to);
		replace(buf, sizeof buf, "&replyto&", me.adminemail);
		replace(buf, sizeof buf, "&date&", date);
		replace(buf, sizeof buf, "&accountname&", mu != NULL ? entity(mu)->name : "Guest");
		replace(buf, sizeof buf, "&entityname&", u != NULL ? entity(u)->name : "???");
		replace(buf, sizeof buf, "&netname&", me.netname);
		replace(buf, sizeof buf, "&param&", param);
		replace(buf, sizeof buf, "&sourceinfo&", sourceinfo);
		if ((svs = service_find("nickserv")) != NULL)
			replace(buf, sizeof buf, "&nicksvs&", svs->me->nick);
		if ((svs = service_find("chanserv")) != NULL)
			replace(buf, sizeof buf, "&chansvs&", svs->me->nick);
		if ((svs = service_find("memoserv")) != NULL)
			replace(buf, sizeof buf, "&memosvs&", svs->me->nick);
		if ((svs = service_find("operserv")) != NULL)
			replace(buf, sizeof buf, "&opersvs&", svs->me->nick);

		fprintf(out, "%s\n", buf);
	}

	fclose(in);

	rc = 1;
	if (ferror(out))
		rc = 0;
	if (fclose(out) < 0)
		rc = 0;
	if (rc == 0)
		slog(LG_ERROR, "sendemail(): mta failure");
	return rc;
#else
# warning implement me :(
	return 0;
#endif
}

/* various access level checkers */
bool is_founder(mychan_t *mychan, myentity_t *mt)
{
	return_val_if_fail(mt != NULL, false);

	if (chanacs_entity_has_flag(mychan, mt, CA_FOUNDER))
		return true;

	return false;
}

bool is_ircop(user_t *user)
{
	if (UF_IRCOP & user->flags)
		return true;

	return false;
}

bool is_admin(user_t *user)
{
	if (UF_ADMIN & user->flags)
		return true;

	return false;
}

bool is_autokline_exempt(user_t *user)
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

char *sbytes(float x)
{
	if (x > 1073741824.0)
		return "GB";

	else if (x > 1048576.0)
		return "MB";

	else if (x > 1024.0)
		return "KB";

	return "B";
}

float bytes(float x)
{
	if (x > 1073741824.0)
		return (x / 1073741824.0);

	if (x > 1048576.0)
		return (x / 1048576.0);

	if (x > 1024.0)
		return (x / 1024.0);

	return x;
}

int srename(const char *old_fn, const char *new_fn)
{
#ifdef MOWGLI_OS_WIN
	unlink(new_fn);
#endif

	return rename(old_fn, new_fn);
}

char *combine_path(const char *parent, const char *child)
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
