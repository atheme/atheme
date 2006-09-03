/*
 * Copyright (c) 2006 Atheme Development Group
 * Rights to this code are documented in doc/LICENCE.
 *
 * This file contains functionality implementing clone detection.
 *
 * $Id: clones.c 6285 2006-09-03 23:03:38Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/clones", FALSE, _modinit, _moddeinit,
	"$Id: clones.c 6285 2006-09-03 23:03:38Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void clones_newuser(void *);
static void clones_userquit(void *);

static void os_cmd_clones(char *);
static void os_cmd_clones_kline(char *, char *);
static void os_cmd_clones_list(char *, char *);
static void os_cmd_clones_addexempt(char *, char *);
static void os_cmd_clones_delexempt(char *, char *);
static void os_cmd_clones_listexempt(char *, char *);

static void write_exemptdb(void);
static void load_exemptdb(void);

list_t *os_cmdtree;
list_t *os_helptree;
list_t os_clones_cmds;

static list_t clone_exempts;
boolean_t kline_enabled;
dictionary_tree_t *hostlist;
BlockHeap *hostentry_heap;

typedef struct cexcept_ cexcept_t;
struct cexcept_
{
	char *ip;
	int clones;
	char *reason;
};

typedef struct hostentry_ hostentry_t;
struct hostentry_
{
	char ip[HOSTIPLEN];
	list_t clients;
};

command_t os_clones = { "CLONES", "Manages network wide clones.", PRIV_AKILL, os_cmd_clones };

fcommand_t os_clones_kline = { "KLINE", AC_NONE, os_cmd_clones_kline };
fcommand_t os_clones_list = { "LIST", AC_NONE, os_cmd_clones_list };
fcommand_t os_clones_addexempt = { "ADDEXEMPT", AC_NONE, os_cmd_clones_addexempt };
fcommand_t os_clones_delexempt = { "DELEXEMPT", AC_NONE, os_cmd_clones_delexempt };
fcommand_t os_clones_listexempt = { "LISTEXEMPT", AC_NONE, os_cmd_clones_listexempt };

void _modinit(module_t *m)
{
	int i;
	node_t *n, *tn;

	MODULE_USE_SYMBOL(os_cmdtree, "operserv/main", "os_cmdtree");
	MODULE_USE_SYMBOL(os_helptree, "operserv/main", "os_helptree");

	command_add(&os_clones, os_cmdtree);

	fcommand_add(&os_clones_kline, &os_clones_cmds);
	fcommand_add(&os_clones_list, &os_clones_cmds);
	fcommand_add(&os_clones_addexempt, &os_clones_cmds);
	fcommand_add(&os_clones_delexempt, &os_clones_cmds);
	fcommand_add(&os_clones_listexempt, &os_clones_cmds);

	help_addentry(os_helptree, "CLONES", "help/oservice/clones", NULL);

	hook_add_event("user_add");
	hook_add_hook("user_add", clones_newuser);
	hook_add_event("user_delete");
	hook_add_hook("user_delete", clones_userquit);

	hostlist = dictionary_create("clones hostlist", HASH_USER / 2, strcmp);
	hostentry_heap = BlockHeapCreate(sizeof(hostentry_t), HEAP_USER);

	load_exemptdb();

	/* add everyone to host hash */
	for (i = 0; i < HASHSIZE; i++)
	{
		LIST_FOREACH_SAFE(n, tn, userlist[i].head)
		{
			clones_newuser(n->data);
		}
	}
}

static void free_hostentry(dictionary_elem_t *delem, void *privdata)
{
	node_t *n, *tn;
	hostentry_t *he = delem->node.data;

	LIST_FOREACH_SAFE(n, tn, he->clients.head)
	{
		node_del(n, &he->clients);
		node_free(n);
	}
	BlockHeapFree(hostentry_heap, he);
}

void _moddeinit(void)
{
	int i = 0;
	node_t *n, *tn;

	dictionary_destroy(hostlist, free_hostentry, NULL);
	BlockHeapDestroy(hostentry_heap);

	LIST_FOREACH_SAFE(n, tn, clone_exempts.head)
	{
		cexcept_t *c = n->data;

		free(c->ip);
		free(c->reason);
		free(c);

		node_del(n, &clone_exempts);
		node_free(n);
	}

	command_delete(&os_clones, os_cmdtree);

	fcommand_delete(&os_clones_kline, &os_clones_cmds);
	fcommand_delete(&os_clones_list, &os_clones_cmds);
	fcommand_delete(&os_clones_addexempt, &os_clones_cmds);
	fcommand_delete(&os_clones_delexempt, &os_clones_cmds);
	fcommand_delete(&os_clones_listexempt, &os_clones_cmds);

	help_delentry(os_helptree, "CLONES");

	hook_del_hook("user_add", clones_newuser);
	hook_del_hook("user_delete", clones_userquit);
}

