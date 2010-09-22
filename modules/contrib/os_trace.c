/*
 * Copyright (c) 2010 William Pitcock <nenolod@atheme.org>
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Looks for users and performs actions on them.
 */

#include "atheme.h"
#include <limits.h>

DECLARE_MODULE_V1
(
	"operserv/trace", false, _modinit, _moddeinit,
	"Copyright (c) 2010 William Pitcock <nenolod@atheme.org>",
	"Atheme Development Group <http://www.atheme.org>"
);

static char *reason_extract(char **args);
static void os_cmd_trace(sourceinfo_t *si, int parc, char *parv[]);

command_t os_trace = { "TRACE", N_("Looks for users and performs actions on them."), PRIV_USER_AUSPEX, 2, os_cmd_trace, { .path = "help/contrib/trace" } };

typedef struct {
	void /* trace_query_domain_t */ *(*prepare)(char **args);
	bool (*exec)(user_t *u, void /* trace_query_domain_t */ *q);
	void (*cleanup)(void /* trace_query_domain_t */ *q);
} trace_query_constructor_t;

typedef struct {
	trace_query_constructor_t *cons;
	node_t node;
} trace_query_domain_t;

static int read_comparison_operator(char** string, int default_comparison)
{
	if (**string == '<')
	{
		if (*((*string)+1) == '=')
		{
			*string += 2;
			return 2;
		}
		else
		{
			*string += 1;
			return 1;
		}
	}
	else if (**string == '>')
	{
		if (*((*string)+1) == '=')
		{
			*string += 2;
			return 4;
		}
		else
		{
			*string += 1;
			return 3;
		}
	}
	else if (**string == '=')
	{
		*string += 1;
		return 0;
	}
	else
		return default_comparison;
}

char *reason_extract(char **args)
{
	char *start = *args;
	bool quotes = false;

	while (**args == ' ')
	{
		(*args)++;
	}
	if (**args == '"')
	{
		start = ++(*args);
		quotes = true;
	}
	else
		start = *args;

	while (**args)
	{
		if (quotes && **args == '"')
		{
			quotes = false;
			**args = 0;
			(*args)++;
			break;
		}
		else if (!quotes && **args == ' ')
		{
			**args = 0;
			(*args)++;
			break;
		}
		
		(*args)++;
	}

	if (!(**args))
		*args = NULL;

	if (start == *args)
		return NULL;  /* No reason found. */
	if (quotes)
		return NULL;  /* Unclosed quotes. */

	return start;
}

typedef struct {
	trace_query_domain_t domain;
	atheme_regex_t *regex;
	char *pattern;
	int flags;
} trace_query_regexp_domain_t;

static void *trace_regexp_prepare(char **args)
{
	trace_query_regexp_domain_t *domain;

	return_val_if_fail(args != NULL, NULL);
	return_val_if_fail(*args != NULL, NULL);

	domain = scalloc(sizeof(trace_query_regexp_domain_t), 1);
	domain->pattern = regex_extract(*args, &(*args), &domain->flags);
	domain->regex = regex_create(domain->pattern, domain->flags);
	
	return domain;
}

static bool trace_regexp_exec(user_t *u, void *q)
{
	char usermask[512];
	trace_query_regexp_domain_t *domain = (trace_query_regexp_domain_t *) q;

	return_val_if_fail(domain != NULL, false);
	return_val_if_fail(u != NULL, false);

	if (domain->regex == NULL)
		return false;

	snprintf(usermask, 512, "%s!%s@%s %s", u->nick, u->user, u->host, u->gecos);

	return regex_match(domain->regex, usermask);
}

static void trace_regexp_cleanup(void *q)
{
	trace_query_regexp_domain_t *domain = (trace_query_regexp_domain_t *) q;

	return_if_fail(domain != NULL);

	if (domain->regex != NULL)
		regex_destroy(domain->regex);

	free(domain);
}

