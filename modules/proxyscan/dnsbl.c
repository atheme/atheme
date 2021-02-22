/*
 * SPDX-License-Identifier: (ISC AND BSD-3-Clause)
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 * SPDX-URL: https://spdx.org/licenses/BSD-3-Clause.html
 *
 * Copyright (C) 2012 William Pitcock <nenolod@dereferenced.org>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 */

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
 */

// To configure/use: see the proxyscan{} section of your atheme.conf

#include <atheme.h>

#define DNSBL_ELIST_PERSIST_MDNAME "atheme.proxyscan.dnsbl.elist"
#define IRCD_RES_HOSTLEN 255

// A configured DNSBL
struct Blacklist {
	struct atheme_object parent;
	char host[IRCD_RES_HOSTLEN + 1];
	unsigned int hits;
	time_t lastwarning;

	mowgli_node_t node;
};

// A lookup in progress for a particular DNSBL for a particular client
struct BlacklistClient {
	struct Blacklist *blacklist;
	struct user *u;
	mowgli_dns_query_t dns_query;
	mowgli_node_t node;
};

struct dnsbl_exemption
{
	char *ip;
	time_t exempt_ts;
	char *creator;
	char *reason;

	mowgli_node_t node;
};

static enum dnsbl_action {
	DNSBL_ACT_NONE,
	DNSBL_ACT_NOTIFY,
	DNSBL_ACT_SNOOP,
	DNSBL_ACT_KLINE,
} action;

#define ITEM_DESC(x) [DNSBL_ACT_ ## x] = #x

static const char *action_names[] = {
	ITEM_DESC(NONE),
	ITEM_DESC(NOTIFY),
	ITEM_DESC(SNOOP),
	ITEM_DESC(KLINE),
	NULL
};

#undef ITEM_DESC

static mowgli_list_t blacklist_list = { NULL, NULL, 0 };
static mowgli_patricia_t **os_set_cmdtree = NULL;

static struct service *proxyscan = NULL;
static mowgli_list_t *dnsbl_elist = NULL;
static mowgli_dns_t *dns_base = NULL;

static inline mowgli_list_t *
dnsbl_queries(struct user *u)
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

static void
os_cmd_set_dnsblaction(struct sourceinfo *si, int parc, char *parv[])
{
	char *act = parv[0];

	if (!act)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "DNSBLACTION");
		command_fail(si, fault_needmoreparams, _("Syntax: SET DNSBLACTION <action>"));
		return;
	}

	for (enum dnsbl_action n = 0; action_names[n] != NULL; n++)
	{
		if (!strcasecmp(action_names[n], act))
		{
			action = n;
			command_success_nodata(si, _("DNSBLACTION successfully set to \2%s\2"), action_names[n]);
			logcommand(si, CMDLOG_ADMIN, "SET:DNSBLACTION: \2%s\2", action_names[n]);
			return;
		}
	}

	command_fail(si, fault_badparams, _("Invalid action given."));
}

