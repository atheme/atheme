/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Copyright (c) 2005-2006 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains functionality which implements
 * the OperServ SQLINE command.
 *
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/sqline", FALSE, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void os_sqline_newuser(hook_user_nick_t *data);
static void os_sqline_chanjoin(hook_channel_joinpart_t *hdata);

static void os_cmd_sqline(sourceinfo_t *si, int parc, char *parv[]);
static void os_cmd_sqline_add(sourceinfo_t *si, int parc, char *parv[]);
static void os_cmd_sqline_del(sourceinfo_t *si, int parc, char *parv[]);
static void os_cmd_sqline_list(sourceinfo_t *si, int parc, char *parv[]);
static void os_cmd_sqline_sync(sourceinfo_t *si, int parc, char *parv[]);


command_t os_sqline = { "SQLINE", N_("Manages network name bans."), PRIV_MASS_AKILL, 3, os_cmd_sqline, { .path = "oservice/sqline" } };

command_t os_sqline_add = { "ADD", N_("Adds a network name ban"), AC_NONE, 2, os_cmd_sqline_add, { .path = "" } };
command_t os_sqline_del = { "DEL", N_("Deletes a network name ban"), AC_NONE, 1, os_cmd_sqline_del, { .path = "" } };
command_t os_sqline_list = { "LIST", N_("Lists all network name bans"), AC_NONE, 1, os_cmd_sqline_list, { .path = "" } };
command_t os_sqline_sync = { "SYNC", N_("Synchronises network name bans to servers"), AC_NONE, 0, os_cmd_sqline_sync, { .path = "" } };

mowgli_patricia_t *os_sqline_cmds;

void _modinit(module_t *m)
{
	if (ircd != NULL && qline_sts == generic_qline_sts)
	{
		slog(LG_INFO, "Module %s requires qline support, refusing to load.",
				m->header->name);
		m->mflags = MODTYPE_FAIL;
		return;
	}

	service_named_bind_command("operserv", &os_sqline);

	os_sqline_cmds = mowgli_patricia_create(strcasecanon);

	/* Add sub-commands */
	command_add(&os_sqline_add, os_sqline_cmds);
	command_add(&os_sqline_del, os_sqline_cmds);
	command_add(&os_sqline_list, os_sqline_cmds);
	command_add(&os_sqline_sync, os_sqline_cmds);

	hook_add_event("user_add");
	hook_add_user_add(os_sqline_newuser);
	hook_add_event("user_nickchange");
	hook_add_user_nickchange(os_sqline_newuser);
	hook_add_event("channel_join");
	hook_add_channel_join(os_sqline_chanjoin); 
}

void _moddeinit()
{
	service_named_unbind_command("operserv", &os_sqline);

	/* Delete sub-commands */
	command_delete(&os_sqline_add, os_sqline_cmds);
	command_delete(&os_sqline_del, os_sqline_cmds);
	command_delete(&os_sqline_list, os_sqline_cmds);
	command_delete(&os_sqline_sync, os_sqline_cmds);
	
	hook_del_user_add(os_sqline_newuser);
	hook_del_user_nickchange(os_sqline_newuser);
	hook_del_channel_join(os_sqline_chanjoin);

	mowgli_patricia_destroy(os_sqline_cmds, NULL, NULL);
}

static void os_sqline_newuser(hook_user_nick_t *data)
{
	user_t *u = data->u;
	qline_t *q;

	/* If the user has been killed, don't do anything. */
	if (!u)
		return;

	if (is_internal_client(u))
		return;
	q = qline_find_user(u);
	if (q != NULL)
	{
		/* Server didn't have that qline, send it again.
		 * To ensure qline exempt works on sqlines too, do
		 * not send a KILL. -- jilles */
		qline_sts("*", q->mask, q->duration ? q->expires - CURRTIME : 0, q->reason);
	}
}