trace_query_constructor_t trace_regexp = { trace_regexp_prepare, trace_regexp_exec, trace_regexp_cleanup };

typedef struct {
	trace_query_domain_t domain;
	server_t *server;
} trace_query_server_domain_t;

static void *trace_server_prepare(char **args)
{
	char *server;
	trace_query_server_domain_t *domain;

	return_val_if_fail(args != NULL, NULL);
	return_val_if_fail(*args != NULL, NULL);

	/* split out the next space */
	server = strtok(*args, " ");

	domain = scalloc(sizeof(trace_query_server_domain_t), 1);
	domain->server = server_find(server);

	/* advance *args to next token */
	*args = strtok(NULL, "");

	return domain;
}

static bool trace_server_exec(user_t *u, void *q)
{
	trace_query_server_domain_t *domain = (trace_query_server_domain_t *) q;

	return_val_if_fail(domain != NULL, false);
	return_val_if_fail(u != NULL, false);

	return (domain->server == u->server);
}

static void trace_server_cleanup(void *q)
{
	trace_query_server_domain_t *domain = (trace_query_server_domain_t *) q;

	return_if_fail(domain != NULL);

	free(domain);
}

trace_query_constructor_t trace_server = { trace_server_prepare, trace_server_exec, trace_server_cleanup };

typedef struct {
	trace_query_domain_t domain;
	channel_t *channel;
} trace_query_channel_domain_t;

static void *trace_channel_prepare(char **args)
{
	char *channel;
	trace_query_channel_domain_t *domain;

	return_val_if_fail(args != NULL, NULL);
	return_val_if_fail(*args != NULL, NULL);

	/* split out the next space */
	channel = strtok(*args, " ");

	domain = scalloc(sizeof(trace_query_channel_domain_t), 1);
	domain->channel = channel_find(channel);

	/* advance *args to next token */
	*args = strtok(NULL, "");

	return domain;
}

static bool trace_channel_exec(user_t *u, void *q)
{
	trace_query_channel_domain_t *domain = (trace_query_channel_domain_t *) q;

	return_val_if_fail(domain != NULL, false);
	return_val_if_fail(u != NULL, false);

	return (chanuser_find(domain->channel, u) != NULL);
}

static void trace_channel_cleanup(void *q)
{
	trace_query_channel_domain_t *domain = (trace_query_channel_domain_t *) q;

	return_if_fail(domain != NULL);

	free(domain);
}

trace_query_constructor_t trace_channel = { trace_channel_prepare, trace_channel_exec, trace_channel_cleanup };

typedef struct {
	trace_query_domain_t domain;
	int nickage;
	int comparison;
} trace_query_nickage_domain_t;

static void *trace_nickage_prepare(char **args)
{
	char *nickage_string;
	trace_query_nickage_domain_t *domain;

	return_val_if_fail(args != NULL, NULL);
	return_val_if_fail(*args != NULL, NULL);

	/* split out the next space */
	nickage_string = strtok(*args, " ");

	domain = scalloc(sizeof(trace_query_nickage_domain_t), 1);
	domain->comparison = read_comparison_operator(&nickage_string, 2);
	domain->nickage = atoi(nickage_string);

	/* advance *args to next token */
	*args = strtok(NULL, "");

	return domain;
}

static bool trace_nickage_exec(user_t *u, void *q)
{
	trace_query_nickage_domain_t *domain = (trace_query_nickage_domain_t *) q;

	return_val_if_fail(domain != NULL, false);
	return_val_if_fail(u != NULL, false);

	int nickage = CURRTIME - u->ts;
	if (domain->comparison == 1)
		return (nickage < domain->nickage);
	else if (domain->comparison == 2)
		return (nickage <= domain->nickage);
	else if (domain->comparison == 3)
		return (nickage > domain->nickage);
	else if (domain->comparison == 4)
		return (nickage >= domain->nickage);
	else
		return (nickage == domain->nickage);
}

