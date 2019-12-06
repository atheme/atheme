/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2009 Atheme Project (http://atheme.org/)
 * Copyright (C) 2009 Rizon Development Team (http://redmine.rizon.net/)
 *
 * This file contains functionality which implements
 * the OperServ SGLINE command.
 */

#include <atheme.h>

static mowgli_patricia_t *os_sgline_cmds = NULL;

static void
os_sgline_newuser(struct hook_user_nick *data)
{
	struct user *u = data->u;
	struct xline *x;

	// If the user has been killed, don't do anything.
	if (!u)
		return;

	if (is_internal_client(u))
		return;
	x = xline_find_user(u);
	if (x != NULL)
	{
		/* Server didn't have that xline, send it again.
		 * To ensure xline exempt works on sglines too, do
		 * not send a KILL. -- jilles */
		xline_sts("*", x->realname, x->duration ? x->expires - CURRTIME : 0, x->reason);
	}
}

static void
os_cmd_sgline(struct sourceinfo *const restrict si, const int parc, char **const restrict parv)
{
	if (parc < 1)
	{
		(void) command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SGLINE");
		(void) command_fail(si, fault_needmoreparams, _("Syntax: SGLINE ADD|DEL|LIST"));
		return;
	}

	(void) subcommand_dispatch_simple(si->service, si, parc, parv, os_sgline_cmds, "SGLINE");
}

static void
os_cmd_sgline_add(struct sourceinfo *si, int parc, char *parv[])
{
	char *target = parv[0];
	char *token = strtok(parv[1], " ");
	char *treason, reason[BUFSIZE];
	long duration;
	char *s;
	struct xline *x;

	if (!target || !token)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SGLINE ADD");
		command_fail(si, fault_needmoreparams, _("Syntax: SGLINE ADD <gecos> [!P|!T <minutes>] <reason>"));
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
				command_fail(si, fault_badparams, _("Syntax: SGLINE ADD <gecos> [!P|!T <minutes>] <reason>"));
				return;
			}
		}
		else {
			command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SGLINE ADD");
			command_fail(si, fault_needmoreparams, _("Syntax: SGLINE ADD <gecos> [!P|!T <minutes>] <reason>"));
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

	/* make sure there's at least 3 non-wildcards */
	/* except if there are no wildcards at all */
	for (p = target; *p; p++)
	{
		if (*p != '*' && *p != '?')
			i++;
	}

	if (i < 3 && (strchr(target, '*') || strchr(target, '?')) && !has_priv(si, PRIV_AKILL_ANYMASK))
	{
		command_fail(si, fault_badparams, _("Invalid gecos: \2%s\2. At least three non-wildcard characters are required."), target);
		return;
	}

	if (strlen(target) > (GECOSLEN + 1) * 2)
	{
		command_fail(si, fault_badparams, _("The mask provided is too long."));
		return;
	}

	if (xline_find(target))
	{
		command_fail(si, fault_nochange, _("SGLINE \2%s\2 is already matched in the database."), target);
		return;
	}

	x = xline_add(target, reason, duration, get_storage_oper_name(si));

	if (duration)
		command_success_nodata(si, _("Timed SGLINE on \2%s\2 was successfully added and will expire in %s."), x->realname, timediff(duration));
	else
		command_success_nodata(si, _("SGLINE on \2%s\2 was successfully added."), x->realname);

	verbose_wallops("\2%s\2 is \2adding\2 an \2SGLINE\2 for \2%s\2 -- reason: \2%s\2", get_oper_name(si), x->realname,
		x->reason);
	logcommand(si, CMDLOG_ADMIN, "SGLINE:ADD: \2%s\2 (reason: \2%s\2)", x->realname, x->reason);
}

static void
os_cmd_sgline_del(struct sourceinfo *si, int parc, char *parv[])
{
	char *target = parv[0];
	struct xline *x;
	unsigned int number;
	char *s;

	if (!target)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SGLINE DEL");
		command_fail(si, fault_needmoreparams, _("Syntax: SGLINE DEL <gecos>"));
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
					if (!(x = xline_find_num(i)))
					{
						command_fail(si, fault_nosuch_target, _("No such SGLINE with number \2%u\2."), i);
						continue;
					}

					command_success_nodata(si, _("SGLINE on \2%s\2 has been successfully removed."), x->realname);
					verbose_wallops("\2%s\2 is \2removing\2 an \2SGLINE\2 for \2%s\2 -- reason: \2%s\2",
						get_oper_name(si), x->realname, x->reason);

					logcommand(si, CMDLOG_ADMIN, "SGLINE:DEL: \2%s\2", x->realname);
					xline_delete(x->realname);
				}

				continue;
			}

			number = atoi(s);

			if (!(x = xline_find_num(number)))
			{
				command_fail(si, fault_nosuch_target, _("No such SGLINE with number \2%u\2."), number);
				return;
			}

			command_success_nodata(si, _("SGLINE on \2%s\2 has been successfully removed."), x->realname);
			verbose_wallops("\2%s\2 is \2removing\2 an \2SGLINE\2 for \2%s\2 -- reason: \2%s\2",
				get_oper_name(si), x->realname, x->reason);

			logcommand(si, CMDLOG_ADMIN, "SGLINE:DEL: \2%s\2", x->realname);
			xline_delete(x->realname);
		} while ((s = strtok(NULL, ",")));

		return;
	}

	if (!(x = xline_find(target)))
	{
		command_fail(si, fault_nosuch_target, _("No such SGLINE: \2%s\2."), target);
		return;
	}

	command_success_nodata(si, _("SGLINE on \2%s\2 has been successfully removed."), target);

	verbose_wallops("\2%s\2 is \2removing\2 an \2SGLINE\2 for \2%s\2 -- reason: \2%s\2",
		get_oper_name(si), x->realname, x->reason);

	logcommand(si, CMDLOG_ADMIN, "SGLINE:DEL: \2%s\2", target);
	xline_delete(target);
}

