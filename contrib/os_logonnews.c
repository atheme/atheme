/*
 * Copyright (c) 2005-2006 William Pitcock <nenolod -at- nenolod.net>
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Logon News stuff...
 *
 * $Id: os_logonnews.c 7785 2007-03-03 15:54:32Z pippijn $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/logonnews", FALSE, _modinit, _moddeinit,
	"$Revision: 7785 $",
	"William Pitcock <nenolod -at- nenolod.net>"
);

list_t *os_cmdtree;

static void os_cmd_logonnews(sourceinfo_t *si, int parc, char *parv[]);
static void write_newsdb(void);
static void load_newsdb(void);
static void display_news(void *vptr);

command_t os_logonnews = { "LOGONNEWS", "Manages logon news.",
		  	   PRIV_GLOBAL, 3, os_cmd_logonnews };

struct logonnews_ {
	char *nick;
	char *target;
	time_t news_ts;
	char *story;
};

typedef struct logonnews_ logonnews_t;

list_t logon_news;

void _modinit(module_t *m)
{
	os_cmdtree = module_locate_symbol("operserv/main", "os_cmdtree");
	hook_add_event("user_add");
	hook_add_hook("user_add", display_news);
	command_add(&os_logonnews, os_cmdtree);
	load_newsdb();
}

void _moddeinit(void)
{
	hook_del_hook("user_add", display_news);
	command_delete(&os_logonnews, os_cmdtree);
}

static void write_newsdb(void)
{
	FILE *f;
	node_t *n;
	logonnews_t *l;

	if (!(f = fopen(DATADIR "/news.db.new", "w")))
	{
		slog(LG_DEBUG, "write_newsdb(): cannot write news database: %s", strerror(errno));
		return;
	}

	LIST_FOREACH(n, logon_news.head)
	{
		l = n->data;
		fprintf(f, "GL %s %s %ld %s\n", l->nick, l->target,
			l->news_ts, l->story);
	}

	fclose(f);

	if ((rename(DATADIR "/news.db.new", DATADIR "/news.db")) < 0)
	{
		slog(LG_DEBUG, "write_newsdb(): couldn't rename news database.");
		return;
	}
}

static void load_newsdb(void)
{
	FILE *f;
	node_t *n;
	logonnews_t *l;
	char *item, rBuf[BUFSIZE];

	if (!(f = fopen(DATADIR "/news.db", "r")))
	{
		slog(LG_DEBUG, "read_newsdb(): cannot open news database: %s", strerror(errno));
		return;
	}

	while (fgets(rBuf, BUFSIZE, f))
	{
		item = strtok(rBuf, " ");
		strip(item);

		if (!strcmp(item, "GL"))
		{
			char *nick = strtok(NULL, " ");
			char *target = strtok(NULL, " ");
			char *news_ts = strtok(NULL, " ");
			char *story = strtok(NULL, "");

			if (!nick || !target || !news_ts || !story)
				continue;

			l = smalloc(sizeof(logonnews_t));
			l->nick = sstrdup(nick);
			l->target = sstrdup(target);
			l->news_ts = atol(news_ts);
			l->story = sstrdup(story);

			n = node_create();
			node_add(l, n, &logon_news); 
		}
	}

	fclose(f);
}

static void display_news(void *vptr)
{
	user_t *u = vptr;
	node_t *n;
	logonnews_t *l;
	char dBuf[BUFSIZE];
	struct tm tm;
	int count = 0;

	/* abort if it's an internal client */
	if (is_internal_client(u))
		return;

	if (logon_news.count > 0)
	{
		notice(globsvs.nick, u->nick, "*** \2Message(s) of the Day\2 ***");

		LIST_FOREACH_PREV(n, logon_news.tail)
		{
			l = n->data;
			tm = *localtime(&l->news_ts);
			strftime(dBuf, BUFSIZE, "%H:%M on %m/%d/%y", &tm);
			notice(globsvs.nick, u->nick, "[\2%s\2] Notice from %s, posted %s:",
				l->target, l->nick, dBuf);
			notice(globsvs.nick, u->nick, "%s", l->story);
			count++;

			/* only display three latest entries, max. */
			if (count == 3)
				break;
		}

		notice(globsvs.nick, u->nick, "*** \2End of Message(s) of the Day\2 ***");
	}
}

static void os_cmd_logonnews(sourceinfo_t *si, int parc, char *parv[])
{
	char *action = parv[0];
	char *target = parv[1];
	char *story = parv[2];
	logonnews_t *l;
	node_t *n;

	if (!action)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "LOGONNEWS");
		command_fail(si, fault_needmoreparams, "Syntax: LOGONNEWS ADD|DEL|LIST [target] [message]");
		return;
	}

	if (!strcasecmp(action, "ADD"))
	{
		if (!target || !story)
		{
			command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "LOGONNEWS ADD");
			command_fail(si, fault_needmoreparams, "Syntax: LOGONNEWS ADD <target> <message>");
			return;
		}

		l = smalloc(sizeof(logonnews_t));
		l->nick = sstrdup(get_source_name(si));
		l->news_ts = CURRTIME;
		l->story = sstrdup(story);
		l->target = sstrdup(target);

		n = node_create();
		node_add(l, n, &logon_news);

		write_newsdb();

		command_success_nodata(si, "Added entry to logon news.");
		logcommand(si, CMDLOG_ADMIN, "LOGONNEWS ADD %s %s", target, story);
		return;
	}
	else if (!strcasecmp(action, "DEL"))
	{
		int x = 0;
		int id;

		if (!target)
		{
			command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "LOGONNEWS DEL");
			command_fail(si, fault_needmoreparams, "Syntax: LOGONNEWS DEL <id>");
			return;
		}

		id = atoi(target);

		if (id <= 0)
		{
			command_fail(si, fault_badparams, STR_INVALID_PARAMS, "LOGONNEWS DEL");
			command_fail(si, fault_badparams, "Syntax: LOGONNEWS DEL <id>");
			return;
		}

		/* search for it */
		LIST_FOREACH(n, logon_news.head)
		{
			l = n->data;
			x++;

			if (x == id)
			{
				logcommand(si, CMDLOG_ADMIN, "LOGONNEWS DEL %s %s", l->target, l->story);

				node_del(n, &logon_news);

				free(l->nick);
				free(l->target);
				free(l->story);
				free(l);

				write_newsdb();

				command_success_nodata(si, "Deleted entry %d from logon news.", id);
				return;
			}
		}

		command_fail(si, fault_nosuch_target, "Entry %d not found in logon news.", id);
		return;
	}
	else if (!strcasecmp(action, "LIST"))
	{
		int x = 0;

		LIST_FOREACH(n, logon_news.head)
		{
			l = n->data;
			x++;

			command_success_nodata(si, "%d: [\2%s\2] by \2%s\2, \2%s\2", 
				x, l->target, l->nick, l->story);
		}

		command_success_nodata(si, "End of list.");
		logcommand(si, CMDLOG_GET, "LOGONNEWS LIST");
		return;
	}
	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "LOGONNEWS");
		command_fail(si, fault_needmoreparams, "Syntax: LOGONNEWS ADD|DEL|LIST [target] [message]");
		return;
	}
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
