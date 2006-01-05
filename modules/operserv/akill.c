/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains functionality which implements
 * the OService AKILL/KLINE command.
 *
 * $Id: akill.c 4495 2006-01-05 00:36:09Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/akill", FALSE, _modinit, _moddeinit,
	"$Id: akill.c 4495 2006-01-05 00:36:09Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void os_cmd_akill(char *origin);
static void os_cmd_akill_add(char *origin, char *target);
static void os_cmd_akill_del(char *origin, char *target);
static void os_cmd_akill_list(char *origin, char *target);


command_t os_kline = { "KLINE", "Manages network bans. [deprecated, use OS AKILL in future.]", PRIV_AKILL, os_cmd_akill };
command_t os_akill = { "AKILL", "Manages network bans.", PRIV_AKILL, os_cmd_akill };

fcommand_t os_akill_add = { "ADD", AC_NONE, os_cmd_akill_add };
fcommand_t os_akill_del = { "DEL", AC_NONE, os_cmd_akill_del };
fcommand_t os_akill_list = { "LIST", AC_NONE, os_cmd_akill_list };

list_t *os_cmdtree;
list_t *os_helptree;
list_t os_akill_cmds;

void _modinit(module_t *m)
{
	os_cmdtree = module_locate_symbol("operserv/main", "os_cmdtree");
	os_helptree = module_locate_symbol("operserv/main", "os_helptree");

	command_add(&os_kline, os_cmdtree);
        command_add(&os_akill, os_cmdtree);

    /* Add sub-commands */
	fcommand_add(&os_akill_add, &os_akill_cmds);
	fcommand_add(&os_akill_del, &os_akill_cmds);
	fcommand_add(&os_akill_list, &os_akill_cmds);
	
	help_addentry(os_helptree, "AKILL", "help/oservice/akill", NULL);
}

void _moddeinit()
{
	command_delete(&os_kline, os_cmdtree);
	command_delete(&os_akill, os_cmdtree);

	/* Delete sub-commands */
	fcommand_delete(&os_akill_add, &os_akill_cmds);
	fcommand_delete(&os_akill_del, &os_akill_cmds);
	fcommand_delete(&os_akill_list, &os_akill_cmds);
	
	help_delentry(os_helptree, "AKILL");
}

static void os_cmd_akill(char *origin)
{
	/* Grab args */
	user_t *u = user_find(origin);
	char *cmd = strtok(NULL, " ");
	char *arg = strtok(NULL, " ");
	
	/* Bad/missing arg */
	if (!cmd)
	{
		notice(opersvs.nick, origin, STR_INSUFFICIENT_PARAMS, "AKILL");
		notice(opersvs.nick, origin, "Syntax: AKILL ADD|DEL|LIST");
		return;
	}
	
	fcommand_exec(opersvs.me, arg, origin, cmd, &os_akill_cmds);
}