static void trace_nickage_cleanup(void *q)
{
	trace_query_nickage_domain_t *domain = (trace_query_nickage_domain_t *) q;

	return_if_fail(domain != NULL);

	free(domain);
}

trace_query_constructor_t trace_nickage = { trace_nickage_prepare, trace_nickage_exec, trace_nickage_cleanup };

typedef struct {
	trace_query_domain_t domain;
	int numchan;
	int comparison;
} trace_query_numchan_domain_t;

static void *trace_numchan_prepare(char **args)
{
	char *numchan_string;
	trace_query_numchan_domain_t *domain;

	return_val_if_fail(args != NULL, NULL);
	return_val_if_fail(*args != NULL, NULL);

	/* split out the next space */
	numchan_string = strtok(*args, " ");

	domain = scalloc(sizeof(trace_query_numchan_domain_t), 1);
	domain->comparison = read_comparison_operator(&numchan_string, 0);
	domain->numchan = atoi(numchan_string);

	/* advance *args to next token */
	*args = strtok(NULL, "");

	return domain;
}

static bool trace_numchan_exec(user_t *u, void *q)
{
	trace_query_numchan_domain_t *domain = (trace_query_numchan_domain_t *) q;

	return_val_if_fail(domain != NULL, false);
	return_val_if_fail(u != NULL, false);

	int numchan = u->channels.count;
	if (domain->comparison == 1)
		return (numchan < domain->numchan);
	else if (domain->comparison == 2)
		return (numchan <= domain->numchan);
	else if (domain->comparison == 3)
		return (numchan > domain->numchan);
	else if (domain->comparison == 4)
		return (numchan >= domain->numchan);
	else
		return (numchan == domain->numchan);
}

static void trace_numchan_cleanup(void *q)
{
	trace_query_numchan_domain_t *domain = (trace_query_numchan_domain_t *) q;

	return_if_fail(domain != NULL);

	free(domain);
}

trace_query_constructor_t trace_numchan = { trace_numchan_prepare, trace_numchan_exec, trace_numchan_cleanup };

typedef struct {
	trace_query_domain_t domain;
	bool identified;
} trace_query_identified_domain_t;

static void *trace_identified_prepare(char **args)
{
	char *yesno;
	bool identified;
	trace_query_identified_domain_t *domain;

	return_val_if_fail(args != NULL, NULL);
	return_val_if_fail(*args != NULL, NULL);

	yesno = strtok(*args, " ");

	if (!strcasecmp(yesno, "yes"))
		identified = true;
	else if (!strcasecmp(yesno, "no"))
		identified = false;
	else
		return NULL;

	domain = scalloc(sizeof(trace_query_identified_domain_t), 1);
	domain->identified = identified;

	/* advance *args to next token */
	*args = strtok(NULL, "");

	return domain;
}

static bool trace_identified_exec(user_t *u, void *q)
{
	trace_query_identified_domain_t *domain = (trace_query_identified_domain_t *) q;

	return_val_if_fail(domain != NULL, false);
	return_val_if_fail(u != NULL, false);

	return (domain->identified == (u->myuser != NULL));
}

static void trace_identified_cleanup(void *q)
{
	trace_query_identified_domain_t *domain = (trace_query_identified_domain_t *) q;

	return_if_fail(domain != NULL);

	free(domain);
}

trace_query_constructor_t trace_identified = { trace_identified_prepare, trace_identified_exec, trace_identified_cleanup };

/****************************************************************************************************/

typedef struct {
	sourceinfo_t *si;
	bool matched;
} trace_action_t;

typedef struct {
	trace_action_t *(*prepare)(sourceinfo_t *si, char **args);
	void (*exec)(user_t *u, trace_action_t *a);
	void (*cleanup)(trace_action_t *a, bool succeeded);
} trace_action_constructor_t;

static void trace_action_init(trace_action_t *a, sourceinfo_t *si);