static void
ps_cmd_dnsblexempt(struct sourceinfo *si, int parc, char *parv[])
{
	char *command = parv[0];
	char *ip = parv[1];
	char *reason = parv[2];
	mowgli_node_t *n, *tn;
	struct dnsbl_exemption *de;

	if (!command)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "DNSBLEXEMPT");
		command_fail(si, fault_needmoreparams, _("Syntax: DNSBLEXEMPT ADD|DEL|LIST [ip] [reason]"));
		return;
	}

	if (!strcasecmp("ADD", command))
	{

		if (!ip || !reason)
		{
			command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "DNSBLEXEMPT");
			command_fail(si, fault_needmoreparams, _("Syntax: DNSBLEXEMPT ADD <ip> <reason>"));
			return;
		}

		MOWGLI_ITER_FOREACH(n, dnsbl_elist->head)
		{
			de = n->data;

			if (!irccasecmp(de->ip, ip))
			{
				command_success_nodata(si, _("\2%s\2 has already been entered into the DNSBL exempts list."), ip);
				return;
			}
		}

		de = smalloc(sizeof *de);
		de->exempt_ts = CURRTIME;;
		de->creator = sstrdup(get_source_name(si));
		de->reason = sstrdup(reason);
		de->ip = sstrdup(ip);
		mowgli_node_add(de, &de->node, dnsbl_elist);

		command_success_nodata(si, _("You have added \2%s\2 to the DNSBL exempts list."), ip);
		logcommand(si, CMDLOG_ADMIN, "DNSBL:EXEMPT:ADD: \2%s\2 \2%s\2", ip, reason);
	}
	else if (!strcasecmp("DEL", command))
	{

		if (!ip)
		{
			command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "DNSBLEXEMPT");
			command_fail(si, fault_needmoreparams, _("Syntax: DNSBLEXEMPT DEL <ip>"));
			return;
		}

		MOWGLI_ITER_FOREACH_SAFE(n, tn, dnsbl_elist->head)
		{
			de = n->data;

			if (!irccasecmp(de->ip, ip))
			{
				logcommand(si, CMDLOG_SET, "DNSBL:EXEMPT:DEL: \2%s\2", de->ip);
				command_success_nodata(si, _("DNSBL Exempt IP \2%s\2 has been deleted."), de->ip);

				mowgli_node_delete(n, dnsbl_elist);

				sfree(de->creator);
				sfree(de->reason);
				sfree(de->ip);
				sfree(de);

				return;
			}
		}
		command_success_nodata(si, _("IP \2%s\2 not found in DNSBL Exempt database."), ip);
	}
	else if (!strcasecmp("LIST", command))
	{
		char buf[BUFSIZE];
		struct tm *tm;

		MOWGLI_ITER_FOREACH(n, dnsbl_elist->head)
		{
			de = n->data;

			tm = localtime(&de->exempt_ts);
			strftime(buf, BUFSIZE, TIME_FORMAT, tm);
			command_success_nodata(si, _("IP: \2%s\2 Reason: \2%s\2 (%s - %s)"),
					de->ip, de->reason, de->creator, buf);
		}
		command_success_nodata(si, _("End of list."));
		logcommand(si, CMDLOG_GET, "DNSBL:EXEMPT:LIST");
	}
	else
	{
		command_fail(si, fault_needmoreparams, STR_INVALID_PARAMS, "DNSBLEXEMPT");
		command_fail(si, fault_needmoreparams, _("Syntax: DNSBLEXEMPT ADD|DEL|LIST [ip] [reason]"));
		return;
	}
}

static void
abort_blacklist_queries(struct user *u)
{
	mowgli_node_t *n, *tn;
	mowgli_list_t *l = dnsbl_queries(u);

	MOWGLI_ITER_FOREACH_SAFE(n, tn, l->head)
	{
		struct BlacklistClient *blcptr = n->data;

		mowgli_dns_delete_query(dns_base, &blcptr->dns_query);
		mowgli_node_delete(n, l);
		sfree(blcptr);
	}
}

static void
dnsbl_hit(struct user *u, struct Blacklist *blptr)
{
	struct service *svs;

	svs = service_find("operserv");

	abort_blacklist_queries(u);

	switch (action)
	{
		case DNSBL_ACT_KLINE:
			if (! (u->flags & UF_KLINESENT)) {
				slog(LG_INFO, "DNSBL: k-lining \2%s\2!%s@%s [%s] who is listed in DNS Blacklist %s.", u->nick, u->user, u->host, u->gecos, blptr->host);
				notice(svs->nick, u->nick, "Your IP address %s is listed in DNS Blacklist %s", u->ip, blptr->host);
				kline_add("*", u->ip, "Banned (DNS Blacklist)", SECONDS_PER_DAY, "Proxyscan");
				u->flags |= UF_KLINESENT;
			}
			break;

		case DNSBL_ACT_NOTIFY:
			notice(svs->nick, u->nick, "Your IP address %s is listed in DNS Blacklist %s", u->ip, blptr->host);
			ATHEME_FALLTHROUGH;

		case DNSBL_ACT_SNOOP:
			slog(LG_INFO, "DNSBL: \2%s\2!%s@%s [%s] is listed in DNS Blacklist %s.", u->nick, u->user, u->host, u->gecos, blptr->host);
			break;

		case DNSBL_ACT_NONE:
			break; // do nothing
	}
}

