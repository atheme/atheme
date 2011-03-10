/*
 * charybdis: A slightly useful ircd.
 * blacklist.c: Manages DNS blacklist entries and lookups
 *
 * Copyright (C) 2006-2008 charybdis development team
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "atheme.h"
#include "conf.h"

DECLARE_MODULE_V1
(
	"contrib/dnsbl", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

mowgli_list_t blacklist_list = { NULL, NULL, 0 };
mowgli_patricia_t **os_set_cmdtree;
char *action;

static void unref_blacklist(struct Blacklist *blptr);
static void os_cmd_set_dnsblaction(sourceinfo_t *si, int parc, char *parv[]);

command_t os_set_dnsblaction = { "DNSBLACTION", N_("Changes what happens to a user when they hit a DNSBL."), PRIV_USER_ADMIN, 1, os_cmd_set_dnsblaction, { .path = "contrib/set_dnsblaction" } };

static inline mowgli_list_t *dnsbl_queries(user_t *u)
{
        mowgli_list_t *l;

        return_val_if_fail(u != NULL, NULL);

        l = privatedata_get(u, "dnsbl:queries");
        if (l != NULL)
                return l;

        l = mowgli_list_create();
        privatedata_set(u, "dnsbl:queries", l);

        return l;
}

static inline struct Blacklist *listed_in_dnsbl(user_t *u)
{
        struct Blacklist *blptr;

        return_val_if_fail(u != NULL, NULL);

        blptr = privatedata_get(u, "dnsbl:listed");
        if (blptr != NULL)
	{
		privatedata_set(u, "dnsbl:listed", blptr);
		return blptr;
	}

        return blptr;
}

static void os_cmd_set_dnsblaction(sourceinfo_t *si, int parc, char *parv[])
{
	char *act = parv[0];

	if (!act)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "DNSBLACTION");
		command_fail(si, fault_needmoreparams, _("Syntax: SET DNSBLACTION <action>"));
		return;
	}

	if (!strcasecmp("SNOOP", act) || !strcasecmp("KLINE", act) || !strcasecmp("NOTIFY", act))
	{
		action = sstrdup(act);
		command_success_nodata(si, _("DNSBLACTION successfully set to \2%s\2"), act);
		logcommand(si, CMDLOG_ADMIN, "SET:DNSBLACTION: \2%s\2", act);
		return;
	}
	else if (!strcasecmp("NONE", act))
	{
		action = NULL;
		command_success_nodata(si, _("DNSBLACTION successfully set to \2%s\2"), act);
		logcommand(si, CMDLOG_ADMIN, "SET:DNSBLACTION: \2%s\2", act);
		return;
	}
	else
	{
		command_fail(si, fault_badparams, _("Invalid action given."));
		return;
	}
}

/* private interfaces */
static struct Blacklist *find_blacklist(char *name)
{
	mowgli_node_t *n;

	MOWGLI_ITER_FOREACH(n, blacklist_list.head)
	{
		struct Blacklist *blptr = (struct Blacklist *) n->data;

		if (!strcasecmp(blptr->host, name))
			return blptr;
	}

	return NULL;
}

static void blacklist_dns_callback(void *vptr, dns_reply_t *reply)
{
	struct BlacklistClient *blcptr = (struct BlacklistClient *) vptr;
	int listed = 0;
	mowgli_list_t *l;

	if (blcptr == NULL)
		return;

	if (blcptr->u == NULL)
	{
		free(blcptr);
		return;
	}

	if (reply != NULL)
	{
		/* only accept 127.x.y.z as a listing */
		if (reply->addr.saddr.sa.sa_family == AF_INET &&
				!memcmp(&((struct sockaddr_in *)&reply->addr)->sin_addr, "\177", 1))
			listed++;
		else if (blcptr->blacklist->lastwarning + 3600 < CURRTIME)
		{
			slog(LG_DEBUG,
				"Garbage reply from blacklist %s",
				blcptr->blacklist->host);
			blcptr->blacklist->lastwarning = CURRTIME;
		}
	}

	struct Blacklist *dnsbl_listed = listed_in_dnsbl(blcptr->u);

	/* they have a blacklist entry for this client */
	if (listed && dnsbl_listed == NULL)
	{
		dnsbl_listed = blcptr->blacklist;
		privatedata_set(blcptr->u, "dnsbl:listed", blcptr->blacklist);
		/* reference to blacklist moves from blcptr to u */
	}
	else
		unref_blacklist(blcptr->blacklist);


	l = dnsbl_queries(blcptr->u);
	mowgli_node_delete(&blcptr->node, l);

	free(blcptr);
}

/* XXX: no IPv6 implementation, not to concerned right now though. */
static void initiate_blacklist_dnsquery(struct Blacklist *blptr, user_t *u)
{
	struct BlacklistClient *blcptr = malloc(sizeof(struct BlacklistClient));
	char buf[IRCD_RES_HOSTLEN + 1];
	int ip[4];
	mowgli_list_t *l;

	blcptr->blacklist = blptr;
	blcptr->u = u;

	blcptr->dns_query.ptr = blcptr;
	blcptr->dns_query.callback = blacklist_dns_callback;

	/* A sscanf worked fine for chary for many years, it'll be fine here */
	sscanf(u->ip, "%d.%d.%d.%d", &ip[3], &ip[2], &ip[1], &ip[0]); 

	/* becomes 2.0.0.127.torbl.ahbl.org or whatever */
	snprintf(buf, sizeof buf, "%d.%d.%d.%d.%s", ip[0], ip[1], ip[2], ip[3], blptr->host);

	gethost_byname_type(buf, &blcptr->dns_query, T_A);

	l = dnsbl_queries(u);
	mowgli_node_add(blcptr, &blcptr->node, l);
	blptr->refcount++;
}