/* initializes common fields for trace action object. */
void trace_action_init(trace_action_t *a, sourceinfo_t *si)
{
	return_if_fail(a != NULL);
	return_if_fail(si != NULL);

	a->si = si;
	a->matched = false;
}

static trace_action_t *trace_print_prepare(sourceinfo_t *si, char **args)
{
	trace_action_t *a;

	return_val_if_fail(si != NULL, NULL);

	a = scalloc(sizeof(trace_action_t), 1);
	trace_action_init(a, si);

	return a;
}

static void trace_print_exec(user_t *u, trace_action_t *a)
{
	return_if_fail(u != NULL);
	return_if_fail(a != NULL);
	if(is_internal_client(u))
		return;

	a->matched = true;
	command_success_nodata(a->si, _("\2Match:\2  %s!%s@%s %s {%s}"), u->nick, u->user, u->host, u->gecos, u->server->name);
}

static void trace_print_cleanup(trace_action_t *a, bool succeeded)
{
	return_if_fail(a != NULL);

	if (!a->matched && succeeded)
		command_success_nodata(a->si, _("No matches."));
	free(a);
}

trace_action_constructor_t trace_print = { trace_print_prepare, trace_print_exec, trace_print_cleanup };

typedef struct {
	trace_action_t base;
	char *reason;
} trace_action_kill_t;

static trace_action_t *trace_kill_prepare(sourceinfo_t *si, char **args)
{
	trace_action_kill_t *a;
	char *reason;

	return_val_if_fail(si != NULL, NULL);
	return_val_if_fail(args != NULL, NULL);
	if (*args == NULL)
		return NULL;

	reason = reason_extract(args);
	if (reason == NULL)
		return NULL;

	a = scalloc(sizeof(trace_action_kill_t), 1);
	trace_action_init(&a->base, si);
	a->reason = reason;

	return (trace_action_t*) a;
}

static void trace_kill_exec(user_t *u, trace_action_t *act)
{
	service_t *svs;
	trace_action_kill_t *a = (trace_action_kill_t *) act;

	return_if_fail(u != NULL);
	return_if_fail(a != NULL);
	if (is_internal_client(u))
		return;
	if (is_ircop(u))
		return;
	if (u->myuser && is_soper(u->myuser))
		return;
	if ((svs = service_find("operserv")) != NULL)
		return;

	act->matched = true;
	kill_user(svs->me, u, "%s", a->reason);
	command_success_nodata(act->si, _("\2%s\2 has been killed."), u->nick);
}

static void trace_kill_cleanup(trace_action_t *a, bool succeeded)
{
	return_if_fail(a != NULL);

	if (!a->matched && succeeded)
		command_success_nodata(a->si, _("No matches."));
	free(a);
}

trace_action_constructor_t trace_kill = { trace_kill_prepare, trace_kill_exec, trace_kill_cleanup };

typedef struct {
	trace_action_t base;
	long duration;
	char *reason;
} trace_action_akill_t;

static trace_action_t *trace_akill_prepare(sourceinfo_t *si, char **args)
{
	trace_action_akill_t *a;
	char *s, *reason;
	long duration = config_options.kline_time;
	char token;

	return_val_if_fail(si != NULL, NULL);
	return_val_if_fail(args != NULL, NULL);
	if (*args == NULL)
		return NULL;

	while (**args == ' ')
		(*args)++;

	/* Extract a token, but only if there's one to remove.
	 * Otherwise, this would clip a word off the reason. */
	token = 0;
	s = *args; 
	if (!strncasecmp(s, "!T", 2) || !strncasecmp(s, "!P", 2))
	{
		if (s[2] == ' ')
		{
			token = tolower(s[1]);
			s[2] = '\0';
			*args += 3;
		}
		else if (s[2] == '\0')
		{
			token = tolower(s[1]);
			*args += 2;
		}
	}

	if (token == 't')
	{
		s = strtok(*args, " ");
		*args = strtok(NULL, "");
		if (*args == NULL)
			return NULL;

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
			return NULL;
	}
	else if (token == 'p')
		duration = 0;
	
	reason = reason_extract(args);
	if (reason == NULL)
		return NULL;
	
	a = scalloc(sizeof(trace_action_akill_t), 1);
	trace_action_init(&a->base, si);
	a->duration = duration;
	a->reason = reason;


	return (trace_action_t*) a;
}