static void os_cmd_akill_add(char *origin, char *target)
{
	user_t *u;
	char *token = strtok(NULL, " ");
	char *treason, reason[BUFSIZE];
	long duration;
	char *s;
	kline_t *k;

	if (!target || !token)
	{
		notice(opersvs.nick, origin, STR_INSUFFICIENT_PARAMS, "AKILL ADD");
		notice(opersvs.nick, origin, "Syntax: AKILL ADD <nick|hostmask> [!P|!T <minutes>] " "<reason>");
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
			duration = (atol(s) * 60);
		else {
			notice(opersvs.nick, origin, STR_INSUFFICIENT_PARAMS, "AKILL ADD");
			notice(opersvs.nick, origin, "Syntax: AKILL ADD <nick|hostmask> [!P|!T <minutes>] " "<reason>");
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

	if (strchr(target,'!'))
	{
		notice(opersvs.nick, origin, "Invalid character '%c' in user@host.", '!');
		return;
	}

	if (!(strchr(target, '@')))
	{
		if (!(u = user_find_named(target)))
		{
			notice(opersvs.nick, origin, "\2%s\2 is not on IRC.", target);
			return;
		}

		if (is_internal_client(u))
			return;

		if ((k = kline_find(u->user, u->host)))
		{
			notice(opersvs.nick, origin, "AKILL \2%s@%s\2 is already matched in the database.", u->user, u->host);
			return;
		}

		k = kline_add(u->user, u->host, reason, duration);
		k->setby = sstrdup(origin);
	}
	else
	{
		char *userbuf = strtok(target, "@");
		char *hostbuf = strtok(NULL, "");
		char *p;
		int i = 0;

		if (!userbuf || !hostbuf)
		{
			notice(opersvs.nick, origin, STR_INSUFFICIENT_PARAMS, "AKILL ADD");
			notice(opersvs.nick, origin, "Syntax: AKILL ADD <user>@<host> [options] <reason>");
			return;
		}

		/* make sure there's at least 4 non-wildcards */
		for (p = userbuf; *p; p++)
		{
			if (*p != '*' && *p != '?' && *p != '.')
				i++;
		}
		for (p = hostbuf; *p; p++)
		{
			if (*p != '*' && *p != '?' && *p != '.')
				i++;
		}

		if (i < 4)
		{
			notice(opersvs.nick, origin, "Invalid user@host: \2%s@%s\2. At least four non-wildcard characters are required.", userbuf, hostbuf);
			return;
		}

		if ((k = kline_find(userbuf, hostbuf)))
		{
			notice(opersvs.nick, origin, "AKILL \2%s@%s\2 is already matched in the database.", userbuf, hostbuf);
			return;
		}

		k = kline_add(userbuf, hostbuf, reason, duration);
		k->setby = sstrdup(origin);
	}

	if (duration)
		notice(opersvs.nick, origin, "Timed AKILL on \2%s@%s\2 was successfully added and will " "expire in %s.", k->user, k->host, timediff(duration));
	else
		notice(opersvs.nick, origin, "AKILL on \2%s@%s\2 was successfully added.", k->user, k->host);

	snoop("AKILL:ADD: \2%s@%s\2 by \2%s\2 for \2%s\2", k->user, k->host, origin, k->reason);
	logcommand(opersvs.me, user_find(origin), CMDLOG_SET, "AKILL ADD %s@%s %s", k->user, k->host, k->reason);
}

static void os_cmd_akill_del(char *origin, char *target)
{
	char *userbuf, *hostbuf;
	uint32_t number;
	char *s;
	kline_t *k;

	if (!target)
	{
		notice(opersvs.nick, origin, STR_INSUFFICIENT_PARAMS, "AKILL DEL");
		notice(opersvs.nick, origin, "Syntax: AKILL DEL <hostmask>");
		return;
	}

	if (strchr(target, ','))
	{
		uint32_t start = 0, end = 0, i;
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
					if (!(k = kline_find_num(i)))
					{
						notice(opersvs.nick, origin, "No such AKILL with number \2%d\2.", i);
						continue;
					}

					notice(opersvs.nick, origin, "AKILL on \2%s@%s\2 has been successfully removed.", k->user, k->host);
					snoop("AKILL:DEL: \2%s@%s\2 by \2%s\2", k->user, k->host, origin);
					logcommand(opersvs.me, user_find(origin), CMDLOG_SET, "AKILL DEL %s@%s", k->user, k->host);
					kline_delete(k->user, k->host);
				}

				continue;
			}

			number = atoi(s);

			if (!(k = kline_find_num(number)))
			{
				notice(opersvs.nick, origin, "No such AKILL with number \2%d\2.", number);
				return;
			}

			notice(opersvs.nick, origin, "AKILL on \2%s@%s\2 has been successfully removed.", k->user, k->host);
			snoop("AKILL:DEL: \2%s@%s\2 by \2%s\2", k->user, k->host, origin);
			logcommand(opersvs.me, user_find(origin), CMDLOG_SET, "AKILL DEL %s@%s", k->user, k->host);
			kline_delete(k->user, k->host);
		} while ((s = strtok(NULL, ",")));

		return;
	}

	if (!strchr(target, '@'))
	{
		uint32_t start = 0, end = 0, i;
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
				if (!(k = kline_find_num(i)))
				{
					notice(opersvs.nick, origin, "No such AKILL with number \2%d\2.", i);
					continue;
				}

				notice(opersvs.nick, origin, "AKILL on \2%s@%s\2 has been successfully removed.", k->user, k->host);
				snoop("AKILL:DEL: \2%s@%s\2 by \2%s\2", k->user, k->host, origin);
				kline_delete(k->user, k->host);
			}

			return;
		}

		number = atoi(target);

		if (!(k = kline_find_num(number)))
		{
			notice(opersvs.nick, origin, "No such AKILL with number \2%d\2.", number);
			return;
		}

		notice(opersvs.nick, origin, "AKILL on \2%s@%s\2 has been successfully removed.", k->user, k->host);
		snoop("AKILL:DEL: \2%s@%s\2 by \2%s\2", k->user, k->host, origin);
		logcommand(opersvs.me, user_find(origin), CMDLOG_SET, "AKILL DEL %s@%s", k->user, k->host);
		kline_delete(k->user, k->host);
		return;
	}

	userbuf = strtok(target, "@");
	hostbuf = strtok(NULL, "@");

	if (!(k = kline_find(userbuf, hostbuf)))
	{
		notice(opersvs.nick, origin, "No such AKILL: \2%s@%s\2.", userbuf, hostbuf);
		return;
	}

	notice(opersvs.nick, origin, "AKILL on \2%s@%s\2 has been successfully removed.", userbuf, hostbuf);
	snoop("AKILL:DEL: \2%s@%s\2 by \2%s\2", k->user, k->host, origin);
	logcommand(opersvs.me, user_find(origin), CMDLOG_SET, "AKILL DEL %s@%s", k->user, k->host);
	kline_delete(userbuf, hostbuf);
}