/* public interfaces */
static struct Blacklist *new_blacklist(char *name)
{
	struct Blacklist *blptr;

	if (name == NULL)
		return NULL;

	blptr = find_blacklist(name);
	
	if (blptr == NULL)
	{
		blptr = malloc(sizeof(struct Blacklist));
		mowgli_node_add(blptr, mowgli_node_create(), &blacklist_list);
	}

	strlcpy(blptr->host, name, IRCD_RES_HOSTLEN + 1);
	blptr->lastwarning = 0;

	return blptr;
}

static void unref_blacklist(struct Blacklist *blptr)
{
	mowgli_node_t *n;

	blptr->refcount--;
	if (blptr->refcount <= 0)
	{
		n = mowgli_node_find(blptr, &blacklist_list);
		mowgli_node_delete(n, &blacklist_list);

		free(blptr);
	}
}

static void lookup_blacklists(user_t *u)
{
	mowgli_node_t *n;

	MOWGLI_ITER_FOREACH(n, blacklist_list.head)
	{
		struct Blacklist *blptr = (struct Blacklist *) n->data;
		blptr->status = 0;

		if (u == NULL)
			return;

		initiate_blacklist_dnsquery(blptr, u);
	}
}

static void abort_blacklist_queries(user_t *u)
{
	mowgli_node_t *n, *tn;
	mowgli_list_t *l;
	struct BlacklistClient *blcptr;

	if (u == NULL)
		return;

	l = dnsbl_queries(u);

	MOWGLI_ITER_FOREACH_SAFE(n, tn, l->head)
	{
		blcptr = n->data;
		mowgli_node_delete(&blcptr->node, l);
		unref_blacklist(blcptr->blacklist);
		delete_resolver_queries(&blcptr->dns_query);
		free(blcptr);
	}
}

static void destroy_blacklists(void)
{
	mowgli_node_t *n, *tn;
	struct Blacklist *blptr;

	MOWGLI_ITER_FOREACH_SAFE(n, tn, blacklist_list.head)
	{
		blptr = n->data;
		blptr->hits = 0; /* keep it simple and consistent */
		free(n->data);
		mowgli_node_delete(n, &blacklist_list);
		mowgli_node_free(n);
	}
}

static int dnsbl_config_handler(config_entry_t *ce)
{
	config_entry_t *cce;

	for (cce = ce->ce_entries; cce != NULL; cce = cce->ce_next)
	{
		char *line = sstrdup(cce->ce_varname);
		new_blacklist(line);
	}

	return 0;
}

static void dnsbl_config_purge(void *unused)
{
	destroy_blacklists();
}

static void check_dnsbls(hook_user_nick_t *data)
{
	user_t *u = data->u;
	service_t *svs;
	mowgli_list_t *l;

    if (!u)
        return;

    if (is_internal_client(u))
        return;

	if (!action)
		return;

	lookup_blacklists(u);

	l = dnsbl_queries(u);
	struct Blacklist *dnsbl_listed = listed_in_dnsbl(u);

	svs = service_find("operserv");

	if (MOWGLI_LIST_LENGTH(l) > 0)
	{
		if (dnsbl_listed == NULL)
			return;

		if (!strcasecmp("SNOOP", action))
		{
			slog(LG_INFO, "DNSBL: \2%s\2!%s@%s [%s] is listed in DNS Blacklist.", u->nick, u->user, u->host, u->gecos);
			abort_blacklist_queries(u);
			return;
		}
		else if (!strcasecmp("NOTIFY", action))
		{
			slog(LG_INFO, "DNSBL: \2%s\2!%s@%s [%s] who is listed in DNS Blacklist %s.", u->nick, u->user, u->host, u->gecos, dnsbl_listed->host);
			notice(svs->nick, u->nick, "Your IP address %s is listed in DNS Blacklist %s", u->ip, dnsbl_listed->host);
			abort_blacklist_queries(u);
			return;
		}
		else if (!strcasecmp("KLINE", action))
		{
			slog(LG_INFO, "DNSBL: k-lining \2%s\2!%s@%s [%s] who is listed in DNS Blacklist %s.", u->nick, u->user, u->host, u->gecos, dnsbl_listed->host);
			notice(svs->nick, u->nick, "Your IP address %s is listed in DNS Blacklist %s", u->ip, dnsbl_listed->host);
			kline_sts("*", "*", u->host, 86400, "Banned (DNS Blacklist)");
			abort_blacklist_queries(u);
			return;
		}
	}	
}
static void osinfo_hook(sourceinfo_t *si)
{
	mowgli_node_t *n;

    MOWGLI_ITER_FOREACH(n, blacklist_list.head)
    {
        struct Blacklist *blptr = (struct Blacklist *) n->data;

		command_success_nodata(si, "Blacklists: %s", blptr->host);
	}
}

void
_modinit(module_t *m)
{
	MODULE_TRY_REQUEST_SYMBOL(m, os_set_cmdtree, "operserv/set", "os_set_cmdtree");

	hook_add_event("config_purge");
	hook_add_config_purge(dnsbl_config_purge);

	hook_add_event("user_add");
	hook_add_user_add(check_dnsbls);

	hook_add_event("operserv_info");
	hook_add_operserv_info(osinfo_hook);

	add_conf_item("BLACKLISTS", &conf_gi_table, dnsbl_config_handler);
	command_add(&os_set_dnsblaction, *os_set_cmdtree);
}

void
_moddeinit(module_unload_intent_t intent)
{
	hook_del_user_add(check_dnsbls);
	hook_del_config_purge(dnsbl_config_purge);
	hook_del_operserv_info(osinfo_hook);

	del_conf_item("BLACKLISTS", &conf_gi_table);
	command_delete(&os_set_dnsblaction, *os_set_cmdtree);
}