static void trace_akill_exec(user_t *u, trace_action_t *act)
{
	char *kuser, *khost;
	trace_action_akill_t *a = (trace_action_akill_t *) act;

	return_if_fail(u != NULL);
	return_if_fail(a != NULL);
	if (is_internal_client(u))
		return;
	if (is_ircop(u))
		return;
	if (u->myuser && is_soper(u->myuser))
		return;

	kuser = "*";
	khost = u->host;

	if (!match(khost, "127.0.0.1") || !match_ips(khost, "127.0.0.1"))
		return;
	if (me.vhost != NULL && (!match(khost, me.vhost) || !match_ips(khost, me.vhost)))
		return;
	if (kline_find(kuser, khost))
		return;

	act->matched = true;
	kline_add(kuser, khost, a->reason, a->duration, get_storage_oper_name(act->si));
	command_success_nodata(act->si, _("\2%s\2 has been akilled."), u->nick);
}

static void trace_akill_cleanup(trace_action_t *a, bool succeeded)
{
	return_if_fail(a != NULL);

	if (!a->matched && succeeded)
		command_success_nodata(a->si, _("No matches."));
	free(a);
}

trace_action_constructor_t trace_akill = { trace_akill_prepare, trace_akill_exec, trace_akill_cleanup };

typedef struct {
	trace_action_t base;
	int matches;
} trace_action_count_t;

static trace_action_t *trace_count_prepare(sourceinfo_t *si, char **args)
{
	trace_action_count_t *a;

	return_val_if_fail(si != NULL, NULL);

	a = scalloc(sizeof(trace_action_count_t), 1);
	trace_action_init(&a->base, si);

	return (trace_action_t *) a;
}

static void trace_count_exec(user_t *u, trace_action_t *act)
{
	trace_action_count_t *a = (trace_action_count_t *) act;

	return_if_fail(u != NULL);
	return_if_fail(a != NULL);
	if (is_internal_client(u))
		return;

	act->matched = true;
	a->matches++;
}

static void trace_count_cleanup(trace_action_t *act, bool succeeded)
{
	trace_action_count_t *a = (trace_action_count_t *) act;

	return_if_fail(a != NULL);

	if (succeeded)
		command_success_nodata(act->si, _("\2%d\2 matches"), a->matches);

	free(a);
}

trace_action_constructor_t trace_count = { trace_count_prepare, trace_count_exec, trace_count_cleanup };

/*
 * Add-on interface.
 *
 * This allows third-party module writers to extend the trace API.
 * Just copy the prototypes out of trace.c, and add the trace_cmdtree
 * symbol to your module with MODULE_USE_SYMBOL().
 *
 * Then add your criteria to the tree with mowgli_patricia_add().
 */
mowgli_patricia_t *trace_cmdtree = NULL;
mowgli_patricia_t *trace_acttree = NULL;

void _modinit(module_t *m)
{
	service_named_bind_command("operserv", &os_trace);

	trace_cmdtree = mowgli_patricia_create(strcasecanon);
	mowgli_patricia_add(trace_cmdtree, "REGEXP", &trace_regexp);
	mowgli_patricia_add(trace_cmdtree, "SERVER", &trace_server);
	mowgli_patricia_add(trace_cmdtree, "CHANNEL", &trace_channel);
	mowgli_patricia_add(trace_cmdtree, "NICKAGE", &trace_nickage);
	mowgli_patricia_add(trace_cmdtree, "NUMCHAN", &trace_numchan);
	mowgli_patricia_add(trace_cmdtree, "IDENTIFIED", &trace_identified);

	trace_acttree = mowgli_patricia_create(strcasecanon);
	mowgli_patricia_add(trace_acttree, "PRINT", &trace_print);
	mowgli_patricia_add(trace_acttree, "KILL", &trace_kill);
	mowgli_patricia_add(trace_acttree, "AKILL", &trace_akill);
	mowgli_patricia_add(trace_acttree, "COUNT", &trace_count);
}

