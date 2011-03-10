/*
 * Copyright (c) 2011 William Pitcock <nenolod@dereferenced.org>
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Does an A record lookup.
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
        "contrib/os_resolve", false, _modinit, _moddeinit,
        PACKAGE_STRING,
        "Atheme Development Group <http://www.atheme.org>"
);

static void os_cmd_resolve(sourceinfo_t *si, int parc, char *parv[]);

command_t cmd_resolve = { "RESOLVE", N_("Perform DNS lookup on hostname"), PRIV_ADMIN, 1, os_cmd_resolve, { .path = "contrib/os_resolve" } };

void _modinit(module_t *m)
{
	service_named_bind_command("operserv", &cmd_resolve);
}

void _moddeinit(module_unload_intent_t intent)
{
	service_named_unbind_command("operserv", &cmd_resolve);
}

static void resolve_cb(void *vptr, dns_reply_t *reply)
{
	service_t *svs;
	user_t *u = vptr;
	char buf[BUFSIZE];

	return_if_fail(vptr != NULL);
	return_if_fail(reply != NULL);

	if (reply->addr.saddr.sa.sa_family != AF_INET)
		return;

	inet_ntop(reply->addr.saddr.sa.sa_family, &reply->addr.saddr.sin.sin_addr, buf, reply->addr.saddr_len);

	svs = service_find("operserv");
	notice(svs->nick, u->nick, "Result is %s", buf);
}

dns_query_t dns_query;

static void os_cmd_resolve(sourceinfo_t *si, int parc, char *parv[])
{
	if (!parv[0])
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "RESOLVE");
		return;
	}

	memset(&dns_query, '\0', sizeof(dns_query_t));

	dns_query.ptr = si->su;
	dns_query.callback = resolve_cb;

	gethost_byname_type(parv[0], &dns_query, T_A);
}
