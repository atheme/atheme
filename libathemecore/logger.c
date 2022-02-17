/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2014 Atheme Project (http://atheme.org/)
 * Copyright (C) 2017-2018 Atheme Development Group (https://atheme.github.io/)
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * atheme-services: A collection of minimalist IRC services
 * logger.c: Logging routines
 */

#include <atheme.h>
#include "internal.h"

static struct logfile *log_file;
int log_force;

static mowgli_list_t log_files = { NULL, NULL, 0 };

/* private destructor function for struct logfile. */
static void
logfile_delete_file(void *vdata)
{
	struct logfile *lf = (struct logfile *) vdata;

	logfile_unregister(lf);

	fclose(lf->log_file);
	sfree(lf->log_path);
	metadata_delete_all(lf);
	sfree(lf);
}

static void
logfile_delete_channel(void *vdata)
{
	struct logfile *lf = (struct logfile *) vdata;

	logfile_unregister(lf);

	sfree(lf->log_path);
	metadata_delete_all(lf);
	sfree(lf);
}

static void
logfile_join_channels(struct channel *c)
{
	mowgli_node_t *n;

	return_if_fail(c != NULL);

	MOWGLI_ITER_FOREACH(n, log_files.head)
	{
		struct logfile *lf = n->data;

		if (!irccasecmp(c->name, lf->log_path))
		{
			if (!(c->flags & CHAN_LOG))
			{
				c->flags |= CHAN_LOG;
				joinall(lf->log_path);
			}
			return;
		}
	}
}

static void
logfile_join_service(struct service *svs)
{
	mowgli_node_t *n;
	struct channel *c;

	return_if_fail(svs != NULL && svs->me != NULL);

	/* no botserv bots */
	if (svs->botonly)
		return;

	MOWGLI_ITER_FOREACH(n, log_files.head)
	{
		struct logfile *lf = n->data;

		if (!VALID_GLOBAL_CHANNEL_PFX(lf->log_path))
			continue;
		c = channel_find(lf->log_path);
		if (c == NULL || !(c->flags & CHAN_LOG))
			continue;
		join(c->name, svs->me->nick);
	}
}

static void
logfile_part_removed(void *unused)
{
	struct channel *c;
	mowgli_patricia_iteration_state_t state;
	mowgli_node_t *n;
	bool valid;

	MOWGLI_PATRICIA_FOREACH(c, &state, chanlist)
	{
		if (!(c->flags & CHAN_LOG))
			continue;
		valid = false;
		MOWGLI_ITER_FOREACH(n, log_files.head)
		{
			struct logfile *lf = n->data;

			if (!irccasecmp(c->name, lf->log_path))
			{
				valid = true;
				break;
			}
		}
		if (!valid)
		{
			c->flags &= ~CHAN_LOG;
			partall(c->name);
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
	char *out = outbuf;

	for (const char *in = buf; *in != '\0'; /* No action */)
	{
		if (*in > 31)
		{
			*out++ = *in++;
		}
		else if (*in == 3)
		{
			/* mIRC colour codes have to be treated a bit specially.
			 *
			 * They are the character 0x03 followed by up to 2 decimal foreground digits.
			 * This can optionally be followed by a comma and up to 2 more decimal background digits.
			 *
			 * We also have to gracefully handle the pathological cases where the input may be e.g.
			 * "\003,\003data" or "\003,12data", without skipping over the start of data, but without
			 * printing the 0x03 bytes or the comma, or background colour in the latter case.
			 */

			in++;

			for (size_t i = 0; i < 2 && isdigit((unsigned char) *in); i++, in++);

			if (*in == ',')
			{
				in++;

				for (size_t i = 0; i < 2 && isdigit((unsigned char) *in); i++, in++);
			}
		}
		else
			in++;
	}

	*out = '\0';
	return outbuf;
}

/*
 * logfile_write(struct logfile *lf, const char *buf)
 *
 * Writes an I/O stream to a static file.
 *
 * Inputs:
 *       - struct logfile representing the I/O stream.
 *       - data to write to the file
 *
 * Outputs:
 *       - none
 *
 * Side Effects:
 *       - none
 */
static void
logfile_write(struct logfile *lf, const char *buf)
{
	char datetime[BUFSIZE];
	time_t t;
	struct tm *tm;

	return_if_fail(lf != NULL);
	return_if_fail(lf->log_file != NULL);
	return_if_fail(buf != NULL);

	time(&t);
	tm = localtime(&t);
	strftime(datetime, sizeof datetime, "[%Y-%m-%d %H:%M:%S]", tm);

	fprintf((FILE *) lf->log_file, "%s %s\n", datetime, logfile_strip_control_codes(buf));
	fflush((FILE *) lf->log_file);
}

/*
 * logfile_write_irc(struct logfile *lf, const char *buf)
 *
 * Writes an I/O stream to an IRC target.
 *
 * Inputs:
 *       - struct logfile representing the I/O stream.
 *       - data to write to the IRC target
 *
 * Outputs:
 *       - none
 *
 * Side Effects:
 *       - none
 */
static void
logfile_write_irc(struct logfile *lf, const char *buf)
{
	struct channel *c;

	return_if_fail(lf != NULL);
	return_if_fail(lf->log_path != NULL);
	return_if_fail(buf != NULL);

	if (!me.connected || me.bursting)
		return;

	c = channel_find(lf->log_path);
	if (c != NULL && c->flags & CHAN_LOG)
	{
		size_t targetlen;
		char targetbuf[NICKLEN + 1];
		struct service *svs = NULL;

		memset(targetbuf, '\0', sizeof targetbuf);
		targetlen = (strchr(buf, ' ') - buf);

		if (targetlen < sizeof targetbuf)
		{
			mowgli_strlcpy(targetbuf, buf, sizeof targetbuf);
			targetbuf[targetlen] = '\0';

			svs = service_find_nick(targetbuf);
			targetlen++;

			if (svs == NULL)
				targetlen = 0;
		}
		else
			targetlen = 0;

		if (svs == NULL)
			svs = service_find("operserv");

		if (svs == NULL)
			svs = service_find_any();

		if (svs != NULL && svs->me != NULL)
			msg(svs->me->nick, c->name, "%s", (buf + targetlen));
		else
			wallops("%s", (buf + targetlen));
	}
}

/*
 * logfile_write_snotices(struct logfile *lf, const char *buf)
 *
 * Writes an I/O stream to IRC snotes.
 *
 * Inputs:
 *       - struct logfile representing the I/O stream.
 *       - data to write to the IRC target
 *
 * Outputs:
 *       - none
 *
 * Side Effects:
 *       - none
 */
static void
logfile_write_snotices(struct logfile *lf, const char *buf)
{
	struct channel *c;

	return_if_fail(lf != NULL);
	return_if_fail(lf->log_path != NULL);
	return_if_fail(buf != NULL);

	if (!me.connected || me.bursting)
		return;

	wallops("%s", buf);
}

/*
 * logfile_register(struct logfile *lf)
 *
 * Registers a log I/O stream.
 *
 * Inputs:
 *       - struct logfile representing the I/O stream.
 *
 * Outputs:
 *       - none
 *
 * Side Effects:
 *       - log_files is populated with the given object.
 */
void
logfile_register(struct logfile *lf)
{
	mowgli_node_add(lf, &lf->node, &log_files);
}

/*
 * logfile_unregister(struct logfile *lf)
 *
 * Unregisters a log I/O stream.
 *
 * Inputs:
 *       - struct logfile representing the I/O stream.
 *
 * Outputs:
 *       - none
 *
 * Side Effects:
 *       - the given object is removed from log_files, but remains valid.
 */
void
logfile_unregister(struct logfile *lf)
{
	mowgli_node_delete(&lf->node, &log_files);
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
 *       - a struct logfile object describing how this logfile should be used
 *
 * Side Effects:
 *       - log_files is populated with the newly created struct logfile.
 */
struct logfile * ATHEME_FATTR_MALLOC_UNCHECKED
logfile_new(const char *path, unsigned int log_mask)
{
	static bool hooked = false;
	static time_t lastfail = 0;
	struct channel *c;
	struct logfile *const lf = smalloc(sizeof *lf);

	if (!strcasecmp(path, "!snotices") || !strcasecmp(path, "!wallops"))
	{
		atheme_object_init(atheme_object(lf), path, logfile_delete_channel);

		lf->log_path = sstrdup(path);
		lf->log_type = LOG_INTERACTIVE;
		lf->write_func = logfile_write_snotices;
	}
	else if (!VALID_GLOBAL_CHANNEL_PFX(path))
	{
		errno = 0;

#ifndef O_CLOEXEC
		const bool cloexec = true;
		const int fd = open(path, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP);
#else
		bool cloexec = false;
		int fd = open(path, O_WRONLY | O_CREAT | O_APPEND | O_CLOEXEC, S_IRUSR | S_IWUSR | S_IRGRP);

		if (fd == -1)
		{
			cloexec = true;
			fd = open(path, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP);
		}
#endif

		if (fd == -1 || ! (lf->log_file = fdopen(fd, "a")))
		{
			if (me.connected && lastfail + SECONDS_PER_HOUR < CURRTIME)
			{
				lastfail = CURRTIME;
				wallops("Could not open log file '%s' (%s), log entries will be missing!",
				        path, strerror(errno));
			}

			sfree(lf);
			return NULL;
		}

#ifdef FD_CLOEXEC
		if (cloexec)
		{
			const int res = fcntl(fd, F_GETFD, NULL);

			if (res == -1 || fcntl(fd, F_SETFD, res | FD_CLOEXEC) == -1)
				wallops("Could not mark log file '%s' close-on-exec!", path);
		}
#else
		(void) cloexec;
#endif

		atheme_object_init(atheme_object(lf), path, logfile_delete_file);

		lf->log_path = sstrdup(path);
		lf->log_type = LOG_NONINTERACTIVE;
		lf->write_func = logfile_write;
	}
	else
	{
		atheme_object_init(atheme_object(lf), path, logfile_delete_channel);

		lf->log_path = sstrdup(path);
		lf->log_type = LOG_INTERACTIVE;
		lf->write_func = logfile_write_irc;

		c = channel_find(lf->log_path);

		if (me.connected && c != NULL)
		{
			joinall(lf->log_path);
			c->flags |= CHAN_LOG;
		}
		if (!hooked)
		{
			hook_add_channel_add(logfile_join_channels);
			hook_add_service_introduce(logfile_join_service);
			hook_add_config_ready(logfile_part_removed);
			hooked = true;
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
void
log_open(void)
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
 *       - struct logfile objects in the log_files list are destroyed.
 */
void
log_shutdown(void)
{
	mowgli_node_t *n, *tn;

	MOWGLI_ITER_FOREACH_SAFE(n, tn, log_files.head)
		atheme_object_unref(n->data);
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
bool
log_debug_enabled(void)
{
	mowgli_node_t *n;
	struct logfile *lf;

	if (log_force)
		return true;
	MOWGLI_ITER_FOREACH(n, log_files.head)
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
void
log_master_set_mask(unsigned int mask)
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
 *       - struct logfile object pointer, guaranteed to be a file, not some
 *         other kind of log stream
 *
 * Side Effects:
 *       - none
 */
struct logfile *
logfile_find_mask(unsigned int log_mask)
{
	mowgli_node_t *n;
	struct logfile *lf;

	MOWGLI_ITER_FOREACH(n, log_files.head)
	{
		lf = n->data;
		if (lf->write_func != logfile_write)
			continue;
		if (lf->log_mask == log_mask)
			return lf;
	}
	MOWGLI_ITER_FOREACH(n, log_files.head)
	{
		lf = n->data;
		if (lf->write_func != logfile_write)
			continue;
		if ((lf->log_mask & log_mask) == log_mask)
			return lf;
	}
	return NULL;
}

static void ATHEME_FATTR_PRINTF(3, 0)
vslog_ext(enum log_type type, unsigned int level, const char *fmt, va_list args)
{
	static bool in_vslog_ext = false;

	// Detect infinite logging recursion
	if (in_vslog_ext)
		return;

	in_vslog_ext = true;

	char buf[BUFSIZE];
	(void) vsnprintf(buf, sizeof buf, fmt, args);

	const mowgli_node_t *n;
	MOWGLI_ITER_FOREACH(n, log_files.head)
	{
		struct logfile *const lf = n->data;

		continue_if_fail(lf != NULL);
		continue_if_fail(lf->write_func != NULL);

		if (type != LOG_ANY && type != lf->log_type)
			continue;
		if ((lf != log_file || !log_force) && !(level & lf->log_mask))
			continue;

		(void) lf->write_func(lf, buf);
	}

	/* If the event is in the default loglevel, and we are starting, then
	 * display it in the controlling terminal.
	 */
	if (type != LOG_INTERACTIVE && ((runflags & (RF_LIVE | RF_STARTING) &&
		(log_file != NULL ? log_file->log_mask : LG_ERROR | LG_INFO) & level) ||
		(runflags & RF_LIVE && log_force)))
	{
		char datetime[BUFSIZE];
		const time_t ts = time(NULL);
		const struct tm *const tm = localtime(&ts);

		(void) strftime(datetime, sizeof datetime, "[%Y-%m-%d %H:%M:%S]", tm);
		(void) fprintf(stderr, "%s %s\n", datetime, logfile_strip_control_codes(buf));
	}

	in_vslog_ext = false;
}

static void ATHEME_FATTR_PRINTF(3, 4)
slog_ext(enum log_type type, unsigned int level, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vslog_ext(type, level, fmt, args);
	va_end(args);
}

/*
 * slog(unsigned int level, const char *fmt, ...)
 *
 * Handles the basic logging of log messages to the log files defined
 * in the configuration file. All I/O is handled here, and no longer
 * in the various struct sourceinfo log transforms.
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
void ATHEME_FATTR_PRINTF(2, 3)
slog(unsigned int level, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vslog_ext(LOG_ANY, level, fmt, args);
	va_end(args);
}

static const char *
format_user(struct user *source, bool full)
{
	static char buf[BUFSIZE];
	char accountbuf[(NICKLEN + 1) * 5]; /* entity name len is NICKLEN * 4, plus another for the ID */
	bool showaccount;

	accountbuf[0] = '\0';
	if (source->myuser != NULL)
		snprintf(accountbuf, sizeof accountbuf, "%s/%s",
				entity(source->myuser)->name, entity(source->myuser)->id);
	showaccount = source->myuser == NULL || irccasecmp(entity(source->myuser)->name, source->nick);

	if(full)
		snprintf(buf, sizeof buf, "%s:%s!%s@%s[%s]",
			accountbuf,
			source->nick, source->user, source->host,
			source->ip != NULL ? source->ip : source->host);
	else
		snprintf(buf, sizeof buf, "%s%s%s%s",
			source->nick,
			showaccount ? " (" : "",
			showaccount ? (source->myuser ? entity(source->myuser)->name : "") : "",
			showaccount ? ")" : "");
	return buf;
}

static const char *
format_external(const char *type, struct connection *source, const char *sourcedesc, struct myuser *mu, bool full)
{
	static char buf[BUFSIZE];

	if(full)
		snprintf(buf, sizeof buf, "%s:%s(%s)[%s]",
			mu != NULL ? entity(mu)->name : "",
			type,
			source != NULL ? source->name : "<noconn>",
			sourcedesc != NULL ? sourcedesc : "<unknown>");
	else
		snprintf(buf, sizeof buf, "<%s>%s",
			type,
			mu != NULL ? entity(mu)->name : "");
	return buf;
}

static const char *
format_sourceinfo(struct sourceinfo *si, bool full)
{
	if(si->v != NULL && si->v->format != NULL)
		return si->v->format(si, full);

	if(si->su != NULL)
		return format_user(si->su, full);
	else
		return format_external(si->v != NULL ? si->v->description : "unknown", si->connection, si->sourcedesc, si->smu, full);
}

/*
 * logcommand(struct sourceinfo *si, int level, const char *fmt, ...)
 *
 * Logs usage of a command from a user or other source (RPC call) as
 * described by struct sourceinfo.
 *
 * Inputs:
 *       - struct sourceinfo object which describes the source of the command call
 *       - bitmask of log categories to log the command use to.
 *       - printf-style format string
 *       - (optional) additional parameters
 *
 * Outputs:
 *       - none
 *
 * Side Effects:
 *       - qualifying struct logfile objects in log_files are updated.
 */
void ATHEME_FATTR_PRINTF(3, 4)
logcommand(struct sourceinfo *si, int level, const char *fmt, ...)
{
	va_list args;
	char lbuf[BUFSIZE];

	va_start(args, fmt);
	vsnprintf(lbuf, BUFSIZE, fmt, args);
	va_end(args);
	slog_ext(LOG_NONINTERACTIVE, level, "%s %s %s",
			service_get_log_target(si->service),
			format_sourceinfo(si, true),
			lbuf);
	slog_ext(LOG_INTERACTIVE, level, "%s %s %s",
			service_get_log_target(si->service),
			format_sourceinfo(si, false),
			lbuf);
}

/*
 * logcommand_user(struct service *svs, struct user *source, int level, const char *fmt, ...)
 *
 * Logs usage of a command from a user as described by struct sourceinfo.
 *
 * Inputs:
 *       - struct service object which describes the service the command is attached to
 *       - struct user object which describes the source of the command call
 *       - bitmask of log categories to log the command use to.
 *       - printf-style format string
 *       - (optional) additional parameters
 *
 * Outputs:
 *       - none
 *
 * Side Effects:
 *       - qualifying struct logfile objects in log_files are updated.
 */
void ATHEME_FATTR_PRINTF(4, 5)
logcommand_user(struct service *svs, struct user *source, int level, const char *fmt, ...)
{
	va_list args;
	char lbuf[BUFSIZE];

	va_start(args, fmt);
	vsnprintf(lbuf, BUFSIZE, fmt, args);
	va_end(args);

	slog_ext(LOG_NONINTERACTIVE, level, "%s %s %s",
			service_get_log_target(svs),
			format_user(source, true),
			lbuf);
	slog_ext(LOG_INTERACTIVE, level, "%s %s %s",
			service_get_log_target(svs),
			format_user(source, false),
			lbuf);
}

/*
 * logcommand_external(struct service *svs, const char *type, struct connection *source,
 *       const char *sourcedesc, struct myuser *login, int level, const char *fmt, ...)
 *
 * Logs usage of a command from an RPC call as described by struct sourceinfo.
 *
 * Inputs:
 *       - struct service object which describes the service the command is attached to
 *       - string which describes the type of RPC call
 *       - struct connection which describes the socket source of the RPC call
 *       - string which describes the socket source of the RPC call
 *       - struct myuser which describes the services account used for the RPC call
 *       - bitmask of log categories to log the command use to.
 *       - printf-style format string
 *       - (optional) additional parameters
 *
 * Outputs:
 *       - none
 *
 * Side Effects:
 *       - qualifying struct logfile objects in log_files are updated.
 */
void ATHEME_FATTR_PRINTF(7, 8)
logcommand_external(struct service *svs, const char *type, struct connection *source, const char *sourcedesc, struct myuser *mu, int level, const char *fmt, ...)
{
	va_list args;
	char lbuf[BUFSIZE];

	va_start(args, fmt);
	vsnprintf(lbuf, BUFSIZE, fmt, args);
	va_end(args);

	slog_ext(LOG_NONINTERACTIVE, level, "%s %s %s",
			service_get_log_target(svs),
			format_external(type, source, sourcedesc, mu, true),
			lbuf);
	slog_ext(LOG_INTERACTIVE, level, "%s %s %s",
			service_get_log_target(svs),
			format_external(type, source, sourcedesc, mu, false),
			lbuf);
}

/*
 * logaudit_denycmd(struct sourceinfo *si, struct command *cmd, const char *userlevel)
 *
 * Logs a command access denial for auditing.
 *
 * Inputs:
 *       - struct sourceinfo object that was denied
 *       - struct command object that was denied
 *       - optional userlevel (if none, will be NULL)
 *
 * Outputs:
 *       - nothing
 *
 * Side Effects:
 *       - qualifying struct logfile objects in log_files are updated
 */
void
logaudit_denycmd(struct sourceinfo *si, struct command *cmd, const char *userlevel)
{
	slog_ext(LOG_NONINTERACTIVE, LG_DENYCMD, "DENYCMD: [%s] was denied execution of [%s], need privileges [%s %s]",
		 get_source_security_label(si), cmd->name, cmd->access, userlevel != NULL ? userlevel : "");
	slog_ext(LOG_INTERACTIVE, LG_DENYCMD, "DENYCMD: \2%s\2 was denied execution of \2%s\2, need privileges \2%s %s\2",
		 get_source_security_label(si), cmd->name, cmd->access, userlevel != NULL ? userlevel : "");
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