static void
blacklist_dns_callback(mowgli_dns_reply_t *reply, int result, void *vptr)
{
	struct BlacklistClient *blcptr = (struct BlacklistClient *) vptr;
	int listed = 0;
	mowgli_list_t *l;

	if (blcptr == NULL)
		return;

	if (blcptr->u == NULL)
	{
		sfree(blcptr);
		return;
	}

	l = dnsbl_queries(blcptr->u);
	mowgli_node_delete(&blcptr->node, l);

	if (reply != NULL)
	{
		// only accept 127.x.y.z as a listing
		if (reply->addr.addr.ss_family == AF_INET &&
				!memcmp(&((struct sockaddr_in *)&reply->addr.addr)->sin_addr, "\177", 1))
			listed++;
		else if (blcptr->blacklist->lastwarning + SECONDS_PER_HOUR < CURRTIME)
		{
			slog(LG_DEBUG,
					"Garbage reply from blacklist %s",
					blcptr->blacklist->host);
			blcptr->blacklist->lastwarning = CURRTIME;
		}
	}

	// they have a blacklist entry for this client
	if (listed)
		dnsbl_hit(blcptr->u, blcptr->blacklist);

	atheme_object_unref(blcptr->blacklist);
	sfree(blcptr);
}

/* XXX: no IPv6 implementation, not to concerned right now though. */
/* 2015-12-06: at least we shouldn't crash on bad inputs anymore... -bcode */
static void
initiate_blacklist_dnsquery(struct Blacklist *blptr, struct user *u)
{
	char buf[IRCD_RES_HOSTLEN + 1];
	unsigned int ip[4];
	mowgli_list_t *l;

	if (u->ip == NULL)
		return;

	// A sscanf worked fine for chary for many years, it'll be fine here
	if (sscanf(u->ip, "%u.%u.%u.%u", &ip[3], &ip[2], &ip[1], &ip[0]) != 4)
		return;

	struct BlacklistClient *blcptr = smalloc(sizeof *blcptr);

	blcptr->blacklist = atheme_object_ref(blptr);
	blcptr->u = u;

	blcptr->dns_query.ptr = blcptr;
	blcptr->dns_query.callback = blacklist_dns_callback;

	// becomes 2.0.0.127.torbl.ahbl.org or whatever
	snprintf(buf, sizeof buf, "%u.%u.%u.%u.%s", ip[0], ip[1], ip[2], ip[3], blptr->host);

	mowgli_dns_gethost_byname(dns_base, buf, &blcptr->dns_query, MOWGLI_DNS_T_A);

	l = dnsbl_queries(u);
	mowgli_node_add(blcptr, &blcptr->node, l);
}

static void
lookup_blacklists(struct user *u)
{
	mowgli_node_t *n;

	MOWGLI_ITER_FOREACH(n, blacklist_list.head)
	{
		struct Blacklist *blptr = (struct Blacklist *) n->data;

		if (u == NULL)
			return;

		initiate_blacklist_dnsquery(blptr, u);
	}
}

static void
ps_cmd_dnsblscan(struct sourceinfo *si, int parc, char *parv[])
{
	char *user = parv[0];
	struct user *u;

	if (!user)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "DNSBLSCAN");
		command_fail(si, fault_needmoreparams, _("Syntax: DNSBLSCAN <user>"));
		return;
	}

	if ((u = user_find_named(user)))
	{
		lookup_blacklists(u);
		logcommand(si, CMDLOG_ADMIN, "DNSBLSCAN: %s", user);
		command_success_nodata(si, _("%s has been scanned."), user);
		return;
	}
	else
	{
		command_fail(si, fault_badparams, _("User %s is not on the network, you can not scan them."), user);
		return;
	}
}

// private interfaces
static struct Blacklist *
find_blacklist(char *name)
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

// public interfaces
static struct Blacklist *
new_blacklist(char *name)
{
	struct Blacklist *blptr;

	if (name == NULL)
		return NULL;

	blptr = find_blacklist(name);

	if (blptr == NULL)
	{
		blptr = smalloc(sizeof *blptr);
		atheme_object_init(atheme_object(blptr), "proxyscan dnsbl", NULL);
		mowgli_node_add(atheme_object_ref(blptr), &blptr->node, &blacklist_list);
	}

	mowgli_strlcpy(blptr->host, name, sizeof blptr->host);
	blptr->lastwarning = 0;

	return blptr;
}

