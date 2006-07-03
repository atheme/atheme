/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains functionality which implements
 * the OperServ AKILL command.
 *
 * $Id: akill.c 5686 2006-07-03 16:25:03Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/akill", FALSE, _modinit, _moddeinit,
	"$Id: akill.c 5686 2006-07-03 16:25:03Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void os_akill_newuser(void *vptr);

static void os_cmd_akill(char *origin);
static void os_cmd_akill_add(char *origin, char *target);
static void os_cmd_akill_del(char *origin, char *target);
static void os_cmd_akill_list(char *origin, char *target);
static void os_cmd_akill_sync(char *origin, char *target);


command_t os_akill = { "AKILL", "Manages network bans.", PRIV_AKILL, os_cmd_akill };

fcommand_t os_akill_add = { "ADD", AC_NONE, os_cmd_akill_add };
fcommand_t os_akill_del = { "DEL", AC_NONE, os_cmd_akill_del };
fcommand_t os_akill_list = { "LIST", AC_NONE, os_cmd_akill_list };
fcommand_t os_akill_sync = { "SYNC", AC_NONE, os_cmd_akill_sync };

list_t *os_cmdtree;
list_t *os_helptree;
list_t os_akill_cmds;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(os_cmdtree, "operserv/main", "os_cmdtree");
	MODULE_USE_SYMBOL(os_helptree, "operserv/main", "os_helptree");

        command_add(&os_akill, os_cmdtree);

	/* Add sub-commands */
	fcommand_add(&os_akill_add, &os_akill_cmds);
	fcommand_add(&os_akill_del, &os_akill_cmds);
	fcommand_add(&os_akill_list, &os_akill_cmds);
	fcommand_add(&os_akill_sync, &os_akill_cmds);
	
	help_addentry(os_helptree, "AKILL", "help/oservice/akill", NULL);

	hook_add_event("user_add");
	hook_add_hook("user_add", os_akill_newuser);
}

void _moddeinit()
{
	command_delete(&os_akill, os_cmdtree);

	/* Delete sub-commands */
	fcommand_delete(&os_akill_add, &os_akill_cmds);
	fcommand_delete(&os_akill_del, &os_akill_cmds);
	fcommand_delete(&os_akill_list, &os_akill_cmds);
	fcommand_delete(&os_akill_sync, &os_akill_cmds);
	
	help_delentry(os_helptree, "AKILL");

	hook_del_hook("user_add", os_akill_newuser);
}

static void os_akill_newuser(void *vptr)
{
	user_t *u;
	kline_t *k;

	u = vptr;
	if (is_internal_client(u))
		return;
	k = kline_find_user(u);
	if (k != NULL)
	{
		/* Server didn't have that kline, send it again.
		 * To ensure kline exempt works on akills too, do
		 * not send a KILL. -- jilles */
		kline_sts(u->server->name, k->user, k->host, k->duration ? k->expires - CURRTIME : 0, k->reason);
	}
}

static void os_cmd_akill(char *origin)
{
	/* Grab args */
	user_t *u = user_find_named(origin);
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
				notice(opersvs.nick, origin, "Invalid duration given.");
				notice(opersvs.nick, origin, "Syntax: AKILL ADD <nick|hostmask> [!P|!T <minutes>] " "<reason>");
				return;
			}
		}
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

		if ((k = kline_find("*", u->host)))
		{
			notice(opersvs.nick, origin, "AKILL \2*@%s\2 is already matched in the database.", u->host);
			return;
		}

		k = kline_add("*", u->host, reason, duration);
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
		notice(opersvs.nick, origin, "Timed AKILL on \2%s@%s\2 was successfully added and will expire in %s.", k->user, k->host, timediff(duration));
	else
		notice(opersvs.nick, origin, "AKILL on \2%s@%s\2 was successfully added.", k->user, k->host);

	snoop("AKILL:ADD: \2%s@%s\2 by \2%s\2 for \2%s\2", k->user, k->host, origin, k->reason);

	verbose_wallops("\2%s\2 is \2adding\2 an \2AKILL\2 for \2%s@%s\2 -- reason: \2%s\2", origin, k->user, k->host, 
		k->reason);
	logcommand(opersvs.me, user_find_named(origin), CMDLOG_SET, "AKILL ADD %s@%s %s", k->user, k->host, k->reason);
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
					verbose_wallops("\2%s\2 is \2removing\2 an \2AKILL\2 for \2%s@%s\2 -- reason: \2%s\2",
						origin, k->user, k->host, k->reason);

					snoop("AKILL:DEL: \2%s@%s\2 by \2%s\2", k->user, k->host, origin);
					logcommand(opersvs.me, user_find_named(origin), CMDLOG_SET, "AKILL DEL %s@%s", k->user, k->host);
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
			verbose_wallops("\2%s\2 is \2removing\2 an \2AKILL\2 for \2%s@%s\2 -- reason: \2%s\2",
				origin, k->user, k->host, k->reason);

			snoop("AKILL:DEL: \2%s@%s\2 by \2%s\2", k->user, k->host, origin);
			logcommand(opersvs.me, user_find_named(origin), CMDLOG_SET, "AKILL DEL %s@%s", k->user, k->host);
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
				verbose_wallops("\2%s\2 is \2removing\2 an \2AKILL\2 for \2%s@%s\2 -- reason: \2%s\2",
					origin, k->user, k->host, k->reason);

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

		verbose_wallops("\2%s\2 is \2removing\2 an \2AKILL\2 for \2%s@%s\2 -- reason: \2%s\2",
			origin, k->user, k->host, k->reason);

		snoop("AKILL:DEL: \2%s@%s\2 by \2%s\2", k->user, k->host, origin);
		logcommand(opersvs.me, user_find_named(origin), CMDLOG_SET, "AKILL DEL %s@%s", k->user, k->host);
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

	verbose_wallops("\2%s\2 is \2removing\2 an \2AKILL\2 for \2%s@%s\2 -- reason: \2%s\2",
		origin, k->user, k->host, k->reason);

	snoop("AKILL:DEL: \2%s@%s\2 by \2%s\2", k->user, k->host, origin);
	logcommand(opersvs.me, user_find_named(origin), CMDLOG_SET, "AKILL DEL %s@%s", k->user, k->host);
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
	logcommand(opersvs.me, user_find_named(origin), CMDLOG_GET, "AKILL LIST%s", full ? " FULL" : "");
}

static void os_cmd_akill_sync(char *origin, char *param)
{
	node_t *n;
	kline_t *k;

	logcommand(opersvs.me, user_find_named(origin), CMDLOG_DO, "AKILL SYNC");
	snoop("AKILL:SYNC: \2%s\2", origin);

	LIST_FOREACH(n, klnlist.head)
	{
		k = (kline_t *)n->data;

		if (k->duration == 0)
			kline_sts("*", k->user, k->host, 0, k->reason);
		else if (k->expires > CURRTIME)
			kline_sts("*", k->user, k->host, k->expires - CURRTIME, k->reason);
	}

	notice(opersvs.nick, origin, "AKILL list synchronized to servers.");
}