static void os_sqline_chanjoin(hook_channel_joinpart_t *hdata)
{
	chanuser_t *cu = hdata->cu;
	qline_t *q;

	if (cu == NULL)
		return;

	if (is_internal_client(cu->user))
		return;
	q = qline_find_channel(cu->chan);
	if (q != NULL)
	{
		/* Server didn't have that qline, send it again.
		 * To ensure qline exempt works on sqlines too, do
		 * not send a KICK. -- jilles */
		qline_sts("*", q->mask, q->duration ? q->expires - CURRTIME : 0, q->reason);
	}
}

static void os_cmd_sqline(sourceinfo_t *si, int parc, char *parv[])
{
	/* Grab args */
	char *cmd = parv[0];
	command_t *c;
	
	/* Bad/missing arg */
	if (!cmd)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SQLINE");
		command_fail(si, fault_needmoreparams, _("Syntax: SQLINE ADD|DEL|LIST"));
		return;
	}

	c = command_find(os_sqline_cmds, cmd);
	if(c == NULL)
	{
		command_fail(si, fault_badparams, _("Invalid command. Use \2/%s%s help\2 for a command listing."), (ircd->uses_rcommand == FALSE) ? "msg " : "", si->service->disp);
		return;
	}

	command_exec(si->service, si, c, parc - 1, parv + 1);
}

static void os_cmd_sqline_add(sourceinfo_t *si, int parc, char *parv[])
{
	char *target = parv[0];
	char *token = strtok(parv[1], " ");
	char *treason, reason[BUFSIZE];
	long duration;
	char *s;
	qline_t *q;

	if (!target || !token)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SQLINE ADD");
		command_fail(si, fault_needmoreparams, _("Syntax: SQLINE ADD <nick|chan> [!P|!T <minutes>] <reason>"));
		return;
	}

	if (IsDigit(*target))
	{
		command_fail(si, fault_badparams, _("Invalid target: \2%s\2. You can not SQLINE UIDs."), target);
		return;
	}

	if (!strcasecmp(token, "!P"))
	{
		duration = 0;
		treason = strtok(NULL, "");

		if (treason)
			strlcpy(reason, treason, BUFSIZE);
		else
			strlcpy(reason, "No reason given", BUFSIZE);
	}
	else if (!strcasecmp(token, "!T"))
	{
		s = strtok(NULL, " ");
		treason = strtok(NULL, "");
		if (treason)
			strlcpy(reason, treason, BUFSIZE);
		else
			strlcpy(reason, "No reason given", BUFSIZE);
		if (s)
		{
			duration = (atol(s) * 60);
			while (isdigit(*s))
				s++;
			if (*s == 'h' || *s == 'H')
				duration *= 60;
			else if (*s == 'd' || *s == 'D')
				duration *= 1440;
			else if (*s == 'w' || *s == 'W')
				duration *= 10080;
			else if (*s == '\0')
				;
			else
				duration = 0;
			if (duration == 0)
			{
				command_fail(si, fault_badparams, _("Invalid duration given."));
				command_fail(si, fault_badparams, _("Syntax: SQLINE ADD <nick|chan> [!P|!T <minutes>] <reason>"));
				return;
			}
		}
		else {
			command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SQLINE ADD");
			command_fail(si, fault_needmoreparams, _("Syntax: SQLINE ADD <nick|chan> [!P|!T <minutes>] <reason>"));
			return;
		}

	}
	else
	{
		duration = config_options.kline_time;
		strlcpy(reason, token, BUFSIZE);
		treason = strtok(NULL, "");

		if (treason)
		{
			strlcat(reason, " ", BUFSIZE);
			strlcat(reason, treason, BUFSIZE);
		}			
	}

	char *p;
	int i = 0;

	if (*target != '#' && *target != '&')
	{
		/* make sure there's at least 3 non-wildcards */
		/* except if there are no wildcards at all */
		for (p = target; *p; p++)
		{
			if (*p != '*' && *p != '?' && *p != '.')
				i++;
		}

		if (i < 3 && (strchr(target, '*') || strchr(target, '?')))
		{
			command_fail(si, fault_badparams, _("Invalid target: \2%s\2. At least three non-wildcard characters are required."), target);
			return;
		}
	}

	if (qline_find(target))
	{
		command_fail(si, fault_nochange, _("SQLINE \2%s\2 is already matched in the database."), target);
		return;
	}

	q = qline_add(target, reason, duration, get_storage_oper_name(si));

	if (duration)
		command_success_nodata(si, _("Timed SQLINE on \2%s\2 was successfully added and will expire in %s."), q->mask, timediff(duration));
	else
		command_success_nodata(si, _("SQLINE on \2%s\2 was successfully added."), q->mask);

	verbose_wallops("\2%s\2 is \2adding\2 an \2SQLINE\2 for \2%s\2 -- reason: \2%s\2", get_oper_name(si), q->mask, 
		q->reason);
	logcommand(si, CMDLOG_ADMIN, "SQLINE:ADD: \2%s\2 (reason: \2%s\2)", q->mask, q->reason);
}

