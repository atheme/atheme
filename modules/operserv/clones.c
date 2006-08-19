#include "atheme.h"
#define C_ITER 0x80000000

DECLARE_MODULE_V1
(
	"operserv/clones", FALSE, _modinit, _moddeinit,
	"$Id: clones.c 6145 2006-08-19 18:46:23Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static int32_t hosthash_countip(char *);
static void hook_newuser(void *);
static void hook_userquit(void *);

static void os_cmd_clones(char *);
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
list_t hostlist[HASHSIZE];

typedef struct cexcept_t_ cexcept_t;
struct cexcept_t_
{
	char *ip;
	int clones;
	char *reason;
};

command_t os_clones = { "CLONES", "Manages network wide clones.", PRIV_AKILL, os_cmd_clones };

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

	fcommand_add(&os_clones_list, &os_clones_cmds);
	fcommand_add(&os_clones_addexempt, &os_clones_cmds);
	fcommand_add(&os_clones_delexempt, &os_clones_cmds);
	fcommand_add(&os_clones_listexempt, &os_clones_cmds);

	help_addentry(os_helptree, "CLONES", "help/oservice/clones", NULL);

	hook_add_event("user_add");
	hook_add_hook("user_add", hook_newuser);
	hook_add_event("user_delete");
	hook_add_hook("user_delete", hook_userquit);

	load_exemptdb();

	/* add everyone to host hash */
	for (i = 0; i < HASHSIZE; i++)
	{
		LIST_FOREACH_SAFE(n, tn, userlist[i].head)
		{
			hook_newuser(n->data);
		}
	}
}

void _moddeinit(void)
{
	int i = 0;
	node_t *n, *tn;

	for (i = 0; i < HASHSIZE; i++)
	{
		LIST_FOREACH_SAFE(n, tn, hostlist[i].head)
		{
			node_del(n, &hostlist[i]);
			node_free(n);
		}
	}

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

	fcommand_delete(&os_clones_list, &os_clones_cmds);
	fcommand_delete(&os_clones_addexempt, &os_clones_cmds);
	fcommand_delete(&os_clones_delexempt, &os_clones_cmds);
	fcommand_delete(&os_clones_listexempt, &os_clones_cmds);

	help_delentry(os_helptree, "CLONES");

	hook_del_hook("user_add", hook_newuser);
	hook_del_hook("user_delete", hook_userquit);
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

static void os_cmd_clones_list(char *origin, char *channel)
{
	node_t *n, *tn;
	int32_t i, k = 0, l = 0;

	for (i = 0; i < HASHSIZE; i++)
	{
		LIST_FOREACH(n, hostlist[i].head)
		{
			user_t *tu = (user_t *) n->data;

			if (*tu->ip != '\0' && !(tu->flags & C_ITER))
			{
				k = hosthash_countip(tu->ip);

				if (k > 3)
				{
					if (!(tu->flags & C_ITER))
						if (is_exempt(tu->ip))
							notice(opersvs.nick, origin, "%d from %s (\2EXEMPT\2)", k, tu->ip);
						else
							notice(opersvs.nick, origin, "%d from %s", k, tu->ip);
				}

				/* already viewed this user! */
				tu->flags |= C_ITER;
			}
		}
	}

	/* cleanup */
	for (i = 0; i < HASHSIZE; i++)
	{
		LIST_FOREACH(n, hostlist[i].head)
		{
			user_t *tu = (user_t *) n->data;

			tu->flags &= ~C_ITER;
		}
	}

	notice(opersvs.nick, origin, "End of CLONES LIST");
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
			notice(opersvs.nick, origin, "Removed \2%s\2 from clone exempt list.", arg);
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
}

static int32_t hosthash_countip(char *ip)
{
	node_t *n;
	int32_t count = 0;

	LIST_FOREACH(n, hostlist[shash((unsigned char *) ip)].head)
	{
		user_t *u = (user_t *)n->data;

		if (!strcmp(u->ip, ip))
			count++;
	}

	return count;
}

static void hook_newuser(void *vptr)
{
	int i;
	user_t *u = vptr;

	if (*u->ip != '\0')
	{
		node_add(u, node_create(), &hostlist[UHASH((unsigned char *)u->ip)]);
	}

	if (*u->ip != '\0' && !is_exempt(u->ip))
	{
		i = hosthash_countip(u->ip);

		if (i > 3 && i < 6)
		{
			snoop("CLONES: %d clones on %s", i, u->ip);
		}
		else if (i >= 6)
		{
			snoop("CLONES: %d clones on %s (TKLINE due to excess clones)", i, u->ip);
			kline_add("*", u->host, "Excessive clones", 3600);
		}
	}
}

static void hook_userquit(void *vptr)
{
	user_t *u = vptr;
	node_t *n;

	n = node_find(u, &hostlist[UHASH((unsigned char *)u->ip)]);

	if (n)
	{
		node_del(n, &hostlist[UHASH((unsigned char *)u->ip)]);
		node_free(n);
	}
}