static void
destroy_blacklists(void)
{
	mowgli_node_t *n, *tn;
	struct Blacklist *blptr;

	MOWGLI_ITER_FOREACH_SAFE(n, tn, blacklist_list.head)
	{
		blptr = n->data;

		mowgli_node_delete(n, &blacklist_list);
		atheme_object_unref(blptr);
	}
}

static int
dnsbl_config_handler(mowgli_config_file_entry_t *ce)
{
	mowgli_config_file_entry_t *cce;

	MOWGLI_ITER_FOREACH(cce, ce->entries)
	{
		char *line = sstrdup(cce->varname);
		new_blacklist(line);
		sfree(line);
	}

	return 0;
}

static void
dnsbl_config_purge(void *unused)
{
	destroy_blacklists();
}

static int
dnsbl_action_config_handler(mowgli_config_file_entry_t *ce)
{
	if (ce->vardata == NULL)
	{
		conf_report_warning(ce, "no parameter for configuration option");
		return 0;
	}

	for (enum dnsbl_action n = 0; action_names[n] != NULL; n++)
	{
		if (!strcasecmp(action_names[n], ce->vardata))
		{
			action = n;
			return 0;
		}
	}

	conf_report_warning(ce, "invalid parameter for configuration option");
	return 0;
}

static void
check_dnsbls(struct hook_user_nick *data)
{
	struct user *u = data->u;
	mowgli_node_t *n;

	if (!u)
		return;

	if (is_internal_client(u))
		return;

	if (action == DNSBL_ACT_NONE)
		return;

	MOWGLI_ITER_FOREACH(n, dnsbl_elist->head)
	{
		struct dnsbl_exemption *de = n->data;

		if (!irccasecmp(de->ip, u->ip))
			return;
	}

	lookup_blacklists(u);
}

static void
osinfo_hook(struct sourceinfo *si)
{
	mowgli_node_t *n;
	const char *name = action_names[action];

	return_if_fail(name != NULL);

	command_success_nodata(si, _("Action taken when a user is on a DNSBL: %s"), name);

	MOWGLI_ITER_FOREACH(n, blacklist_list.head)
	{
		struct Blacklist *blptr = (struct Blacklist *) n->data;

		command_success_nodata(si, _("Using DNSBL: %s"), blptr->host);
	}
}

static void
write_dnsbl_exempt_db(struct database_handle *db)
{
	mowgli_node_t *n;

	MOWGLI_ITER_FOREACH(n, dnsbl_elist->head)
	{
		struct dnsbl_exemption *de = n->data;

		db_start_row(db, "BLE");
		db_write_word(db, de->ip);
		db_write_time(db, de->exempt_ts);
		db_write_word(db, de->creator);
		db_write_word(db, de->reason);
		db_commit_row(db);
	}
}

static void
db_h_ble(struct database_handle *db, const char *type)
{
	const char *ip = db_sread_word(db);
	time_t exempt_ts = db_sread_time(db);
	const char *creator = db_sread_word(db);
	const char *reason = db_sread_word(db);

	struct dnsbl_exemption *const de = smalloc(sizeof *de);
	de->ip = sstrdup(ip);
	de->exempt_ts = exempt_ts;
	de->creator = sstrdup(creator);
	de->reason = sstrdup(reason);

	mowgli_node_add(de, &de->node, dnsbl_elist);
}

static struct command os_set_dnsblaction = {
	.name           = "DNSBLACTION",
	.desc           = N_("Changes what happens to a user when they hit a DNSBL."),
	.access         = PRIV_USER_ADMIN,
	.maxparc        = 1,
	.cmd            = &os_cmd_set_dnsblaction,
	.help           = { .path = "proxyscan/set_dnsblaction" },
};