static void os_cmd_sqline_del(sourceinfo_t *si, int parc, char *parv[])
{
	char *target = parv[0];
	qline_t *q;
	unsigned int number;
	char *s; 

	if (!target)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SQLINE DEL");
		command_fail(si, fault_needmoreparams, _("Syntax: SQLINE DEL <nick|chan>"));
		return;
	}

	if (strchr(target, ','))
	{
		unsigned int start = 0, end = 0, i;
		char t[16];

		s = strtok(target, ",");

		do
		{
			if (strchr(s, ':'))
			{
				for (i = 0; *s != ':'; s++, i++)
					t[i] = *s;

				t[++i] = '\0';
				start = atoi(t);

				s++;	/* skip past the : */

				for (i = 0; *s != '\0'; s++, i++)
					t[i] = *s;

				t[++i] = '\0';
				end = atoi(t);

				for (i = start; i <= end; i++)
				{
					if (!(q = qline_find_num(i)))
					{
						command_fail(si, fault_nosuch_target, _("No such SQLINE with number \2%d\2."), i);
						continue;
					}

					command_success_nodata(si, _("SQLINE on \2%s\2 has been successfully removed."), q->mask);
					verbose_wallops("\2%s\2 is \2removing\2 an \2SQLINE\2 for \2%s\2 -- reason: \2%s\2",
						get_oper_name(si), q->mask, q->reason);

					logcommand(si, CMDLOG_ADMIN, "SQLINE:DEL: \2%s\2", q->mask);
					qline_delete(q->mask);
				}

				continue;
			}

			number = atoi(s);

			if (!(q = qline_find_num(number)))
			{
				command_fail(si, fault_nosuch_target, _("No such SQLINE with number \2%d\2."), number);
				return;
			}

			command_success_nodata(si, _("SQLINE on \2%s\2 has been successfully removed."), q->mask);
			verbose_wallops("\2%s\2 is \2removing\2 an \2SQLINE\2 for \2%s\2 -- reason: \2%s\2",
				get_oper_name(si), q->mask, q->reason);

			logcommand(si, CMDLOG_ADMIN, "SQLINE:DEL: \2%s\2", q->mask);
			qline_delete(q->mask);
		} while ((s = strtok(NULL, ",")));

		return;
	} 
	
	if (IsDigit(*target))
	{
		unsigned int start = 0, end = 0, i;
		char t[16];

		if (strchr(target, ':'))
		{
			for (i = 0; *target != ':'; target++, i++)
				t[i] = *target;

			t[++i] = '\0';
			start = atoi(t);

			target++;	/* skip past the : */

			for (i = 0; *target != '\0'; target++, i++)
				t[i] = *target;

			t[++i] = '\0';
			end = atoi(t);

			for (i = start; i <= end; i++)
			{
				if (!(q = qline_find_num(i)))
				{
					command_fail(si, fault_nosuch_target, _("No such SQLINE with number \2%d\2."), i);
					continue;
				}

				command_success_nodata(si, _("SQLINE on \2%s\2 has been successfully removed."), q->mask);
				verbose_wallops("\2%s\2 is \2removing\2 an \2SQLINE\2 for \2%s\2 -- reason: \2%s\2",
					get_oper_name(si), q->mask, q->reason);

				logcommand(si, CMDLOG_ADMIN, "SQLINE:DEL: \2%s\2", q->mask);
				qline_delete(q->mask);
			}

			return;
		}

		number = atoi(target);

		if (!(q = qline_find_num(number)))
		{
			command_fail(si, fault_nosuch_target, _("No such SQLINE with number \2%d\2."), number);
			return;
		}

		command_success_nodata(si, _("SQLINE on \2%s\2 has been successfully removed."), q->mask);
		
		verbose_wallops("\2%s\2 is \2removing\2 an \2SQLINE\2 for \2%s\2 -- reason: \2%s\2",
			get_oper_name(si), q->mask, q->reason);

		logcommand(si, CMDLOG_ADMIN, "SQLINE:DEL: \2%s\2", q->mask);
		qline_delete(q->mask);
		return;
	}

	
	if (!(q = qline_find(target)))
	{
		command_fail(si, fault_nosuch_target, _("No such SQLINE: \2%s\2."), target);
		return;
	}

	command_success_nodata(si, _("SQLINE on \2%s\2 has been successfully removed."), target);

	verbose_wallops("\2%s\2 is \2removing\2 an \2SQLINE\2 for \2%s\2 -- reason: \2%s\2",
		get_oper_name(si), q->mask, q->reason);

	logcommand(si, CMDLOG_ADMIN, "SQLINE:DEL: \2%s\2", target);
	qline_delete(target);
}

