/*
 * Copyright (c) 2007 William Pitcock <nenolod -at- atheme.org>
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains logging routines.
 *
 * $Id: logger.c 8153 2007-04-06 18:57:00Z nenolod $
 */

#include "atheme.h"

static logfile_t *log_file;
int log_force;

static list_t log_files = { NULL, NULL, 0 };

/* private destructor function for logfile_t. */
static void logfile_delete(void *vdata)
{
	logfile_t *lf = (logfile_t *) vdata;

	logfile_unregister(lf);

	fclose(lf->log_file);
	free(lf->log_path);
	free(lf);
}

/*
 * logfile_write(logfile_t *lf, const char *buf)
 *
 * Writes an I/O stream to a static file.
 *
 * Inputs:
 *       - logfile_t representing the I/O stream.
 *       - data to write to the file
 *
 * Outputs:
 *       - none
 *
 * Side Effects:
 *       - none
 */
void logfile_write(logfile_t *lf, const char *buf)
{
	char datetime[64];
	time_t t;
	struct tm tm;

	return_if_fail(lf != NULL);
	return_if_fail(lf->log_file != NULL);
	return_if_fail(buf != NULL);

	time(&t);
	tm = *localtime(&t);
	strftime(datetime, sizeof(datetime) - 1, "[%d/%m/%Y %H:%M:%S]", &tm);

	fprintf((FILE *) lf->log_file, "%s %s\n", datetime, buf);
	fflush((FILE *) lf->log_file);
}

/*
 * logfile_register(logfile_t *lf)
 *
 * Registers a log I/O stream.
 *
 * Inputs:
 *       - logfile_t representing the I/O stream.
 *
 * Outputs:
 *       - none
 *
 * Side Effects:
 *       - log_files is populated with the given object.
 */
void logfile_register(logfile_t *lf)
{
	node_add(lf, &lf->node, &log_files);
}

/*
 * logfile_unregister(logfile_t *lf)
 *
 * Registers a log I/O stream.
 *
 * Inputs:
 *       - logfile_t representing the I/O stream.
 *
 * Outputs:
 *       - none
 *
 * Side Effects:
 *       - log_files is populated with the given object.
 */
void logfile_unregister(logfile_t *lf)
{
	node_del(&lf->node, &log_files);
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
logfile_t *logfile_new(const char *path, unsigned int log_mask)
{
	static time_t lastfail = 0;
	logfile_t *lf = scalloc(sizeof(logfile_t), 1);

	object_init(object(lf), path, logfile_delete);
	if ((lf->log_file = fopen(path, "a")) == NULL)
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
	lf->log_path = sstrdup(path);
	lf->log_mask = log_mask;
	lf->write_func = logfile_write;

	logfile_register(lf);

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
	log_file = logfile_new(log_path, LG_ERROR | LG_INFO | LG_CMD_ADMIN);
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
 * log_debug_enabled(void)
 *
 * Determines whether debug logging (LG_DEBUG and/or LG_RAWDATA) is enabled.
 *
 * Inputs:
 *       - none
 *
 * Outputs:
 *       - boolean
 *
 * Side Effects:
 *       - none
 */
boolean_t log_debug_enabled(void)
{
	node_t *n;
	logfile_t *lf;

	if (log_force)
		return TRUE;
	LIST_FOREACH(n, log_files.head)
	{
		lf = n->data;
		if (lf->log_mask & (LG_DEBUG | LG_RAWDATA))
			return TRUE;
	}
	return FALSE;
}

/*
 * log_master_set_mask(unsigned int mask)
 *
 * Sets the logging mask for the main log file.
 *
 * Inputs:
 *       - mask
 *
 * Outputs:
 *       - none
 *
 * Side Effects:
 *       - logging mask updated
 */
void log_master_set_mask(unsigned int mask)
{
	/* couldn't be opened etc */
	if (log_file == NULL)
		return;
	log_file->log_mask = mask;
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
	char buf[BUFSIZE];
	node_t *n;
	char datetime[64];
	time_t t;
	struct tm tm;

	va_start(args, fmt);
	vsnprintf(buf, BUFSIZE, fmt, args);
	va_end(args);

	time(&t);
	tm = *localtime(&t);
	strftime(datetime, sizeof(datetime) - 1, "[%d/%m/%Y %H:%M:%S]", &tm);

	LIST_FOREACH(n, log_files.head)
	{
		logfile_t *lf = (logfile_t *) n->data;

		if ((lf != log_file || !log_force) && !(level & lf->log_mask))
			continue;

		return_if_fail(lf->write_func != NULL);

		lf->write_func(lf, buf);
	}

	/* 
	 * if the event is in the default loglevel, and we are starting, then
	 * display it in the controlling terminal.
	 */
	if ((runflags & (RF_LIVE | RF_STARTING)) && (log_file != NULL ? log_file->log_mask : LG_ERROR | LG_INFO) & level ||
		((runflags & RF_LIVE) && log_force))
		fprintf(stderr, "%s %s\n", datetime, buf);
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
	va_end(args);

	slog(level, "%s %s:%s!%s@%s[%s] %s",
			svs != NULL ? svs->name : me.name,
			source->myuser != NULL ? source->myuser->name : "",
			source->nick, source->user, source->vhost,
			source->ip[0] != '\0' ? source->ip : source->host,
			lbuf);
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
	va_end(args);

	slog(level, "%s %s:%s(%s)[%s] %s",
			svs != NULL ? svs->name : me.name,
			login != NULL ? login->name : "",
			type,
			source != NULL ? source->hbuf : "<noconn>",
			sourcedesc != NULL ? sourcedesc : "<unknown>",
			lbuf);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
