/*
 * Copyright (c) 2005 - 2007 atheme.org
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains logging routines.
 *
 * $Id: function.c 8027 2007-04-02 10:47:18Z nenolod $
 */

#include "atheme.h"

FILE *log_file;
int log_force;

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
			wallops(_("Could not open log file (%s), log entries will be missing!"), strerror(errno)); 
		}
		return;
	}
#ifndef _WIN32
	fcntl(fileno(log_file), F_SETFD, FD_CLOEXEC);
#endif
}

/* logs something to shrike.log */
void slog(unsigned int level, const char *fmt, ...)
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

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
