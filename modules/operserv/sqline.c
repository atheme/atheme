/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2003-2004 E. Will, et al.
 * Copyright (C) 2005-2006 Atheme Project (http://atheme.org/)
 *
 * This file contains functionality which implements
 * the OperServ SQLINE command.
 */

#include <atheme.h>

static mowgli_patricia_t *os_sqline_cmds = NULL;

static void
os_sqline_newuser(struct hook_user_nick *data)
{
	struct user *u = data->u;
	struct qline *q;

	// If the user has been killed, don't do anything.
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

static void
os_sqline_chanjoin(struct hook_channel_joinpart *hdata)
{
	struct chanuser *cu = hdata->cu;
	struct qline *q;

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

static void
os_cmd_sqline(struct sourceinfo *const restrict si, const int parc, char **const restrict parv)
{
	if (parc < 1)
	{
		(void) command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SQLINE");
		(void) command_fail(si, fault_needmoreparams, _("Syntax: SQLINE ADD|DEL|LIST"));
		return;
	}

	(void) subcommand_dispatch_simple(si->service, si, parc, parv, os_sqline_cmds, "SQLINE");
}

static void
os_cmd_sqline_add(struct sourceinfo *si, int parc, char *parv[])
{
	char *target = parv[0];
	char *token = strtok(parv[1], " ");
	char *treason, reason[BUFSIZE];
	long duration;
	char *s;
	struct qline *q;

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
			mowgli_strlcpy(reason, treason, BUFSIZE);
		else
			mowgli_strlcpy(reason, "No reason given", BUFSIZE);
	}
	else if (!strcasecmp(token, "!T"))
	{
		s = strtok(NULL, " ");
		treason = strtok(NULL, "");
		if (treason)
			mowgli_strlcpy(reason, treason, BUFSIZE);
		else
			mowgli_strlcpy(reason, "No reason given", BUFSIZE);
		if (s)
		{
			duration = (atol(s) * SECONDS_PER_MINUTE);
			while (isdigit((unsigned char)*s))
				s++;
			if (*s == 'h' || *s == 'H')
				duration *= MINUTES_PER_HOUR;
			else if (*s == 'd' || *s == 'D')
				duration *= MINUTES_PER_DAY;
			else if (*s == 'w' || *s == 'W')
				duration *= MINUTES_PER_WEEK;
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
		mowgli_strlcpy(reason, token, BUFSIZE);
		treason = strtok(NULL, "");

		if (treason)
		{
			mowgli_strlcat(reason, " ", BUFSIZE);
			mowgli_strlcat(reason, treason, BUFSIZE);
		}
	}

	char *p;
	int i = 0;

	if (!VALID_CHANNEL_PFX(target))
	{
		/* make sure there's at least 3 non-wildcards */
		/* except if there are no wildcards at all */
		for (p = target; *p; p++)
		{
			if (*p != '*' && *p != '?' && *p != '.')
				i++;
		}

		if (i < 3 && (strchr(target, '*') || strchr(target, '?')) && !has_priv(si, PRIV_AKILL_ANYMASK))
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

static void
os_cmd_sqline_del(struct sourceinfo *si, int parc, char *parv[])
{
	char *target = parv[0];
	struct qline *q;
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

				s++;	// skip past the :

				for (i = 0; *s != '\0'; s++, i++)
					t[i] = *s;

				t[++i] = '\0';
				end = atoi(t);

				for (i = start; i <= end; i++)
				{
					if (!(q = qline_find_num(i)))
					{
						command_fail(si, fault_nosuch_target, _("No such SQLINE with number \2%u\2."), i);
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
				command_fail(si, fault_nosuch_target, _("No such SQLINE with number \2%u\2."), number);
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

			target++;	// skip past the :

			for (i = 0; *target != '\0'; target++, i++)
				t[i] = *target;

			t[++i] = '\0';
			end = atoi(t);

			for (i = start; i <= end; i++)
			{
				if (!(q = qline_find_num(i)))
				{
					command_fail(si, fault_nosuch_target, _("No such SQLINE with number \2%u\2."), i);
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
			command_fail(si, fault_nosuch_target, _("No such SQLINE with number \2%u\2."), number);
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

static void
os_cmd_sqline_list(struct sourceinfo *si, int parc, char *parv[])
{
	char *param = parv[0];
	bool full = false;
	mowgli_node_t *n;
	struct qline *q;

	if (param != NULL && !strcasecmp(param, "FULL"))
		full = true;

	if (full)
		command_success_nodata(si, _("SQLINE list (with reasons):"));
	else
		command_success_nodata(si, _("SQLINE list:"));

	MOWGLI_ITER_FOREACH(n, qlnlist.head)
	{
		q = (struct qline *)n->data;

		if (q->duration && full)
			command_success_nodata(si, _("%u: %s - by \2%s\2 - expires in \2%s\2 - (%s)"), q->number, q->mask, q->setby, timediff(q->expires > CURRTIME ? q->expires - CURRTIME : 0), q->reason);
		else if (q->duration && !full)
			command_success_nodata(si, _("%u: %s - by \2%s\2 - expires in \2%s\2"), q->number, q->mask, q->setby, timediff(q->expires > CURRTIME ? q->expires - CURRTIME : 0));
		else if (!q->duration && full)
			command_success_nodata(si, _("%u: %s - by \2%s\2 - \2permanent\2 - (%s)"), q->number, q->mask, q->setby, q->reason);
		else
			command_success_nodata(si, _("%u: %s - by \2%s\2 - \2permanent\2"), q->number, q->mask, q->setby);
	}

	command_success_nodata(si, ngettext(N_("Total of \2%zu\2 entry in SQLINE list."),
	                                    N_("Total of \2%zu\2 entries in SQLINE list."),
	                                    qlnlist.count), qlnlist.count);

	logcommand(si, CMDLOG_GET, "SQLINE:LIST: \2%s\2", full ? " FULL" : "");
}

static void
os_cmd_sqline_sync(struct sourceinfo *si, int parc, char *parv[])
{
	mowgli_node_t *n;
	struct qline *q;

	logcommand(si, CMDLOG_DO, "SQLINE:SYNC");

	MOWGLI_ITER_FOREACH(n, qlnlist.head)
	{
		q = (struct qline *)n->data;

		if (q->duration == 0)
			qline_sts("*", q->mask, 0, q->reason);
		else if (q->expires > CURRTIME)
			qline_sts("*", q->mask, q->expires - CURRTIME, q->reason);
	}

	command_success_nodata(si, _("SQLINE list synchronized to servers."));
}

static struct command os_sqline = {
	.name           = "SQLINE",
	.desc           = N_("Manages network nickname bans."),
	.access         = PRIV_MASS_AKILL,
	.maxparc        = 3,
	.cmd            = &os_cmd_sqline,
	.help           = { .path = "oservice/sqline" },
};

static struct command os_sqline_add = {
	.name           = "ADD",
	.desc           = N_("Adds a network nickname ban."),
	.access         = AC_NONE,
	.maxparc        = 2,
	.cmd            = &os_cmd_sqline_add,
	.help           = { .path = "" },
};

static struct command os_sqline_del = {
	.name           = "DEL",
	.desc           = N_("Deletes a network nickname ban."),
	.access         = AC_NONE,
	.maxparc        = 1,
	.cmd            = &os_cmd_sqline_del,
	.help           = { .path = "" },
};

static struct command os_sqline_list = {
	.name           = "LIST",
	.desc           = N_("Lists all network nickname bans."),
	.access         = AC_NONE,
	.maxparc        = 1,
	.cmd            = &os_cmd_sqline_list,
	.help           = { .path = "" },
};

static struct command os_sqline_sync = {
	.name           = "SYNC",
	.desc           = N_("Synchronises network nickname bans to all servers."),
	.access         = AC_NONE,
	.maxparc        = 0,
	.cmd            = &os_cmd_sqline_sync,
	.help           = { .path = "" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "operserv/main")

	if (ircd && qline_sts == &generic_qline_sts)
	{
		(void) slog(LG_ERROR, "Module %s requires qline support, refusing to load.", m->name);

		m->mflags |= MODFLAG_FAIL;
		return;
	}

	if (! (os_sqline_cmds = mowgli_patricia_create(&strcasecanon)))
	{
		(void) slog(LG_ERROR, "%s: mowgli_patricia_create() failed", m->name);

		m->mflags |= MODFLAG_FAIL;
		return;
	}

	(void) command_add(&os_sqline_add, os_sqline_cmds);
	(void) command_add(&os_sqline_del, os_sqline_cmds);
	(void) command_add(&os_sqline_list, os_sqline_cmds);
	(void) command_add(&os_sqline_sync, os_sqline_cmds);

	(void) service_named_bind_command("operserv", &os_sqline);

	(void) hook_add_user_add(&os_sqline_newuser);
	(void) hook_add_user_nickchange(&os_sqline_newuser);
	(void) hook_add_channel_join(&os_sqline_chanjoin);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	(void) hook_del_user_add(&os_sqline_newuser);
	(void) hook_del_user_nickchange(&os_sqline_newuser);
	(void) hook_del_channel_join(&os_sqline_chanjoin);

	(void) service_named_unbind_command("operserv", &os_sqline);

	(void) mowgli_patricia_destroy(os_sqline_cmds, &command_delete_trie_cb, os_sqline_cmds);
}

SIMPLE_DECLARE_MODULE_V1("operserv/sqline", MODULE_UNLOAD_CAPABILITY_OK)
