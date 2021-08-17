/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2014 Atheme Project (http://atheme.org/)
 * Copyright (C) 2018-2021 Atheme Development Group (https://atheme.github.io/)
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * atheme-services: A collection of minimalist IRC services
 * email.c: Utilities to handle e-mail addresses and send e-mails.
 */

#include <atheme.h>

static mowgli_list_t email_canonicalizers;

static const char *
sendemail_urlencode(const char *const restrict src)
{
	static char result[BUFSIZE];

	const size_t srclen = strlen(src);
	size_t offset = 0;

	for (size_t i = 0; i < srclen; i++)
	{
		if (! isalnum((unsigned char) src[i]))
		{
			char tmpbuf[8];

			(void) snprintf(tmpbuf, sizeof tmpbuf, "%02X", (unsigned int) src[i]);

			result[offset++] = '%';
			result[offset++] = tmpbuf[0];
			result[offset++] = tmpbuf[1];
		}
		else
			result[offset++] = src[i];
	}

	result[offset] = 0x00;

	return result;
}

#ifndef MOWGLI_OS_WIN
static void
sendemail_waited(const pid_t pid, const int status, void *const restrict data)
{
	if (! WIFEXITED(status) || WEXITSTATUS(status) != 0)
		(void) slog(LG_INFO, "%s: email for %s failed", MOWGLI_FUNC_NAME, (const char *) data);

	(void) sfree(data);
}
#endif

/* Re-canonicalize email addresses.
 * Call this after adding or removing an email_canonicalize hook.
 */
static void
canonicalize_emails(void)
{
	struct myentity_iteration_state state;
	struct myentity *mt;

	MYENTITY_FOREACH_T(mt, &state, ENT_USER)
	{
		struct myuser *mu = user(mt);

		strshare_unref(mu->email_canonical);
		mu->email_canonical = canonicalize_email(mu->email);
	}
}

void
register_email_canonicalizer(email_canonicalizer_fn func, void *user_data)
{
	struct email_canonicalizer_item *const item = smalloc(sizeof *item);
	item->func = func;
	item->user_data = user_data;

	mowgli_node_add(item, &item->node, &email_canonicalizers);

	canonicalize_emails();
}

void
unregister_email_canonicalizer(email_canonicalizer_fn func, void *user_data)
{
	mowgli_node_t *n, *tn;

	MOWGLI_LIST_FOREACH_SAFE(n, tn, email_canonicalizers.head)
	{
		struct email_canonicalizer_item *item = n->data;

		if (item->func == func && item->user_data == user_data)
		{
			mowgli_node_delete(&item->node, &email_canonicalizers);
			sfree(item);

			canonicalize_emails();

			return;
		}
	}
}

stringref
canonicalize_email(const char *email)
{
	mowgli_node_t *n, *tn;
	char buf[EMAILLEN + 1];

	if (email == NULL)
		return NULL;

	mowgli_strlcpy(buf, email, sizeof buf);

	MOWGLI_LIST_FOREACH_SAFE(n, tn, email_canonicalizers.head)
	{
		struct email_canonicalizer_item *item = n->data;

		item->func(buf, item->user_data);
	}

	return strshare_get(buf);
}

void
canonicalize_email_case(char email[static (EMAILLEN + 1)], void *user_data)
{
	strcasecanon(email);
}

int
validemail(const char *email)
{
	int i, valid = 1, chars = 0, atcnt = 0, dotcnt1 = 0;
	char c;
	const char *lastdot = NULL;

	/* sane length */
	if (strlen(email) > EMAILLEN)
		return 0;

	/* RFC2822 */
#define EXTRA_ATEXTCHARS "!#$%&'*+-/=?^_`{|}~"

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
		{
			atcnt++;
			dotcnt1 = 0;
		}
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
	if (isdigit((unsigned char)lastdot[1]))
		return 0;

	/* make sure there are at least 4 characters besides the above
	 * mentioned @ and .
	 */
	if (chars < 4)
		return 0;

	return valid;
}

bool
email_within_limits(const char *email)
{
	mowgli_node_t *n;
	struct myentity_iteration_state state;
	struct myentity *mt;
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
		struct myuser *mu = user(mt);

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

/* send the specified type of email.
 *
 * u is whoever caused this to be called, the corresponding service
 *   in case of xmlrpc
 * type is EMAIL_*, see include/tools.h
 * mu is the recipient user
 * param depends on type, also see include/tools.h
 */
int
sendemail(struct user *u, struct myuser *mu, const char *type, const char *email, const char *param)
{
#ifndef MOWGLI_OS_WIN
	char *date = NULL;
	char timebuf[BUFSIZE], to[BUFSIZE], from[BUFSIZE], buf[BUFSIZE], pathbuf[BUFSIZE], sourceinfo[BUFSIZE];
	FILE *in, *out;
	time_t t;
	struct tm *tm;
	int pipfds[2];
	pid_t pid;
	int rc;
	static time_t period_start = 0, lastwallops = 0;
	static unsigned int emailcount = 0;
	struct service *svs;

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
		if ((CURRTIME - lastwallops) > SECONDS_PER_MINUTE)
		{
			wallops("Rejecting email for %s[%s@%s] due to too high load (type '%s' to %s <%s>)",
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
	tm = localtime(&t);
	strftime(timebuf, sizeof timebuf, "%a, %d %b %Y %H:%M:%S %z", tm);

	date = timebuf;

	snprintf(from, sizeof from, "\"%s Network Services\" <%s>",
			me.netname, me.register_email);
	snprintf(to, sizeof to, "\"%s\" <%s>", entity(mu)->name, email);
	/* \ is special here; escape it */
	replace(to, sizeof to, "\\", "\\\\");
	snprintf(sourceinfo, sizeof sourceinfo, "%s[%s@%s]", u->nick, u->user, u->vhost);

	/* now set up the email */
	if (pipe(pipfds) < 0)
	{
		fclose(in);
		return 0;
	}
	switch (pid = fork())
	{
		case -1:
			fclose(in);
			close(pipfds[0]);
			close(pipfds[1]);
			return 0;
		case 0:
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
		replace(buf, sizeof buf, "&accountname&", entity(mu)->name);
		replace(buf, sizeof buf, "&urlaccountname&", sendemail_urlencode(entity(mu)->name));
		replace(buf, sizeof buf, "&entityname&", u->myuser ? entity(u->myuser)->name : u->nick);
		replace(buf, sizeof buf, "&netname&", me.netname);
		replace(buf, sizeof buf, "&param&", param);
		replace(buf, sizeof buf, "&sourceinfo&", sourceinfo);

		if ((svs = service_find("alis")) != NULL)
			replace(buf, sizeof buf, "&alissvs&", svs->me->nick);

		if ((svs = service_find("botserv")) != NULL)
			replace(buf, sizeof buf, "&botsvs&", svs->me->nick);

		if ((svs = service_find("chanfix")) != NULL)
			replace(buf, sizeof buf, "&chanfix&", svs->me->nick);

		if ((svs = service_find("chanserv")) != NULL)
			replace(buf, sizeof buf, "&chansvs&", svs->me->nick);

		if ((svs = service_find("gameserv")) != NULL)
			replace(buf, sizeof buf, "&gamesvs&", svs->me->nick);

		if ((svs = service_find("groupserv")) != NULL)
			replace(buf, sizeof buf, "&groupsvs&", svs->me->nick);

		if ((svs = service_find("helpserv")) != NULL)
			replace(buf, sizeof buf, "&helpsvs&", svs->me->nick);

		if ((svs = service_find("hostserv")) != NULL)
			replace(buf, sizeof buf, "&hostsvs&", svs->me->nick);

		if ((svs = service_find("infoserv")) != NULL)
			replace(buf, sizeof buf, "&infosvs&", svs->me->nick);

		if ((svs = service_find("memoserv")) != NULL)
			replace(buf, sizeof buf, "&memosvs&", svs->me->nick);

		if ((svs = service_find("nickserv")) != NULL)
			replace(buf, sizeof buf, "&nicksvs&", svs->me->nick);

		if ((svs = service_find("operserv")) != NULL)
			replace(buf, sizeof buf, "&opersvs&", svs->me->nick);

		if ((svs = service_find("rpgserv")) != NULL)
			replace(buf, sizeof buf, "&rpgsvs&", svs->me->nick);

		if ((svs = service_find("statserv")) != NULL)
			replace(buf, sizeof buf, "&statsvs&", svs->me->nick);

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