static void
os_cmd_sgline_list(struct sourceinfo *si, int parc, char *parv[])
{
	char *param = parv[0];
	bool full = false;
	mowgli_node_t *n;
	struct xline *x;

	if (param != NULL && !strcasecmp(param, "FULL"))
		full = true;

	if (full)
		command_success_nodata(si, _("SGLINE list (with reasons):"));
	else
		command_success_nodata(si, _("SGLINE list:"));

	MOWGLI_ITER_FOREACH(n, xlnlist.head)
	{
		x = (struct xline *)n->data;

		if (x->duration && full)
			command_success_nodata(si, _("%u: %s - by \2%s\2 - expires in \2%s\2 - (%s)"), x->number, x->realname, x->setby, timediff(x->expires > CURRTIME ? x->expires - CURRTIME : 0), x->reason);
		else if (x->duration && !full)
			command_success_nodata(si, _("%u: %s - by \2%s\2 - expires in \2%s\2"), x->number, x->realname, x->setby, timediff(x->expires > CURRTIME ? x->expires - CURRTIME : 0));
		else if (!x->duration && full)
			command_success_nodata(si, _("%u: %s - by \2%s\2 - \2permanent\2 - (%s)"), x->number, x->realname, x->setby, x->reason);
		else
			command_success_nodata(si, _("%u: %s - by \2%s\2 - \2permanent\2"), x->number, x->realname, x->setby);
	}

	command_success_nodata(si, ngettext(N_("Total of \2%zu\2 entry in SGLINE list."),
	                                    N_("Total of \2%zu\2 entries in SGLINE list."),
	                                    xlnlist.count), xlnlist.count);

	logcommand(si, CMDLOG_GET, "SGLINE:LIST: \2%s\2", full ? " FULL" : "");
}

static void
os_cmd_sgline_sync(struct sourceinfo *si, int parc, char *parv[])
{
	mowgli_node_t *n;
	struct xline *x;

	logcommand(si, CMDLOG_DO, "SGLINE:SYNC");

	MOWGLI_ITER_FOREACH(n, xlnlist.head)
	{
		x = (struct xline *)n->data;

		if (x->duration == 0)
			xline_sts("*", x->realname, 0, x->reason);
		else if (x->expires > CURRTIME)
			xline_sts("*", x->realname, x->expires - CURRTIME, x->reason);
	}

	command_success_nodata(si, _("SGLINE list synchronized to servers."));
}

static struct command os_sgline = {
	.name           = "SGLINE",
	.desc           = N_("Manages network realname bans."),
	.access         = PRIV_MASS_AKILL,
	.maxparc        = 3,
	.cmd            = &os_cmd_sgline,
	.help           = { .path = "oservice/sgline" },
};

static struct command os_sgline_add = {
	.name           = "ADD",
	.desc           = N_("Adds a network realname ban."),
	.access         = AC_NONE,
	.maxparc        = 2,
	.cmd            = &os_cmd_sgline_add,
	.help           = { .path = "" },
};

static struct command os_sgline_del = {
	.name           = "DEL",
	.desc           = N_("Deletes a network realname ban."),
	.access         = AC_NONE,
	.maxparc        = 1,
	.cmd            = &os_cmd_sgline_del,
	.help           = { .path = "" },
};

static struct command os_sgline_list = {
	.name           = "LIST",
	.desc           = N_("Lists all network realname bans."),
	.access         = AC_NONE,
	.maxparc        = 1,
	.cmd            = &os_cmd_sgline_list,
	.help           = { .path = "" },
};

static struct command os_sgline_sync = {
	.name           = "SYNC",
	.desc           = N_("Synchronises network realname bans to all servers."),
	.access         = AC_NONE,
	.maxparc        = 0,
	.cmd            = &os_cmd_sgline_sync,
	.help           = { .path = "" },
};

static void
mod_init(struct module *const restrict m)
{
	if (ircd && xline_sts == &generic_xline_sts)
	{
		(void) slog(LG_ERROR, "Module %s requires xline support, refusing to load.", m->name);

		m->mflags |= MODFLAG_FAIL;
		return;
	}

	MODULE_TRY_REQUEST_DEPENDENCY(m, "operserv/main")

	if (! (os_sgline_cmds = mowgli_patricia_create(&strcasecanon)))
	{
		(void) slog(LG_ERROR, "%s: mowgli_patricia_create() failed", m->name);

		m->mflags |= MODFLAG_FAIL;
		return;
	}

	(void) command_add(&os_sgline_add, os_sgline_cmds);
	(void) command_add(&os_sgline_del, os_sgline_cmds);
	(void) command_add(&os_sgline_list, os_sgline_cmds);
	(void) command_add(&os_sgline_sync, os_sgline_cmds);

	(void) service_named_bind_command("operserv", &os_sgline);

	(void) hook_add_user_add(&os_sgline_newuser);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	(void) hook_del_user_add(&os_sgline_newuser);

	(void) service_named_unbind_command("operserv", &os_sgline);

	(void) mowgli_patricia_destroy(os_sgline_cmds, &command_delete_trie_cb, os_sgline_cmds);
}

SIMPLE_DECLARE_MODULE_V1("operserv/sgline", MODULE_UNLOAD_CAPABILITY_OK)
