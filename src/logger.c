/*
 * Copyright (c) 2007 William Pitcock <nenolod -at- atheme.org>
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains logging routines.
 *
 * $Id: logger.c 8049 2007-04-02 12:40:41Z nenolod $
 */

#include "atheme.h"

logfile_t *log_file;
int log_force;

list_t log_files = { NULL, NULL, 0 };

/* private destructor function for logfile_t. */
static void logfile_delete(void *vdata)
{
	logfile_t *lf = (logfile_t *) vdata;

	node_del(&lf->node, &log_files);

	fclose(lf->log_file);
	free(lf->log_path);
	free(lf);
}

/*
 * logfile_new(const char *log_path, unsigned int log_mask)
 *
 * Logfile object factory.
 *
 * Inputs:
 *       - path to new logfile
 *       - bitmask of events to log
 *
 * Outputs:
 *       - a logfile_t object describing how this logfile should be used
 *
 * Side Effects:
 *       - log_files is populated with the newly created logfile_t.
 */
logfile_t *logfile_new(const char *log_path, unsigned int log_mask)
{
	static time_t lastfail = 0;
	logfile_t *lf = scalloc(sizeof(logfile_t), 1);

	object_init(object(lf), log_path, logfile_delete);
	if ((lf->log_file = fopen(log_path, "a")) == NULL)
	{
		free(lf);

		if (me.connected && lastfail + 3600 < CURRTIME)
		{
			lastfail = CURRTIME;
			wallops(_("Could not open log file (%s), log entries will be missing!"), strerror(errno));
		}

		return NULL;
	}
#ifdef FD_CLOEXEC
	fcntl(fileno(lf->log_file), F_SETFD, FD_CLOEXEC);
#endif

	lf->log_path = sstrdup(log_path);
	lf->log_mask = log_mask;

	node_add(lf, &lf->node, &log_files);

	return lf;
}

/*
 * log_open(void)
 *
 * Initializes the logging subsystem.
 *
 * Inputs:
 *       - none
 *
 * Outputs:
 *       - none
 *
 * Side Effects:
 *       - the log_files list is populated with the master
 *         atheme.log reference.
 */
void log_open(void)
{
	log_file = logfile_new(log_path, me.loglevel);
}

/*
 * log_shutdown(void)
 *
 * Shuts down the logging subsystem.
 *
 * Inputs:
 *       - none
 *
 * Outputs:
 *       - none
 *
 * Side Effects:
 *       - logfile_t objects in the log_files list are destroyed.
 */
void log_shutdown(void)
{
	node_t *n, *tn;

	LIST_FOREACH_SAFE(n, tn, log_files.head)
		object_unref(n->data);
}

/*
 * slog(unsigned int level, const char *fmt, ...)
 *
 * Handles the basic logging of log messages to the log files defined
 * in the configuration file. All I/O is handled here, and no longer
 * in the various sourceinfo_t log transforms.
 *
 * Inputs:
 *       - a bitmask of the various log categories this event qualifies to
 *       - a printf-style format string
 *       - (optional) additional arguments to be used with that format string
 *
 * Outputs:
 *       - none
 *
 * Side Effects:
 *       - logfiles are updated depending on how they are configured.
 */
void slog(unsigned int level, const char *fmt, ...)
{
	va_list args;
	time_t t;
	struct tm tm;
	char datetime[64];
	char buf[BUFSIZE];
	node_t *n;

	va_start(args, fmt);

	time(&t);
	tm = *localtime(&t);
	strftime(datetime, sizeof(datetime) - 1, "[%d/%m/%Y %H:%M:%S]", &tm);

	vsnprintf(buf, BUFSIZE, fmt, args);

	LIST_FOREACH(n, log_files.head)
	{
		logfile_t *lf = (logfile_t *) n->data;

		if ((level & lf->log_mask) && !log_force)
			continue;

		return_if_fail(lf->log_file != NULL);

		fprintf(lf->log_file, "%s %s\n", datetime, buf);
		fflush(lf->log_file);
	}

	/* 
	 * if the event is in the default loglevel, and we are starting, then
	 * display it in the controlling terminal.
	 */
	if ((runflags & (RF_LIVE | RF_STARTING)) && me.loglevel & level ||
		((runflags & RF_LIVE) && log_force))
		fprintf(stderr, "%s %s\n", datetime, buf);

	va_end(args);
}

/*
 * logcommand(sourceinfo_t *si, int level, const char *fmt, ...)
 *
 * Logs usage of a command from a user or other source (RPC call) as
 * described by sourceinfo_t.
 *
 * Inputs:
 *       - sourceinfo_t object which describes the source of the command call
 *       - bitmask of log categories to log the command use to.
 *       - printf-style format string
 *       - (optional) additional parameters
 *
 * Outputs:
 *       - none
 *
 * Side Effects:
 *       - qualifying logfile_t objects in log_files are updated.
 */
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

/*
 * logcommand_user(service_t *svs, user_t *source, int level, const char *fmt, ...)
 *
 * Logs usage of a command from a user as described by sourceinfo_t.
 *
 * Inputs:
 *       - service_t object which describes the service the command is attached to
 *       - user_t object which describes the source of the command call
 *       - bitmask of log categories to log the command use to.
 *       - printf-style format string
 *       - (optional) additional parameters
 *
 * Outputs:
 *       - none
 *
 * Side Effects:
 *       - qualifying logfile_t objects in log_files are updated.
 */
void logcommand_user(service_t *svs, user_t *source, int level, const char *fmt, ...)
{
	va_list args;
	time_t t;
	struct tm tm;
	char lbuf[BUFSIZE];

	va_start(args, fmt);
	vsnprintf(lbuf, BUFSIZE, fmt, args);

	slog(level, "%s %s:%s!%s@%s[%s] %s\n",
			svs != NULL ? svs->name : me.name,
			source->myuser != NULL ? source->myuser->name : "",
			source->nick, source->user, source->vhost,
			source->ip[0] != '\0' ? source->ip : source->host,
			lbuf);

	va_end(args);
}

/*
 * logcommand_external(service_t *svs, const char *type, connection_t *source,
 *       const char *sourcedesc, myuser_t *login, int level, const char *fmt, ...)
 *
 * Logs usage of a command from an RPC call as described by sourceinfo_t.
 *
 * Inputs:
 *       - service_t object which describes the service the command is attached to
 *       - string which describes the type of RPC call
 *       - connection_t which describes the socket source of the RPC call
 *       - string which describes the socket source of the RPC call
 *       - myuser_t which describes the services account used for the RPC call
 *       - bitmask of log categories to log the command use to.
 *       - printf-style format string
 *       - (optional) additional parameters
 *
 * Outputs:
 *       - none
 *
 * Side Effects:
 *       - qualifying logfile_t objects in log_files are updated.
 */
void logcommand_external(service_t *svs, const char *type, connection_t *source, const char *sourcedesc, myuser_t *login, int level, const char *fmt, ...)
{
	va_list args;
	time_t t;
	struct tm tm;
	char lbuf[BUFSIZE];

	va_start(args, fmt);
	vsnprintf(lbuf, BUFSIZE, fmt, args);

	slog(level, "%s %s:%s(%s)[%s] %s\n",
			svs != NULL ? svs->name : me.name,
			login != NULL ? login->name : "",
			type,
			source != NULL ? source->hbuf : "<noconn>",
			sourcedesc != NULL ? sourcedesc : "<unknown>",
			lbuf);

	va_end(args);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