void _moddeinit(void)
{
	mowgli_patricia_destroy(trace_cmdtree, NULL, NULL);

	service_named_unbind_command("operserv", &os_trace);
}

#define MAXMATCHES_DEF 1000

static bool os_cmd_trace_run(sourceinfo_t *si, trace_action_constructor_t *actcons, trace_action_t* act, list_t *crit, char *args);

static void os_cmd_trace(sourceinfo_t *si, int parc, char *parv[])
{
	list_t crit = { NULL, NULL, 0 };
	trace_action_constructor_t *actcons;
	trace_action_t* act;
	char *args = parv[1];
	node_t *n, *tn;
	char *params;
	bool succeeded;

	actcons = mowgli_patricia_retrieve(trace_acttree, parv[0]);
	if (actcons == NULL)
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "TRACE");
		command_fail(si, fault_badparams, _("Syntax: TRACE <action> <params>"));
		return;
	}

	act = actcons->prepare(si, &args);
	if (act == NULL)
	{
		command_fail(si, fault_nosuch_target, _("Action compilation failed."));
		return;
	}

	params = sstrdup(args);
	succeeded = os_cmd_trace_run(si, actcons, act, &crit, args);

	LIST_FOREACH_SAFE(n, tn, crit.head)
	{
		trace_query_domain_t *q = (trace_query_domain_t *) n->data;
		q->cons->cleanup(q);		
	}
	actcons->cleanup(act, succeeded);

	if (succeeded)
		logcommand(si, CMDLOG_ADMIN, "TRACE: \2%s\2 \2%s\2", parv[0], params);

	free(params);
}

static bool os_cmd_trace_run(sourceinfo_t *si, trace_action_constructor_t *actcons, trace_action_t* act, list_t *crit, char *args)
{
	user_t *u;
	mowgli_patricia_iteration_state_t state;
	node_t *n;

	if (args == NULL)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "TRACE");
		command_fail(si, fault_needmoreparams, _("Syntax: TRACE <action> <params>"));
		return false;
	}

	while (true)
	{
		trace_query_constructor_t *cons;
		trace_query_domain_t *q;
		char *cmd = strtok(args, " ");

		if (cmd == NULL)
			break;

		cons = mowgli_patricia_retrieve(trace_cmdtree, cmd);
		if (cons == NULL)
		{
			command_fail(si, fault_nosuch_target, _("Invalid criteria specified."));
			return false;
		}

		args = strtok(NULL, "");
		if (args == NULL)
		{
			command_fail(si, fault_nosuch_target, _("Invalid criteria specified."));
			return false;
		}

		slog(LG_DEBUG, "operserv/trace: adding criteria %p(%s) to list [remain: %s]", q, cmd, args);
		q = cons->prepare(&args);
		if (q == NULL)
		{
			command_fail(si, fault_nosuch_target, _("Invalid criteria specified."));
			return false;
		}
		slog(LG_DEBUG, "operserv/trace: new args position [%s]", args);

		q->cons = cons;
		node_add(q, &q->node, crit);
	}

	MOWGLI_PATRICIA_FOREACH(u, &state, userlist)
	{
		bool doit = true;

		LIST_FOREACH(n, crit->head)
		{
			trace_query_domain_t *q = (trace_query_domain_t *) n->data;

			if (!q->cons->exec(u, q))
			{
				doit = false;
				break;
			}
		}

		if (doit)
			actcons->exec(u, act);
	}

	return true;
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