static struct command ps_dnsblexempt = {
	.name           = "DNSBLEXEMPT",
	.desc           = N_("Manage the list of IP's exempt from DNSBL checking."),
	.access         = PRIV_USER_ADMIN,
	.maxparc        = 3,
	.cmd            = &ps_cmd_dnsblexempt,
	.help           = { .path = "proxyscan/dnsblexempt" },
};

static struct command ps_dnsblscan = {
	.name           = "DNSBLSCAN",
	.desc           = N_("Manually scan if a user is in a DNSBL."),
	.access         = PRIV_USER_ADMIN,
	.maxparc        = 1,
	.cmd            = &ps_cmd_dnsblscan,
	.help           = { .path = "proxyscan/dnsblscan" },
};

static void
mod_init(struct module *const restrict m)
{
	if (!module_find_published("backend/opensex"))
	{
		(void) slog(LG_ERROR, "Module %s requires use of the OpenSEX database backend, refusing to load.", m->name);
		m->mflags |= MODFLAG_FAIL;
		return;
	}

	MODULE_CONFLICT(m, "contrib/dnsbl")
	MODULE_TRY_REQUEST_DEPENDENCY(m, "proxyscan/main")
	MODULE_TRY_REQUEST_SYMBOL(m, os_set_cmdtree, "operserv/set_core", "os_set_cmdtree")

	if (! (proxyscan = service_find("proxyscan")))
	{
		(void) slog(LG_ERROR, "%s: service_find() for ProxyScan failed! (BUG)", m->name);
		m->mflags |= MODFLAG_FAIL;
		return;
	}

	if (! (dnsbl_elist = mowgli_global_storage_get(DNSBL_ELIST_PERSIST_MDNAME)))
		dnsbl_elist = mowgli_list_create();
	else
		mowgli_global_storage_free(DNSBL_ELIST_PERSIST_MDNAME);

	if (! dnsbl_elist)
	{
		(void) slog(LG_ERROR, "%s: failed to create Mowgli list for exemptions", m->name);
		m->mflags |= MODFLAG_FAIL;
		return;
	}
	if (! (dns_base = mowgli_dns_create(base_eventloop, MOWGLI_DNS_TYPE_ASYNC)))
	{
		(void) slog(LG_ERROR, "%s: failed to create Mowgli DNS resolver object", m->name);
		(void) mowgli_list_free(dnsbl_elist);
		m->mflags |= MODFLAG_FAIL;
		return;
	}

	hook_add_config_purge(dnsbl_config_purge);
	hook_add_db_write(write_dnsbl_exempt_db);
	hook_add_operserv_info(osinfo_hook);
	hook_add_user_add(check_dnsbls);
	hook_add_user_delete(abort_blacklist_queries);

	db_register_type_handler("BLE", db_h_ble);

	command_add(&os_set_dnsblaction, *os_set_cmdtree);

	service_bind_command(proxyscan, &ps_dnsblexempt);
	service_bind_command(proxyscan, &ps_dnsblscan);

	add_conf_item("DNSBL_ACTION", &proxyscan->conf_table, dnsbl_action_config_handler);
	add_conf_item("BLACKLISTS", &proxyscan->conf_table, dnsbl_config_handler);

	m->mflags |= MODFLAG_DBHANDLER;
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	mowgli_global_storage_put(DNSBL_ELIST_PERSIST_MDNAME, dnsbl_elist);
	mowgli_dns_destroy(dns_base);

	hook_del_config_purge(dnsbl_config_purge);
	hook_del_db_write(write_dnsbl_exempt_db);
	hook_del_operserv_info(osinfo_hook);
	hook_del_user_add(check_dnsbls);
	hook_del_user_delete(abort_blacklist_queries);

	db_unregister_type_handler("BLE");

	command_delete(&os_set_dnsblaction, *os_set_cmdtree);

	service_unbind_command(proxyscan, &ps_dnsblexempt);
	service_unbind_command(proxyscan, &ps_dnsblscan);

	del_conf_item("DNSBL_ACTION", &proxyscan->conf_table);
	del_conf_item("BLACKLISTS", &proxyscan->conf_table);
}

SIMPLE_DECLARE_MODULE_V1("proxyscan/dnsbl", MODULE_UNLOAD_CAPABILITY_RELOAD_ONLY)
