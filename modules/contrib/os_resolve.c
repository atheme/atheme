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

typedef struct {
	dns_query_t dns_query;
	sourceinfo_t *si;
} resolve_req_t;

mowgli_heap_t *request_heap = NULL;

static void resolve_cb(void *vptr, dns_reply_t *reply)
{
	resolve_req_t *req = vptr;
	char buf[BUFSIZE];

	return_if_fail(vptr != NULL);
	return_if_fail(reply != NULL);

	if (reply->addr.saddr.sa.sa_family != AF_INET)
		return;

	inet_ntop(reply->addr.saddr.sa.sa_family, &reply->addr.saddr.sin.sin_addr, buf, reply->addr.saddr_len);

	command_success_nodata(req->si, "Result is %s", buf);

	mowgli_heap_free(request_heap, req);
	object_unref(req->si);
}

static void os_cmd_resolve(sourceinfo_t *si, int parc, char *parv[])
{
	resolve_req_t *req;

	if (request_heap == NULL)
		request_heap = mowgli_heap_create(sizeof(resolve_req_t), 32, BH_LAZY);

	if (!parv[0])
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "RESOLVE");
		return;
	}

	req = mowgli_heap_alloc(request_heap);

	req->si = si;
	req->dns_query.ptr = req;
	req->dns_query.callback = resolve_cb;

	gethost_byname_type(parv[0], &req->dns_query, T_A);

	object_ref(req->si);
}