static void write_exemptdb(void)
{
	FILE *f;
	node_t *n;
	cexcept_t *c;

	if (!(f = fopen("etc/exempts.db.new", "w")))
	{
		slog(LG_ERROR, "write_exemptdb(): cannot write exempt database: %s", strerror(errno));
		return;
	}

	fprintf(f, "CK %d\n", kline_enabled ? 1 : 0);
	LIST_FOREACH(n, clone_exempts.head)
	{
		c = n->data;
		fprintf(f, "EX %s %d %s\n", c->ip, c->clones, c->reason);
	}

	fclose(f);

	if ((rename("etc/exempts.db.new", "etc/exempts.db")) < 0)
	{
		slog(LG_ERROR, "write_exemptdb(): couldn't rename exempts database.");
		return;
	}
}

static void load_exemptdb(void)
{
	FILE *f;
	node_t *n;
	char *item, rBuf[BUFSIZE * 2];

	if (!(f = fopen("etc/exempts.db", "r")))
	{
		slog(LG_DEBUG, "load_exemptdb(): cannot open exempt database: %s", strerror(errno));
		return;
	}

	while (fgets(rBuf, BUFSIZE * 2, f))
	{
		item = strtok(rBuf, " ");
		strip(item);

		if (!strcmp(item, "EX"))
		{
			char *ip = strtok(NULL, " ");
			int clones = atoi(strtok(NULL, " "));
			char *reason = strtok(NULL, "");

			if (!ip || clones < 6 || !reason)
				; /* erroneous, don't add */
			else
			{
				cexcept_t *c = (cexcept_t *)malloc(sizeof(cexcept_t));

				c->ip = sstrdup(ip);
				c->clones = clones;
				c->reason = sstrdup(reason);
				node_add(c, node_create(), &clone_exempts);
			}
		}
		else if (!strcmp(item, "CK"))
		{
			char *enable = strtok(NULL, " ");

			if (enable != NULL)
				kline_enabled = atoi(enable) != 0;
		}
	}

	fclose(f);
}

static int is_exempt(const char *ip)
{
	node_t *n;

	LIST_FOREACH(n, clone_exempts.head)
	{
		cexcept_t *c = n->data;

		if (!strcmp(ip, c->ip))
			return 1;
	}

	return 0;
}

static void os_cmd_clones(char *origin)
{
	/* Grab args */
	user_t *u = user_find_named(origin);
	char *cmd = strtok(NULL, " ");
	
	/* Bad/missing arg */
	if (!cmd)
	{
		notice(opersvs.nick, origin, STR_INSUFFICIENT_PARAMS, "CLONES");
		notice(opersvs.nick, origin, "Syntax: CLONES LIST|ADDEXEMPT|DELEXEMPT|LISTEXEMPT");
		return;
	}
	
	fcommand_exec(opersvs.me, "", origin, cmd, &os_clones_cmds);
}

static void os_cmd_clones_kline(char *origin, char *channel)
{
	const char *arg = strtok(NULL, " ");

	if (arg == NULL)
		arg = "";

	if (!strcasecmp(arg, "ON"))
	{
		if (kline_enabled)
		{
			notice(opersvs.nick, origin, "CLONES klines are already enabled.");
			return;
		}
		kline_enabled = TRUE;
		wallops("\2%s\2 enabled CLONES klines", origin);
		snoop("CLONES:KLINE:ON: \2%s\2", origin);
		logcommand(opersvs.me, user_find_named(origin), CMDLOG_ADMIN, "CLONES KLINE ON");
		write_exemptdb();
	}
	else if (!strcasecmp(arg, "OFF"))
	{
		if (!kline_enabled)
		{
			notice(opersvs.nick, origin, "CLONES klines are already disabled.");
			return;
		}
		kline_enabled = FALSE;
		wallops("\2%s\2 disabled CLONES klines", origin);
		snoop("CLONES:KLINE:OFF: \2%s\2", origin);
		logcommand(opersvs.me, user_find_named(origin), CMDLOG_ADMIN, "CLONES KLINE OFF");
		write_exemptdb();
	}
	else
	{
		if (kline_enabled)
			notice(opersvs.nick, origin, "CLONES klines are currently enabled.");
		else
			notice(opersvs.nick, origin, "CLONES klines are currently disabled.");
	}
}

static void os_cmd_clones_list(char *origin, char *channel)
{
	hostentry_t *he;
	int32_t k = 0;
	dictionary_iteration_state_t state;

	DICTIONARY_FOREACH(he, &state, hostlist)
	{
		k = LIST_LENGTH(&he->clients);

		if (k > 3)
		{
			if (is_exempt(he->ip))
				notice(opersvs.nick, origin, "%d from %s (\2EXEMPT\2)", k, he->ip);
			else
				notice(opersvs.nick, origin, "%d from %s", k, he->ip);
		}
	}
	notice(opersvs.nick, origin, "End of CLONES LIST");
	logcommand(opersvs.me, user_find_named(origin), CMDLOG_ADMIN, "CLONES LIST");
}

