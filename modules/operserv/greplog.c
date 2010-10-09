/*
 * Copyright (c) 2007-2008 Jilles Tjoelker
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Searches through the logs.
 *
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/greplog", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void os_cmd_greplog(sourceinfo_t *si, int parc, char *parv[]);

command_t os_greplog = { "GREPLOG", N_("Searches through the logs."), PRIV_CHAN_AUSPEX, 3, os_cmd_greplog, { .path = "oservice/greplog" } };

void _modinit(module_t *m)
{
	service_named_bind_command("operserv", &os_greplog);
}

void _moddeinit(void)
{
	service_named_unbind_command("operserv", &os_greplog);
}

#define MAXMATCHES 100

static const char *get_logfile(const unsigned int *masks)
{
	logfile_t *lf;
	int i;

	for (i = 0; masks[i] != 0; i++)
	{
		lf = logfile_find_mask(masks[i]);
		if (lf != NULL)
			return lf->log_path;
	}
	return NULL;
}

static const char *get_commands_log(void)
{
	const unsigned int masks[] = {
		LG_CMD_ALL,
		LG_CMD_ADMIN | LG_CMD_REGISTER | LG_CMD_SET | LG_CMD_DO | LG_CMD_LOGIN,
		LG_CMD_ADMIN | LG_CMD_REGISTER | LG_CMD_SET | LG_CMD_DO,
		LG_CMD_ADMIN | LG_CMD_REGISTER | LG_CMD_SET,
		LG_CMD_ADMIN | LG_CMD_REGISTER,
		LG_CMD_ADMIN,
		0
	};
	return get_logfile(masks);
}

static const char *get_account_log(void)
{
	const unsigned int masks[] = {
		LG_CMD_REGISTER | LG_CMD_SET | LG_REGISTER,
		LG_CMD_REGISTER | LG_REGISTER,
		0
	};
	return get_logfile(masks);
}

/* GREPLOG <service> <mask> */
static void os_cmd_greplog(sourceinfo_t *si, int parc, char *parv[])
{
	const char *service, *pattern, *baselog;
	int maxdays, matches = -1, matches1, day, days, lines, linesv;
	FILE *in;
	char str[1024];
	char *p, *q;
	char logfile[256];
	time_t t;
	struct tm tm;
	mowgli_list_t loglines = { NULL, NULL, 0 };
	mowgli_node_t *n, *tn;

	/* require user, channel and server auspex
	 * (channel auspex checked via in command_t)
	 */
	if (!has_priv(si, PRIV_USER_AUSPEX))
	{
		command_fail(si, fault_noprivs, STR_NO_PRIVILEGE, PRIV_USER_AUSPEX);
		return;
	}
	if (!has_priv(si, PRIV_SERVER_AUSPEX))
	{
		command_fail(si, fault_noprivs, STR_NO_PRIVILEGE, PRIV_SERVER_AUSPEX);
		return;
	}

	if (parc < 2)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "GREPLOG");
		command_fail(si, fault_needmoreparams, _("Syntax: GREPLOG <service> <pattern> [days]"));
		return;
	}

	service = parv[0];
	pattern = parv[1];

	if (parc >= 3)
	{
		days = atoi(parv[2]);
		maxdays = !strcmp(service, "*") ? 120 : 30;
		if (days < 0 || days > maxdays)
		{
			command_fail(si, fault_badparams, _("Too many days, maximum is %d."), maxdays);
			return;
		}
	}
	else
		days = 0;

	baselog = !strcmp(service, "*") ? get_account_log() : get_commands_log();
	if (baselog == NULL)
	{
		command_fail(si, fault_badparams, _("There is no log file matching your request."));
		return;
	}

	for (day = 0; day <= days; day++)
	{
		if (day == 0)
			strlcpy(logfile, baselog, sizeof logfile);
		else
		{
			t = CURRTIME - day * 86400;
			tm = *gmtime(&t);
			snprintf(logfile, sizeof logfile, "%s.%04u%02u%02u",
					baselog, tm.tm_year + 1900,
					tm.tm_mon + 1, tm.tm_mday);
		}
		in = fopen(logfile, "r");
		if (in == NULL)
		{
			command_success_nodata(si, "Failed to open log file %s", logfile);
			continue;
		}
		if (matches == -1)
			matches = 0;
		matches1 = matches;
		lines = linesv = 0;
		while (fgets(str, sizeof str, in) != NULL)
		{
			p = strchr(str, '\n');
			if (p != NULL)
				*p = '\0';
			lines++;
			p = *str == '[' ? strchr(str, ']') : NULL;
			if (p == NULL)
				continue;
			p++;
			if (*p++ != ' ')
				continue;
			q = strchr(p, ' ');
			if (q == NULL)
				continue;
			linesv++;
			*q = '\0';
			if (strcmp(service, "*") && strcasecmp(service, p))
				continue;
			*q++ = ' ';
			if (match(pattern, q))
				continue;
			matches++;
			mowgli_node_add_head(sstrdup(str), mowgli_node_create(), &loglines);
			if (matches > MAXMATCHES)
			{
				n = loglines.tail;
				mowgli_node_delete(n, &loglines);
				free(n->data);
				mowgli_node_free(n);
			}
		}
		fclose(in);
		matches = matches1;
		MOWGLI_ITER_FOREACH_SAFE(n, tn, loglines.head)
		{
			p = n->data;
			matches++;
			command_success_nodata(si, "[%d] %s", matches, p);
			mowgli_node_delete(n, &loglines);
			free(n->data);
			mowgli_node_free(n);
		}
		if (matches == 0 && lines > linesv && lines > 0)
			command_success_nodata(si, "Log file may be corrupted, %d/%d unexpected lines", lines - linesv, lines);
		if (matches >= MAXMATCHES)
		{
			command_success_nodata(si, "Too many matches, halting search");
			break;
		}
	}

	logcommand(si, CMDLOG_ADMIN, "GREPLOG: \2%s\2 \2%s\2 (\2%d\2 matches)", service, pattern, matches);
	if (matches == 0)
		command_success_nodata(si, _("No lines matched pattern \2%s\2"), pattern);
	else if (matches > 0)
		command_success_nodata(si, ngettext(N_("\2%d\2 match for pattern \2%s\2"),
						    N_("\2%d\2 matches for pattern \2%s\2"), matches), matches, pattern);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
