/*
 * atheme-services: A collection of minimalist IRC services   
 * logger.c: Logging routines
 *
 * Copyright (c) 2005-2009 Atheme Project (http://www.atheme.org)           
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

static logfile_t *log_file;
int log_force;

static list_t log_files = { NULL, NULL, 0 };

/* private destructor function for logfile_t. */
static void logfile_delete_file(void *vdata)
{
	logfile_t *lf = (logfile_t *) vdata;

	logfile_unregister(lf);

	fclose(lf->log_file);
	free(lf->log_path);
	metadata_delete_all(lf);
	free(lf);
}

static void logfile_delete_channel(void *vdata)
{
	logfile_t *lf = (logfile_t *) vdata;

	logfile_unregister(lf);

	if (opersvs.nick != NULL)
		part(lf->log_path, opersvs.nick);

	free(lf->log_path);
	metadata_delete_all(lf);
	free(lf);
}

static void logfile_join_channels(hook_channel_joinpart_t *hdata)
{
	channel_t *c;
	node_t *n;

	return_if_fail(hdata != NULL);

	if (hdata->cu == NULL)
		return;

	c = hdata->cu->chan;

	LIST_FOREACH(n, log_files.head)
	{
		logfile_t *lf = n->data;

		if (!irccasecmp(c->name, lf->log_path))
		{
			mowgli_patricia_iteration_state_t state;
			service_t *svs;

			if (lf->channel_joined)
				return;

			MOWGLI_PATRICIA_FOREACH(svs, &state, services_name)
			{
				if (svs->me == NULL)
					continue;

				join(lf->log_path, svs->me->nick);
			}

			lf->channel_joined = true;
		}
	}
}

/*
 * logfile_strip_control_codes(const char *buf)
 *
 * Duplicates a string, stripping control codes in the
 * process.
 *
 * Inputs:
 *       - buffer to strip
 *
 * Outputs:
 *       - duplicated buffer without control codes
 *
 * Side Effects:
 *       - none
 */
static const char *
logfile_strip_control_codes(const char *buf)
{
	static char outbuf[BUFSIZE];
	const char *in = buf;
	char *out = outbuf;

	for (; *in != '\0'; in++)
	{
		if (*in > 31)
		{
			*out++ = *in;
			continue;
		}
		else if (*in == 3)
		{
			in++;
			while (isdigit(*in))
				in++;
		}
	}

	*out = '\0';
	return outbuf;
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

	fprintf((FILE *) lf->log_file, "%s %s\n", datetime, logfile_strip_control_codes(buf));
	fflush((FILE *) lf->log_file);
}

/*
 * logfile_write_irc(logfile_t *lf, const char *buf)
 *
 * Writes an I/O stream to an IRC target.
 *
 * Inputs:
 *       - logfile_t representing the I/O stream.
 *       - data to write to the IRC target
 *
 * Outputs:
 *       - none
 *
 * Side Effects:
 *       - none
 */
void logfile_write_irc(logfile_t *lf, const char *buf)
{
	return_if_fail(lf != NULL);
	return_if_fail(lf->log_path != NULL);
	return_if_fail(buf != NULL);

	if (me.connected == false)
		return;

	if (lf->channel_joined == true)
	{
		size_t targetlen;
		char targetbuf[NICKLEN];
		service_t *svs = NULL;

		*targetbuf = '\0';
		targetlen = (strchr(buf, ' ') - buf);

		if (targetlen < NICKLEN)
		{
			strncpy(targetbuf, buf, targetlen);

			svs = service_find_nick(targetbuf);
			targetlen++;

			if (svs == NULL)
				targetlen = 0;
		}
		else
			targetlen = 0;

		msg(svs != NULL ? svs->me->nick : opersvs.nick, lf->log_path, "%s", (buf + targetlen));
	}
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
 * Unregisters a log I/O stream.
 *
 * Inputs:
 *       - logfile_t representing the I/O stream.
 *
 * Outputs:
 *       - none
 *
 * Side Effects:
 *       - the given object is removed from log_files, but remains valid.
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
	static bool hooked = false;
	static time_t lastfail = 0;
	logfile_t *lf = scalloc(sizeof(logfile_t), 1);

	if (*path != '#')
	{
		object_init(object(lf), path, logfile_delete_file);
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
		fcntl(fileno((FILE *)lf->log_file), F_SETFD, FD_CLOEXEC);
#endif
		lf->log_path = sstrdup(path);
		lf->write_func = logfile_write;
	}
	else
	{
		object_init(object(lf), path, logfile_delete_channel);

		lf->log_path = sstrdup(path);
		lf->write_func = logfile_write_irc;

		if (!hooked)
		{
			hook_add_event("channel_join");
			hook_add_channel_join(logfile_join_channels);
			hooked = true;
		}

		if (me.connected)
		{
			join(lf->log_path, opersvs.nick);
			lf->channel_joined = true;
		}
	}

	lf->log_mask = log_mask;

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
bool log_debug_enabled(void)
{
	node_t *n;
	logfile_t *lf;

	if (log_force)
		return true;
	LIST_FOREACH(n, log_files.head)
	{
		lf = n->data;
		if (lf->log_mask & (LG_DEBUG | LG_RAWDATA))
			return true;
	}
	return false;
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
 * logfile_find_mask(unsigned int log_mask)
 *
 * Returns a log file that logs all of the bits in log_mask (and possibly
 * more), or NULL if there is none. If an exact match exists, it is returned.
 *
 * Inputs:
 *       - log mask
 *
 * Outputs:
 *       - logfile_t object pointer, guaranteed to be a file, not some
 *         other kind of log stream
 *
 * Side Effects:
 *       - none
 */
logfile_t *logfile_find_mask(unsigned int log_mask)
{
	node_t *n;
	logfile_t *lf;

	LIST_FOREACH(n, log_files.head)
	{
		lf = n->data;
		if (lf->write_func != logfile_write)
			continue;
		if (lf->log_mask == log_mask)
			return lf;
	}
	LIST_FOREACH(n, log_files.head)
	{
		lf = n->data;
		if (lf->write_func != logfile_write)
			continue;
		if ((lf->log_mask & log_mask) == log_mask)
			return lf;
	}
	return NULL;
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
	if (((runflags & (RF_LIVE | RF_STARTING)) && (log_file != NULL ? log_file->log_mask : LG_ERROR | LG_INFO) & level) ||
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
	char lbuf[BUFSIZE];

	va_start(args, fmt);
	vsnprintf(lbuf, BUFSIZE, fmt, args);
	va_end(args);

	slog(level, "%s %s:%s!%s@%s[%s] %s",
			svs != NULL ? svs->nick : me.name,
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
	char lbuf[BUFSIZE];

	va_start(args, fmt);
	vsnprintf(lbuf, BUFSIZE, fmt, args);
	va_end(args);

	slog(level, "%s %s:%s(%s)[%s] %s",
			svs != NULL ? svs->nick : me.name,
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