static void os_cmd_clones_addexempt(char *origin, char *channel)
{
	node_t *n;
	char *ip = strtok(NULL, " ");
	char *clonesstr = strtok(NULL, " ");
	int clones;
	char *reason = strtok(NULL, "");
	cexcept_t *c;

	if (!ip || !clonesstr || !reason)
	{
		notice(opersvs.nick, origin, STR_INSUFFICIENT_PARAMS, "CLONES ADDEXEMPT");
		notice(opersvs.nick, origin, "Syntax: CLONES ADDEXEMPT <ip> <clones> <reason>");
		return;
	}

	clones = atoi(clonesstr);
	if (clones < 6)
	{
		notice(opersvs.nick, origin, "Allowed clones count must be at least %d", 6);
		return;
	}

	LIST_FOREACH(n, clone_exempts.head)
	{
		cexcept_t *t = n->data;

		if (!strcmp(ip, t->ip))
		{
			notice(opersvs.nick, origin, "\2%s\2 already found in exempt list; not adding.", ip);
			return;
		}
	}

	c = malloc(sizeof(cexcept_t));

	c->ip = sstrdup(ip);
	c->clones = clones;
	c->reason = sstrdup(reason);

	node_add(c, node_create(), &clone_exempts);
	notice(opersvs.nick, origin, "Added \2%s\2 to clone exempt list.", ip);
	logcommand(opersvs.me, user_find_named(origin), CMDLOG_ADMIN, "CLONES ADDEXEMPT %s", ip);
	write_exemptdb();
}

static void os_cmd_clones_delexempt(char *origin, char *channel)
{
	node_t *n, *tn;
	char *arg = strtok(NULL, " ");

	if (!arg)
		return;

	LIST_FOREACH_SAFE(n, tn, clone_exempts.head)
	{
		cexcept_t *c = n->data;

		if (!strcmp(c->ip, arg))
		{
			free(c->ip);
			free(c->reason);
			free(c);
			node_del(n, &clone_exempts);
			node_free(n);
			notice(opersvs.nick, origin, "Removed \2%s\2 from clone exempt list.", arg);
			logcommand(opersvs.me, user_find_named(origin), CMDLOG_ADMIN, "CLONES DELEXEMPT %s", arg);
			write_exemptdb();
			return;
		}
	}

	notice(opersvs.nick, origin, "\2%s\2 not found in clone exempt list.", arg);
}

static void os_cmd_clones_listexempt(char *origin, char *channel)
{
	node_t *n;

	LIST_FOREACH(n, clone_exempts.head)
	{
		cexcept_t *c = n->data;

		notice(opersvs.nick, origin, "%s (%d, %s)", c->ip, c->clones, c->reason);
	}
	notice(opersvs.nick, origin, "End of CLONES LISTEXEMPT");
	logcommand(opersvs.me, user_find_named(origin), CMDLOG_ADMIN, "CLONES LISTEXEMPT");
}

static void clones_newuser(void *vptr)
{
	int i;
	user_t *u = vptr;
	hostentry_t *he;
	node_t *n;
	kline_t *k;

	/* User has no IP, ignore him */
	if (is_internal_client(u) || *u->ip == '\0')
		return;

	he = dictionary_retrieve(hostlist, u->ip);
	if (he == NULL)
	{
		he = BlockHeapAlloc(hostentry_heap);
		strlcpy(he->ip, u->ip, sizeof he->ip);
		dictionary_add(hostlist, he->ip, he);
	}
	node_add(u, node_create(), &he->clients);
	i = LIST_LENGTH(&he->clients);

	if (i > 3 && !is_exempt(u->ip))
	{
		if (i < 6)
			snoop("CLONES: %d clones on %s", i, u->ip);
		else if (!kline_enabled)
			snoop("CLONES: %d clones on %s (TKLINE disabled)", i, u->ip);
		else
		{
			snoop("CLONES: %d clones on %s (TKLINE due to excess clones)", i, u->ip);
			slog(LG_INFO, "clones_newuser(): klining *@%s (user %s!%s@%s)",
					u->ip, u->nick, u->user, u->host);
			kline_sts("*", "*", u->ip, 3600, "Excessive clones");
		}
	}
}

static void clones_userquit(void *vptr)
{
	user_t *u = vptr;
	node_t *n;
	hostentry_t *he;

	/* User has no IP, ignore him */
	if (is_internal_client(u) || *u->ip == '\0')
		return;

	he = dictionary_retrieve(hostlist, u->ip);
	if (he == NULL)
	{
		slog(LG_DEBUG, "clones_userquit(): hostentry for %s not found??", u->ip);
		return;
	}
	n = node_find(u, &he->clients);
	if (n)
	{
		node_del(n, &he->clients);
		node_free(n);
		if (LIST_LENGTH(&he->clients) == 0)
		{
			dictionary_delete(hostlist, he->ip);
			BlockHeapFree(hostentry_heap, he);
		}
	}
}
