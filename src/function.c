/*
 * Copyright (c) 2005-2006 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains misc routines.
 *
 * $Id: function.c 7661 2007-02-15 10:39:35Z jilles $
 */

#include "atheme.h"

FILE *log_file;
int log_force;

/* there is no way windows has this command. */
#ifdef _WIN32
# undef HAVE_GETTIMEOFDAY
#endif

char ch[27] = "abcdefghijklmnopqrstuvwxyz";

/* This function uses smalloc() to allocate memory.
 * You MUST free the result when you are done with it!
 */
char *gen_pw(int8_t sz)
{
	int8_t i;
	char *buf = smalloc(sz + 1); /* padding */

	for (i = 0; i < sz; i++)
	{
		buf[i] = ch[rand() % 26];
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
int32_t tv2ms(struct timeval *tv)
{
	return (tv->tv_sec * 1000) + (int32_t) (tv->tv_usec / 1000);
}
#endif

/* replaces tabs with a single ASCII 32 */
void tb2sp(char *line)
{
	char *c;

	while ((c = strchr(line, '\t')))
		*c = ' ';
}

/* opens atheme.log */
void log_open(void)
{
	static time_t lastfail = 0;

	if (log_file)
		return;

	if ((log_file = fopen(log_path, "a")) == NULL)
	{
		/* At most one warning per hour */
		if (me.connected && lastfail + 3600 < CURRTIME)
		{
			lastfail = CURRTIME;
			wallops("Could not open log file (%s), log entries will be missing!", strerror(errno)); 
		}
		return;
	}
#ifndef _WIN32
	fcntl(fileno(log_file), F_SETFD, FD_CLOEXEC);
#endif
}

/* logs something to shrike.log */
void slog(uint32_t level, const char *fmt, ...)
{
	va_list args;
	time_t t;
	struct tm tm;
	char buf[64];
	char lbuf[BUFSIZE];

	if (!log_force && (level & me.loglevel) == 0)
		return;

	va_start(args, fmt);

	time(&t);
	tm = *localtime(&t);
	strftime(buf, sizeof(buf) - 1, "[%d/%m/%Y %H:%M:%S]", &tm);

	vsnprintf(lbuf, BUFSIZE, fmt, args);

	if (!log_file)
		log_open();

	if (log_file)
	{
		fprintf(log_file, "%s %s\n", buf, lbuf);

		fflush(log_file);
	}

	if ((runflags & (RF_LIVE | RF_STARTING)))
		fprintf(stderr, "%s %s\n", buf, lbuf);

	va_end(args);
}

void logcommand(sourceinfo_t *si, int level, const char *fmt, ...)
{
	va_list args;
	char lbuf[BUFSIZE];

	va_start(args, fmt);
	vsnprintf(lbuf, BUFSIZE, fmt, args);
	va_end(args);
	if (si->su != NULL)
		logcommand_user(si->service, si->su, level, "%s", lbuf);
	else
		logcommand_external(si->service, si->v != NULL ? si->v->description : "unknown", si->connection, si->sourcedesc, si->smu, level, "%s", lbuf);
}

void logcommand_user(service_t *svs, user_t *source, int level, const char *fmt, ...)
{
	va_list args;
	time_t t;
	struct tm tm;
	char datetime[64];
	char lbuf[BUFSIZE];

	if (!log_force && (level & me.loglevel) == 0)
		return;

	va_start(args, fmt);

	time(&t);
	tm = *localtime(&t);
	strftime(datetime, sizeof(datetime) - 1, "[%d/%m/%Y %H:%M:%S]", &tm);

	vsnprintf(lbuf, BUFSIZE, fmt, args);

	if (!log_file)
		log_open();

	if (log_file)
	{
		fprintf(log_file, "%s %s %s:%s!%s@%s[%s] %s\n",
				datetime,
				svs != NULL ? svs->name : me.name,
				source->myuser != NULL ? source->myuser->name : "",
				source->nick, source->user, source->vhost,
				source->ip[0] != '\0' ? source->ip : source->host,
				lbuf);

		fflush(log_file);
	}

#if 0
	if ((runflags & (RF_LIVE | RF_STARTING)))
		fprintf(stderr, "%s %s\n", buf, lbuf);
#endif

	va_end(args);
}

void logcommand_external(service_t *svs, const char *type, connection_t *source,const char *sourcedesc, myuser_t *login, int level, const char *fmt, ...)
{
	va_list args;
	time_t t;
	struct tm tm;
	char datetime[64];
	char lbuf[BUFSIZE];

	if (!log_force && (level & me.loglevel) == 0)
		return;

	va_start(args, fmt);

	time(&t);
	tm = *localtime(&t);
	strftime(datetime, sizeof(datetime) - 1, "[%d/%m/%Y %H:%M:%S]", &tm);

	vsnprintf(lbuf, BUFSIZE, fmt, args);

	if (!log_file)
		log_open();

	if (log_file)
	{
		fprintf(log_file, "%s %s %s:%s(%s)[%s] %s\n",
				datetime,
				svs != NULL ? svs->name : me.name,
				login != NULL ? login->name : "",
				type,
				source != NULL ? source->hbuf : "<noconn>",
				sourcedesc != NULL ? sourcedesc : "<unknown>",
				lbuf);

		fflush(log_file);
	}

#if 0
	if ((runflags & (RF_LIVE | RF_STARTING)))
		fprintf(stderr, "%s %s\n", buf, lbuf);
#endif

	va_end(args);
}

/*
 * This generates a hash value, based on chongo's hash algo,
 * located at http://www.isthe.com/chongo/tech/comp/fnv/
 *
 * The difference between FNV and Atheme's hash algorithm is
 * that FNV uses a random key for toasting, we just use
 * 16 instead.
 */
uint32_t shash(const unsigned char *p)
{
	unsigned int hval = HASHINIT;

	if (!p)
		return (0);
	for (; *p != '\0'; ++p)
	{
		hval += (hval << 1) + (hval << 4) + (hval << 7) + (hval << 8) + (hval << 24);
		hval ^= (ToLower(*p) ^ 16);
	}

	return ((hval >> HASHBITS) ^ (hval & ((1 << HASHBITS) - 1)) % HASHSIZE);
}

/* replace all occurances of 'old' with 'new' */
char *replace(char *s, int32_t size, const char *old, const char *new)
{
	char *ptr = s;
	int32_t left = strlen(s);
	int32_t avail = size - (left + 1);
	int32_t oldlen = strlen(old);
	int32_t newlen = strlen(new);
	int32_t diff = newlen - oldlen;

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
			memmove(ptr + oldlen + diff, ptr + oldlen, left + 1);

		strncpy(ptr, new, newlen);
		ptr += newlen;
		left -= oldlen;
	}

	return s;
}

/* reverse of atoi() */
#ifndef _WIN32
char *itoa(int num)
{
	static char ret[32];
	sprintf(ret, "%d", num);
	return ret;
}
#else
char *r_itoa(int num)
{
	static char ret[32];
	sprintf(ret, "%d", num);
	return ret;
}
#endif

/* return the time elapsed since an event */
char *time_ago(time_t event)
{
	static char ret[128];
	int years, weeks, days, hours, minutes, seconds;

	event = CURRTIME - event;
	years = weeks = days = hours = minutes = seconds = 0;

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
	unsigned long i, j, k;

	i = rand() % (CURRTIME / (cnt.user + 1) + 1);
	j = rand() % (me.start * cnt.chan + 1);

	if (i > j)
		k = (i - j) + strlen(chansvs.user);
	else
		k = (j - i) + strlen(chansvs.host);

	/* shorten or pad it to 9 digits */
	if (k > 1000000000)
		k = k - 1000000000;
	if (k < 100000000)
		k = k + 100000000;

	return k;
}

boolean_t is_internal_client(user_t *u)
{
	return (u && (!u->server || u->server == me.me));
}

int validemail(char *email)
{
	int i, valid = 1, chars = 0;

	/* sane length */
	if (strlen(email) >= EMAILLEN)
		valid = 0;

	/* make sure it has @ and . */
	if (!strchr(email, '@') || !strchr(email, '.'))
		valid = 0;

	/* check for other bad things */
	if (strchr(email, '\'') || strchr(email, ' ') || strchr(email, ',') || strchr(email, '$')
	    || strchr(email, '/') || strchr(email, ';') || strchr(email, '<') || strchr(email, '>') || strchr(email, '&') || strchr(email, '"'))
		valid = 0;

	/* make sure there are at least 6 characters besides the above
	 * mentioned @ and .
	 */
	for (i = strlen(email) - 1; i > 0; i--)
		if (!(email[i] == '@' || email[i] == '.'))
			chars++;

	if (chars < 6)
		valid = 0;

	return valid;
}

boolean_t validhostmask(char *host)
{
	if (strchr(host, ' '))
		return FALSE;

	/* make sure it has ! and @ */
	if (!strchr(host, '!') || !strchr(host, '@'))
		return FALSE;

	/* XXX this NICKLEN is too long */
	if (strlen(host) > NICKLEN + USERLEN + HOSTLEN + 1)
		return FALSE;

	if (host[0] == ',' || host[0] == '-' || host[0] == '#' || host[0] == '@' || host[0] == '!')
		return FALSE;

	return TRUE;
}

/* send the specified type of email.
 *
 * what is what we're sending to, a nickname.
 *
 * param is an extra parameter; email, key, etc.
 *
 * assume that we are either using NickServ or UserServ
 *
 * u is whoever caused this to be called, the corresponding service
 *   in case of xmlrpc
 * type is EMAIL_*, see include/tools.h
 * mu is the recipient user
 * param depends on type, also see include/tools.h
 *
 * XXX -- sendemail() is broken on Windows.
 */
int sendemail(user_t *u, int type, myuser_t *mu, const char *param)
{
#ifndef _WIN32
	char *email, *date = NULL;
	char cmdbuf[512], timebuf[256], to[128], from[128], subject[128];
	FILE *out;
	time_t t;
	struct tm tm;
	int pipfds[2];
	pid_t pid;
	int status;
	int rc;
	static time_t period_start = 0, lastwallops = 0;
	static int emailcount = 0;

	if (u == NULL || mu == NULL)
		return 0;

	if (me.mta == NULL)
	{
		if (type != EMAIL_MEMO && !is_internal_client(u))
			notice(opersvs.me ? opersvs.nick : me.name, u->nick, "Sending email is administratively disabled.");
		return 0;
	}

	if (type == EMAIL_SETEMAIL)
	{
		/* special case for e-mail change */
		metadata_t *md = metadata_find(mu, METADATA_USER, "private:verify:emailchg:newemail");

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

	if (CURRTIME - period_start > me.emailtime)
	{
		emailcount = 0;
		period_start = CURRTIME;
	}
	emailcount++;
	if (emailcount > me.emaillimit)
	{
		if (CURRTIME - lastwallops > 60)
		{
			wallops("Rejecting email for %s[%s@%s] due to too high load (type %d to %s <%s>)",
					u->nick, u->user, u->vhost,
					type, mu->name, email);
			slog(LG_ERROR, "sendemail(): rejecting email for %s[%s@%s] (%s) due to too high load (type %d to %s <%s>)",
					u->nick, u->user, u->vhost,
					u->ip[0] ? u->ip : u->host,
					type, mu->name, email);
			lastwallops = CURRTIME;
		}
		return 0;
	}

	slog(LG_INFO, "sendemail(): email for %s[%s@%s] (%s) type %d to %s <%s>",
			u->nick, u->user, u->vhost, u->ip[0] ? u->ip : u->host,
			type, mu->name, email);

	/* set up the email headers */
	time(&t);
	tm = *gmtime(&t);
	strftime(timebuf, sizeof(timebuf) - 1, "%a, %d %b %Y %H:%M:%S +0000", &tm);

	date = timebuf;

	snprintf(from, sizeof from, "%s automailer <noreply.%s>",
			me.netname, me.adminemail);
	snprintf(to, sizeof to, "%s <%s>", mu->name, email);

	strlcpy(subject, me.netname, sizeof subject);
	strlcat(subject, " ", sizeof subject);
	if (type == EMAIL_REGISTER)
		if (nicksvs.no_nick_ownership)
			strlcat(subject, "Account Registration", sizeof subject);
		else
			strlcat(subject, "Nickname Registration", sizeof subject);
	else if (type == EMAIL_SENDPASS)
		strlcat(subject, "Password Retrieval", sizeof subject);
	else if (type == EMAIL_SETEMAIL)
		strlcat(subject, "Change Email Confirmation", sizeof subject);
	else if (type == EMAIL_MEMO)
		strlcat(subject, "New memo", sizeof subject);
	
	/* now set up the email */
	sprintf(cmdbuf, "%s %s", me.mta, email);
	if (pipe(pipfds) < 0)
		return 0;
	switch (pid = fork())
	{
		case -1:
			return 0;
		case 0:
			close(pipfds[1]);
			dup2(pipfds[0], 0);
			switch (fork())
			{
				/* fork again to avoid zombies -- jilles */
				case -1:
					_exit(255);
				case 0:
					execl("/bin/sh", "sh", "-c", cmdbuf, NULL);
					_exit(255);
				default:
					_exit(0);
			}
	}
	close(pipfds[0]);
	waitpid(pid, &status, 0);
	out = fdopen(pipfds[1], "w");

	fprintf(out, "From: %s\n", from);
	fprintf(out, "To: %s\n", to);
	fprintf(out, "Subject: %s\n", subject);
	fprintf(out, "Date: %s\n\n", date);

	fprintf(out, "%s,\n\n", mu->name);

	if (type == EMAIL_REGISTER)
	{
		fprintf(out, "In order to complete your registration, you must send the following\ncommand on IRC:\n");
		fprintf(out, "/MSG %s VERIFY REGISTER %s %s\n\n", nicksvs.nick, mu->name, param);
		fprintf(out, "Thank you for registering your %s on the %s IRC " "network!\n\n",
				(nicksvs.nick ? "nickname" : "account"), me.netname);
	}
	else if (type == EMAIL_SENDPASS)
	{
		fprintf(out, "Someone has requested the password for %s be sent to the\n" "corresponding email address. If you did not request this action\n" "please let us know.\n\n", mu->name);
		fprintf(out, "The password for %s is %s. Please write this down for " "future reference.\n\n", mu->name, param);
	}
	else if (type == EMAIL_SETEMAIL)
	{
		fprintf(out, "In order to complete your email change, you must send\n" "the following command on IRC:\n");
		fprintf(out, "/MSG %s VERIFY EMAILCHG %s %s\n\n", nicksvs.nick, mu->name, param);
	}
	if (type == EMAIL_MEMO)
	{
		if (u->myuser != NULL)
			fprintf(out,"You have a new memo from %s.\n\n", u->myuser->name);
		else
			/* shouldn't happen */
			fprintf(out,"You have a new memo from %s (unregistered?).\n\n", u->nick);
		fprintf(out,"%s\n\n", param);
	}

	fprintf(out, "Thank you for your interest in the %s IRC network.\n", me.netname);
	if (u->server != me.me)
		fprintf(out, "\nThis email was sent due to a command from %s[%s@%s]\nat %s.\n", u->nick, u->user, u->vhost, date);
	fprintf(out, "If this message is spam, please contact %s\nwith a full copy.\n",
			me.adminemail);
	fprintf(out, ".\n");
	rc = 1;
	if (ferror(out))
		rc = 0;
	if (fclose(out) < 0)
		rc = 0;
	if (rc == 0)
		slog(LG_ERROR, "sendemail(): mta failure");
	return rc;
#else
	return 0;
#endif
}

/* various access level checkers */
boolean_t is_founder(mychan_t *mychan, myuser_t *myuser)
{
	if (!myuser)
		return FALSE;

	if (mychan->founder == myuser)
		return TRUE;

	return FALSE;
}

boolean_t should_owner(mychan_t *mychan, myuser_t *myuser)
{
	if (!myuser)
		return FALSE;

	if (MC_NOOP & mychan->flags)
		return FALSE;

	if (MU_NOOP & myuser->flags)
		return FALSE;

	if (is_founder(mychan, myuser))
		return TRUE;

	return FALSE;
}

boolean_t should_protect(mychan_t *mychan, myuser_t *myuser)
{
	if (!myuser)
		return FALSE;

	if (MC_NOOP & mychan->flags)
		return FALSE;

	if (MU_NOOP & myuser->flags)
		return FALSE;

	if (chanacs_find(mychan, myuser, CA_SET))
		return TRUE;

	return FALSE;
}

boolean_t is_ircop(user_t *user)
{
	if (UF_IRCOP & user->flags)
		return TRUE;

	return FALSE;
}

boolean_t is_admin(user_t *user)
{
	if (UF_ADMIN & user->flags)
		return TRUE;

	return FALSE;
}

void set_password(myuser_t *mu, char *newpassword)
{
	if (mu == NULL || newpassword == NULL)
		return;

	/* if we can, try to crypt it */
	if (crypto_module_loaded == TRUE)
	{
		mu->flags |= MU_CRYPTPASS;
		strlcpy(mu->pass, crypt_string(newpassword, gen_salt()), NICKLEN);
	}
	else
	{
		mu->flags &= ~MU_CRYPTPASS;			/* just in case */
		strlcpy(mu->pass, newpassword, NICKLEN);
	}
}

boolean_t verify_password(myuser_t *mu, char *password)
{
	if (mu == NULL || password == NULL)
		return FALSE;

	if (mu->flags & MU_CRYPTPASS)
		if (crypto_module_loaded == TRUE)
			return crypt_verify_password(password, mu->pass);
		else
		{	/* not good! */
			slog(LG_ERROR, "check_password(): can't check crypted password -- no crypto module!");
			return FALSE;
		}
	else
		return (strcmp(mu->pass, password) == 0);
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
