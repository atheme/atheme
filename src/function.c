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

char ch[27] = "abcdefghijklmnopqrstuvwxyz";

/* This function uses smalloc() to allocate memory.
 * You MUST free the result when you are done with it!
 */
char *gen_pw(int sz)
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

	/* make sure there are at least 6 characters besides the above
	 * mentioned @ and .
	 */
	if (chars < 6)
		return 0;

	return valid;
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

	if (host[0] == ',' || host[0] == '-' || host[0] == '#' || host[0] == '@' || host[0] == '!')
		return false;

	return true;
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

static void sendemail_waited(pid_t pid, int status, void *data)
{
	char *email;

	email = data;
	if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
		slog(LG_INFO, "sendemail_waited(): email for %s failed", email);
	free(email);
}

/* send the specified type of email.
 *
 * u is whoever caused this to be called, the corresponding service
 *   in case of xmlrpc
 * type is EMAIL_*, see include/tools.h
 * mu is the recipient user
 * param depends on type, also see include/tools.h
 */
int sendemail(user_t *u, int type, myuser_t *mu, const char *param)
{
	char *email, *date = NULL;
	char timebuf[256], to[128], from[128], subject[128];
	FILE *out;
	time_t t;
	struct tm tm;
	int pipfds[2];
	pid_t pid;
	int rc;
	static time_t period_start = 0, lastwallops = 0;
	static unsigned int emailcount = 0;

	if (u == NULL || mu == NULL)
		return 0;

	if (me.mta == NULL)
	{
		if (type != EMAIL_MEMO && !is_internal_client(u))
		{
			service_t *svs = service_find("operserv");
			notice(svs ? svs->nick : me.name, u->nick, "Sending email is administratively disabled.");
		}
		return 0;
	}

	if (type == EMAIL_SETEMAIL)
	{
		/* special case for e-mail change */
		metadata_t *md = metadata_find(mu, "private:verify:emailchg:newemail");

		if (md && md->value)
			email = md->value;
		else		/* should NEVER happen */
		{
			slog(LG_ERROR, "sendemail(): got email change request, but newemail unset!");
			return 0;
		}
	}
	else
		email = mu->email;

	if (!validemail(email))
	{
		if (type != EMAIL_MEMO && !is_internal_client(u))
		{
			service_t *svs = service_find("operserv");
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
			wallops(_("Rejecting email for %s[%s@%s] due to too high load (type %d to %s <%s>)"),
					u->nick, u->user, u->vhost,
					type, entity(mu)->name, email);
			slog(LG_ERROR, "sendemail(): rejecting email for %s[%s@%s] (%s) due to too high load (type %d to %s <%s>)",
					u->nick, u->user, u->vhost,
					u->ip[0] ? u->ip : u->host,
					type, entity(mu)->name, email);
			lastwallops = CURRTIME;
		}
		return 0;
	}

	slog(LG_INFO, "sendemail(): email for %s[%s@%s] (%s) type %d to %s <%s>",
			u->nick, u->user, u->vhost, u->ip[0] ? u->ip : u->host,
			type, entity(mu)->name, email);

	/* set up the email headers */
	time(&t);
	tm = *gmtime(&t);
	strftime(timebuf, sizeof(timebuf) - 1, "%a, %d %b %Y %H:%M:%S +0000", &tm);

	date = timebuf;

	snprintf(from, sizeof from, "%s automailer <noreply.%s>",
			me.netname, me.adminemail);
	snprintf(to, sizeof to, "%s <%s>", entity(mu)->name, email);

	strlcpy(subject, me.netname, sizeof subject);
	strlcat(subject, " ", sizeof subject);
	if (type == EMAIL_REGISTER)
		if (nicksvs.no_nick_ownership)
			strlcat(subject, "Account Registration", sizeof subject);
		else
			strlcat(subject, "Nickname Registration", sizeof subject);
	else if (type == EMAIL_SENDPASS || type == EMAIL_SETPASS)
		strlcat(subject, "Password Retrieval", sizeof subject);
	else if (type == EMAIL_SETEMAIL)
		strlcat(subject, "Change Email Confirmation", sizeof subject);
	else if (type == EMAIL_MEMO)
		strlcat(subject, "New memo", sizeof subject);
	
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
			execl(me.mta, me.mta, "-t", NULL);
			_exit(255);
	}
	close(pipfds[0]);
	childproc_add(pid, "email", sendemail_waited, sstrdup(email));
	out = fdopen(pipfds[1], "w");

	fprintf(out, "From: %s\n", from);
	fprintf(out, "To: %s\n", to);
	fprintf(out, "Subject: %s\n", subject);
	fprintf(out, "Date: %s\n\n", date);

	fprintf(out, "%s,\n\n", entity(mu)->name);

	if (type == EMAIL_REGISTER)
	{
		fprintf(out, "In order to complete your registration, you must send the following\ncommand on IRC:\n");
		fprintf(out, "/msg %s VERIFY REGISTER %s %s\n\n", nicksvs.me->disp, entity(mu)->name, param);
		fprintf(out, "Thank you for registering your %s on the %s IRC " "network!\n\n",
				(nicksvs.no_nick_ownership ? "account" : "nickname"), me.netname);
	}
	else if (type == EMAIL_SENDPASS)
	{
		fprintf(out, "Someone has requested the password for %s be sent to the\n" "corresponding email address. If you did not request this action\n" "please let us know.\n\n", entity(mu)->name);
		fprintf(out, "The password for %s is %s. Please write this down for " "future reference.\n\n", entity(mu)->name, param);
	}
	else if (type == EMAIL_SETEMAIL)
	{
		fprintf(out, "In order to complete your email change, you must send\n" "the following command on IRC:\n");
		fprintf(out, "/msg %s VERIFY EMAILCHG %s %s\n\n", nicksvs.me->disp, entity(mu)->name, param);
	}
	else if (type == EMAIL_MEMO)
	{
		if (u->myuser != NULL)
			fprintf(out,"You have a new memo from %s.\n\n", entity(u->myuser)->name);
		else
			/* shouldn't happen */
			fprintf(out,"You have a new memo from %s (unregistered?).\n\n", u->nick);
		fprintf(out,"%s\n\n", param);
	}
	else if (type == EMAIL_SETPASS)
	{
		fprintf(out, "In order to set a new password, you must send\n" "the following command on IRC:\n");
		fprintf(out, "/msg %s SETPASS %s %s <password>\nwhere <password> is your desired new password.\n\n", nicksvs.me->disp, entity(mu)->name, param);
	}

	fprintf(out, "Thank you for your interest in the %s IRC network.\n", me.netname);
	if (u->server != me.me)
		fprintf(out, "\nThis email was sent due to a command from %s[%s@%s]\nat %s.\n", u->nick, u->user, u->vhost, date);
	fprintf(out, "If this message is spam, please contact %s\nwith a full copy.\n",
			me.adminemail);
	rc = 1;
	if (ferror(out))
		rc = 0;
	if (fclose(out) < 0)
		rc = 0;
	if (rc == 0)
		slog(LG_ERROR, "sendemail(): mta failure");
	return rc;
}

/* various access level checkers */
bool is_founder(mychan_t *mychan, myentity_t *mt)
{
	return_val_if_fail(mt != NULL, false);

	if (chanacs_find(mychan, mt, CA_FOUNDER))
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

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