static void os_cmd_sqline_list(sourceinfo_t *si, int parc, char *parv[])
{
	char *param = parv[0];
	bool full = false;
	mowgli_node_t *n;
	qline_t *q;

	if (param != NULL && !strcasecmp(param, "FULL"))
		full = true;
	
	if (full)
		command_success_nodata(si, _("SQLINE list (with reasons):"));
	else
		command_success_nodata(si, _("SQLINE list:"));

	MOWGLI_ITER_FOREACH(n, qlnlist.head)
	{
		q = (qline_t *)n->data;

		if (q->duration && full)
			command_success_nodata(si, _("%d: %s - by \2%s\2 - expires in \2%s\2 - (%s)"), q->number, q->mask, q->setby, timediff(q->expires > CURRTIME ? q->expires - CURRTIME : 0), q->reason);
		else if (q->duration && !full)
			command_success_nodata(si, _("%d: %s - by \2%s\2 - expires in \2%s\2"), q->number, q->mask, q->setby, timediff(q->expires > CURRTIME ? q->expires - CURRTIME : 0));
		else if (!q->duration && full)
			command_success_nodata(si, _("%d: %s - by \2%s\2 - \2permanent\2 - (%s)"), q->number, q->mask, q->setby, q->reason);
		else
			command_success_nodata(si, _("%d: %s - by \2%s\2 - \2permanent\2"), q->number, q->mask, q->setby);
	}

	command_success_nodata(si, _("Total of \2%zu\2 %s in SQLINE list."), qlnlist.count, (qlnlist.count == 1) ? "entry" : "entries");
	logcommand(si, CMDLOG_GET, "SQLINE:LIST: \2%s\2", full ? " FULL" : "");
}

static void os_cmd_sqline_sync(sourceinfo_t *si, int parc, char *parv[])
{
	mowgli_node_t *n;
	qline_t *q;

	logcommand(si, CMDLOG_DO, "SQLINE:SYNC");

	MOWGLI_ITER_FOREACH(n, qlnlist.head)
	{
		q = (qline_t *)n->data;

		if (q->duration == 0)
			qline_sts("*", q->mask, 0, q->reason);
		else if (q->expires > CURRTIME)
			qline_sts("*", q->mask, q->expires - CURRTIME, q->reason);
	}

	command_success_nodata(si, _("SQLINE list synchronized to servers."));
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
