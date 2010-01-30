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

list_t *os_cmdtree;
list_t *os_helptree;

static void os_cmd_trace(sourceinfo_t *si, int parc, char *parv[]);

command_t os_trace = { "TRACE", N_("Looks for users and performs actions on them."), PRIV_USER_AUSPEX, 2, os_cmd_trace };

typedef struct {
	void /* trace_query_domain_t */ *(*prepare)(char **args);
	bool (*exec)(user_t *u, void /* trace_query_domain_t */ *q);
	void (*cleanup)(void /* trace_query_domain_t */ *q);
} trace_query_constructor_t;

typedef struct {
	trace_query_constructor_t *cons;
	node_t node;
} trace_query_domain_t;

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
	server = *args;
	while (**args && **args != ' ')
		*args++;
	**args = '\0';

	domain = scalloc(sizeof(trace_query_server_domain_t), 1);
	domain->server = server_find(server);

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
	char *server;
	trace_query_channel_domain_t *domain;

	return_val_if_fail(args != NULL, NULL);
	return_val_if_fail(*args != NULL, NULL);

	/* split out the next space */
	server = *args;
	while (**args && **args != ' ')
		*args++;
	**args = '\0';

	domain = scalloc(sizeof(trace_query_channel_domain_t), 1);
	domain->channel = channel_find(server);

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

/****************************************************************************************************/

typedef struct {
	sourceinfo_t *si;
} trace_action_t;

typedef struct {
	trace_action_t *(*prepare)(sourceinfo_t *si, char **args);
	void (*exec)(user_t *u, trace_action_t *a);
	void (*cleanup)(trace_action_t *a);
} trace_action_constructor_t;

/* initializes common fields for trace action object. */
void trace_action_init(trace_action_t *a, sourceinfo_t *si)
{
	return_if_fail(a != NULL);
	return_if_fail(si != NULL);

	a->si = si;
}

static trace_action_t *trace_print_prepare(sourceinfo_t *si, char **args)
{
	trace_action_t *a;

	return_val_if_fail(si != NULL, NULL);
	return_val_if_fail(args != NULL, NULL);
	return_val_if_fail(*args != NULL, NULL);

	a = scalloc(sizeof(trace_action_t), 1);
	trace_action_init(a, si);

	return a;
}

static void trace_print_exec(user_t *u, trace_action_t *a)
{
	return_if_fail(u != NULL);
	return_if_fail(a != NULL);

	command_success_nodata(a->si, _("\2Match:\2  %s!%s@%s %s {%s}"), u->nick, u->user, u->host, u->gecos, u->server->name);
}

static void trace_print_cleanup(trace_action_t *a)
{
	return_if_fail(a != NULL);

	free(a);
}

trace_action_constructor_t trace_print = { trace_print_prepare, trace_print_exec, trace_print_cleanup };

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
	MODULE_USE_SYMBOL(os_cmdtree, "operserv/main", "os_cmdtree");
	MODULE_USE_SYMBOL(os_helptree, "operserv/main", "os_helptree");

	command_add(&os_trace, os_cmdtree);
	help_addentry(os_helptree, "TRACE", "help/oservice/trace", NULL);

	trace_cmdtree = mowgli_patricia_create(strcasecanon);
	mowgli_patricia_add(trace_cmdtree, "REGEXP", &trace_regexp);
	mowgli_patricia_add(trace_cmdtree, "SERVER", &trace_server);
	mowgli_patricia_add(trace_cmdtree, "CHANNEL", &trace_channel);

	trace_acttree = mowgli_patricia_create(strcasecanon);
	mowgli_patricia_add(trace_cmdtree, "PRINT", &trace_print);
}

void _moddeinit(void)
{
	mowgli_patricia_destroy(trace_cmdtree, NULL, NULL);

	command_delete(&os_trace, os_cmdtree);
	help_delentry(os_helptree, "TRACE");
}

#define MAXMATCHES_DEF 1000

static void os_cmd_trace(sourceinfo_t *si, int parc, char *parv[])
{
	atheme_regex_t *regex;
	char usermask[512];
	unsigned int matches = 0, maxmatches;
	mowgli_patricia_iteration_state_t state;
	user_t *u;
	char *args = parv[0];
	char *pattern;
	int flags = 0;

	if (args == NULL)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "trace");
		command_fail(si, fault_needmoreparams, _("Syntax: trace /<regex>/[i] [FORCE]"));
		return;
	}

	pattern = regex_extract(args, &args, &flags);
	if (pattern == NULL)
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "trace");
		command_fail(si, fault_badparams, _("Syntax: trace /<regex>/[i] [FORCE]"));
		return;
	}

	while (*args == ' ')
		args++;

	if (!strcasecmp(args, "FORCE"))
		maxmatches = UINT_MAX;
	else if (*args == '\0')
		maxmatches = MAXMATCHES_DEF;
	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "trace");
		command_fail(si, fault_badparams, _("Syntax: trace /<regex>/[i] [FORCE]"));
		return;
	}

	regex = regex_create(pattern, flags);
	
	if (regex == NULL)
	{
		command_fail(si, fault_badparams, _("The provided regex \2%s\2 is invalid."), pattern);
		return;
	}
		
	MOWGLI_PATRICIA_FOREACH(u, &state, userlist)
	{
		sprintf(usermask, "%s!%s@%s %s", u->nick, u->user, u->host, u->gecos);

		if (regex_match(regex, usermask))
		{
			matches++;
			if (matches <= maxmatches)
				command_success_nodata(si, _("\2Match:\2  %s!%s@%s %s"), u->nick, u->user, u->host, u->gecos);
			else if (matches == maxmatches + 1)
			{
				command_success_nodata(si, _("Too many matches, not displaying any more"));
				command_success_nodata(si, _("Add the FORCE keyword to see them all"));
			}
		}
	}
	
	regex_destroy(regex);
	command_success_nodata(si, _("\2%d\2 matches for %s"), matches, pattern);
	logcommand(si, CMDLOG_ADMIN, "trace: \2%s\2 (\2%d\2 matches)", pattern, matches);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