static void os_cmd_akill_list(char *origin, char *param)
{
	boolean_t full = FALSE;
	node_t *n;
	kline_t *k;

	if (param != NULL && !strcasecmp(param, "FULL"))
		full = TRUE;
	
	if (full)
		notice(opersvs.nick, origin, "AKILL list (with reasons):");
	else
		notice(opersvs.nick, origin, "AKILL list:");

	LIST_FOREACH(n, klnlist.head)
	{
		k = (kline_t *)n->data;

		if (k->duration && full)
			notice(opersvs.nick, origin, "%d: %s@%s - by \2%s\2 - expires in \2%s\2 - (%s)", k->number, k->user, k->host, k->setby, timediff(k->expires > CURRTIME ? k->expires - CURRTIME : 0), k->reason);
		else if (k->duration && !full)
			notice(opersvs.nick, origin, "%d: %s@%s - by \2%s\2 - expires in \2%s\2", k->number, k->user, k->host, k->setby, timediff(k->expires > CURRTIME ? k->expires - CURRTIME : 0));
		else if (!k->duration && full)
			notice(opersvs.nick, origin, "%d: %s@%s - by \2%s\2 - \2permanent\2 - (%s)", k->number, k->user, k->host, k->setby, k->reason);
		else
			notice(opersvs.nick, origin, "%d: %s@%s - by \2%s\2 - \2permanent\2", k->number, k->user, k->host, k->setby);
	}

	notice(opersvs.nick, origin, "Total of \2%d\2 %s in AKILL list.", klnlist.count, (klnlist.count == 1) ? "entry" : "entries");
	logcommand(opersvs.me, user_find(origin), CMDLOG_GET, "AKILL LIST%s", full ? " FULL" : "");
}

