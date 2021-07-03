/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2010 William Pitcock <nenolod -at- nenolod.net>
 * Copyright (C) 2014-2017 Xtheme Development Group (https://xtheme.org/)
 *
 * Allows opers to offer vhosts to users
 */

#include <atheme.h>
#include "hostserv.h"
#include "../groupserv/groupserv.h"

struct hsoffered
{
	char *vhost;
	time_t vhost_ts;
	stringref creator;
	struct myentity *group;
	mowgli_node_t node;
};

static mowgli_list_t hs_offeredlist;

static void
write_hsofferdb(struct database_handle *db)
{
	mowgli_node_t *n;

	MOWGLI_ITER_FOREACH(n, hs_offeredlist.head)
	{
		struct hsoffered *l = n->data;

		db_start_row(db, "HO");

		if (l->group != NULL)
			db_write_word(db, l->group->name);

		db_write_word(db, l->vhost);
		db_write_time(db, l->vhost_ts);
		db_write_word(db, l->creator);
		db_commit_row(db);
	}

}

static void
db_h_ho(struct database_handle *db, const char *type)
{
	const char *buf;
	time_t vhost_ts;
	const char *creator;
	struct myentity *mt = NULL;

	buf = db_sread_word(db);
	if (*buf == '!')
	{
		mt = myentity_find(buf);
		buf = db_sread_word(db);
	}

	vhost_ts = db_sread_time(db);
	creator = db_sread_word(db);

	struct hsoffered *const l = smalloc(sizeof *l);
	l->group = mt;
	l->vhost = sstrdup(buf);
	l->vhost_ts = vhost_ts;
	l->creator = strshare_get(creator);

	mowgli_node_add(l, &l->node, &hs_offeredlist);
}

static inline struct hsoffered *
hs_offer_find(const char *host, struct myentity *mt)
{
	mowgli_node_t *n;
	struct hsoffered *l;

	MOWGLI_ITER_FOREACH(n, hs_offeredlist.head)
	{
		l = n->data;

		if (!irccasecmp(l->vhost, host) && (l->group == mt || mt == NULL))
			return l;
	}

	return NULL;
}

static void
remove_group_offered_hosts(struct mygroup *mg)
{
	return_if_fail(mg != NULL);

	struct myentity *mt = entity(mg);
	mowgli_node_t *n, *tn;
	struct hsoffered *l;

	MOWGLI_ITER_FOREACH_SAFE(n, tn, hs_offeredlist.head)
	{
		l = n->data;

		if (l->group != NULL && l->group == mt)
		{
			slog(LG_VERBOSE, "remove_group_offered_hosts(): removing %s (group %s)", l->vhost, l->group->name);

			mowgli_node_delete(n, &hs_offeredlist);

			strshare_unref(l->creator);
			sfree(l->vhost);
			sfree(l);
		}
	}
}

// OFFER <host>
static void
hs_cmd_offer(struct sourceinfo *si, int parc, char *parv[])
{
	char *group = parv[0];
	char *host;
	struct hsoffered *l;
	struct myentity *mt = NULL;

	if (!group)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "OFFER");
		command_fail(si, fault_needmoreparams, _("Syntax: OFFER [!group] <vhost>"));
		return;
	}

	if (*group != '!')
	{
		host = group;
		group = NULL;
	}
	else
		host = parv[1];

	if (!host)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "OFFER");
		command_fail(si, fault_needmoreparams, _("Syntax: OFFER [!group] <vhost>"));
		return;
	}

	if (si->smu == NULL)
	{
		command_fail(si, fault_noprivs, STR_NOT_LOGGED_IN);
		return;
	}

	if (group != NULL)
	{
		mt = myentity_find(group);
		if (mt == NULL)
		{
			command_fail(si, fault_badparams, _("The requested group does not exist."));
			return;
		}
	}

	l = hs_offer_find(host, mt);
	if (l != NULL)
	{
		command_fail(si, fault_badparams, _("The requested offer already exists."));
		return;
	}

	l = smalloc(sizeof *l);
	l->group = mt;
	l->vhost = sstrdup(host);
	l->vhost_ts = CURRTIME;;
	l->creator = strshare_ref(entity(si->smu)->name);

	mowgli_node_add(l, &l->node, &hs_offeredlist);

	if (mt != NULL)
	{
		command_success_nodata(si, _("You have offered vhost \2%s\2 to group \2%s\2."), host, group);
		logcommand(si, CMDLOG_ADMIN, "OFFER: \2%s\2 to \2%s\2", host, group);
	}
	else
	{
		command_success_nodata(si, _("You have offered vhost \2%s\2."), host);
		logcommand(si, CMDLOG_ADMIN, "OFFER: \2%s\2", host);
	}

	return;
}

// UNOFFER <vhost>
static void
hs_cmd_unoffer(struct sourceinfo *si, int parc, char *parv[])
{
	char *host = parv[0];
	struct hsoffered *l;
	mowgli_node_t *n;

	if (!host)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "UNOFFER");
		command_fail(si, fault_needmoreparams, _("Syntax: UNOFFER <vhost>"));
		return;
	}

	l = hs_offer_find(host, NULL);
	if (l == NULL)
	{
		command_fail(si, fault_nosuch_target, _("vhost \2%s\2 not found in vhost offer database."), host);
		return;
	}

	logcommand(si, CMDLOG_ADMIN, "UNOFFER: \2%s\2", host);

	while (l != NULL)
	{
		mowgli_node_delete(&l->node, &hs_offeredlist);

		strshare_unref(l->creator);
		sfree(l->vhost);
		sfree(l);

		l = hs_offer_find(host, NULL);
	}

	command_success_nodata(si, _("You have unoffered vhost \2%s\2."), host);
}

static bool
myuser_is_in_group(struct myuser *mu, struct myentity *mt)
{
	struct mygroup *mg;
	mowgli_node_t *n;

	return_val_if_fail(mt != NULL, false);
	return_val_if_fail((mg = group(mt)) != NULL, false);

	if (!mu) // NULL users can't be in groups! :o
		return false;

	MOWGLI_ITER_FOREACH(n, mg->acs.head)
	{
		struct groupacs *ga = n->data;

		if (ga->mt == entity(mu) && ga->flags & GA_VHOST)
			return true;
	}

	return false;
}

static void
take_vhost(struct sourceinfo *si, const char *host)
{
	char local_host[BUFSIZE];
	if (strstr(host, "$account"))
	{
		mowgli_strlcpy(local_host, host, BUFSIZE);
		replace(local_host, BUFSIZE, "$account", entity(si->smu)->name);
		host = local_host;
	}

	if (!check_vhost_validity(si, host))
		return;

	logcommand(si, CMDLOG_GET, "TAKE: \2%s\2 for \2%s\2", host, entity(si->smu)->name);

	command_success_nodata(si, _("You have taken vhost \2%s\2."), host);
	hs_sethost_all(si->smu, host, get_source_name(si));
	do_sethost_all(si->smu, host);
}

// TAKE <vhost>
static void
hs_cmd_take(struct sourceinfo *si, int parc, char *parv[])
{
	char *host = parv[0];
	struct hsoffered *l;
	mowgli_node_t *n;
	struct metadata *md;
	time_t vhost_time = 0;
	int vhosts_available = 0;
	const char *offered_vhost = NULL; // For the case where there's only one available

	if (si->smu == NULL)
	{
		command_fail(si, fault_nochange, _("You can't take vhosts when not logged in"));
		return;
	}

	if (metadata_find(si->smu, "private:restrict:setter"))
	{
		command_fail(si, fault_noprivs, _("You have been restricted from taking vhosts by network staff"));
		return;
	}

	if ((md = metadata_find(si->smu, "private:usercloak-timestamp")))
		vhost_time = atoi(md->value);

	if (md && config_options.vhost_change && CURRTIME < (vhost_time + config_options.vhost_change))
	{
		command_fail(si, fault_noprivs, _("You must wait at least \2%u\2 days between changes to your vHost."),
			(config_options.vhost_change / SECONDS_PER_DAY));
		return;
	}

	// Now we know there's a valid account and the other checks passed, we can check how many offers are available
	MOWGLI_ITER_FOREACH(n, hs_offeredlist.head)
	{
		l = n->data;
		if (l->group != NULL && !myuser_is_in_group(si->smu, l->group))
			continue;
		offered_vhost = l->vhost;
		vhosts_available++;
	}

	// If there's only one vhost offered, and no host argument was provided, then take the one that's available
	if (!host && vhosts_available == 1)
	{
		take_vhost(si, offered_vhost);
		return;
	}

	// If we get here, either there are multiple vhosts offered, or a host was explicitly specified to take
	if (!host)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "TAKE");
		command_fail(si, fault_needmoreparams, _("Syntax: TAKE <vhost>"));
		return;
	}

	MOWGLI_ITER_FOREACH(n, hs_offeredlist.head)
	{
		l = n->data;

		if (l->group != NULL && !myuser_is_in_group(si->smu, l->group))
			continue;

		if (!irccasecmp(l->vhost, host))
		{
			take_vhost(si, host);

			return;
		}
	}

	command_success_nodata(si, _("vhost \2%s\2 not found in vhost offer database."), host);
}

// OFFERLIST
static void
hs_cmd_offerlist(struct sourceinfo *si, int parc, char *parv[])
{
	struct hsoffered *l;
	mowgli_node_t *n;
	char buf[BUFSIZE];
	struct tm *tm;

	MOWGLI_ITER_FOREACH(n, hs_offeredlist.head)
	{
		l = n->data;

		if (l->group != NULL && !myuser_is_in_group(si->smu, l->group) && !has_priv(si, PRIV_GROUP_ADMIN))
			continue;

		tm = localtime(&l->vhost_ts);
		strftime(buf, BUFSIZE, TIME_FORMAT, tm);

		if(l->group != NULL)
			command_success_nodata(si, _("vHost: \2%s\2, Group: \2%s\2, Creator: \2%s\2 (%s)"),
						l->vhost, entity(l->group)->name, l->creator, buf);
		else
			command_success_nodata(si, _("vHost: \2%s\2, Creator: \2%s\2 (%s)"),
						l->vhost, l->creator, buf);
	}
	command_success_nodata(si, _("End of list."));
	logcommand(si, CMDLOG_GET, "OFFERLIST");
}

static struct command hs_offer = {
	.name           = "OFFER",
	.desc           = N_("Sets vhosts available for users to take."),
	.access         = PRIV_USER_VHOST,
	.maxparc        = 2,
	.cmd            = &hs_cmd_offer,
	.help           = { .path = "hostserv/offer" },
};

static struct command hs_unoffer = {
	.name           = "UNOFFER",
	.desc           = N_("Removes a vhost from the list that users can take."),
	.access         = PRIV_USER_VHOST,
	.maxparc        = 2,
	.cmd            = &hs_cmd_unoffer,
	.help           = { .path = "hostserv/unoffer" },
};

static struct command hs_offerlist = {
	.name           = "OFFERLIST",
	.desc           = N_("Lists all available vhosts."),
	.access         = AC_NONE,
	.maxparc        = 1,
	.cmd            = &hs_cmd_offerlist,
	.help           = { .path = "hostserv/offerlist" },
};

static struct command hs_take = {
	.name           = "TAKE",
	.desc           = N_("Take an offered vhost for use."),
	.access         = AC_AUTHENTICATED,
	.maxparc        = 2,
	.cmd            = &hs_cmd_take,
	.help           = { .path = "hostserv/take" },
};

static void
mod_init(struct module *const restrict m)
{
	if (!module_find_published("backend/opensex"))
	{
		slog(LG_INFO, "Module %s requires use of the OpenSEX database backend, refusing to load.", m->name);
		m->mflags |= MODFLAG_FAIL;
		return;
	}

	MODULE_TRY_REQUEST_DEPENDENCY(m, "hostserv/main")

	hook_add_db_write(write_hsofferdb);
	db_register_type_handler("HO", db_h_ho);

	hook_add_group_drop(remove_group_offered_hosts);

 	service_named_bind_command("hostserv", &hs_offer);
	service_named_bind_command("hostserv", &hs_unoffer);
	service_named_bind_command("hostserv", &hs_offerlist);
	service_named_bind_command("hostserv", &hs_take);

	m->mflags |= MODFLAG_DBHANDLER;
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{

}

SIMPLE_DECLARE_MODULE_V1("hostserv/offer", MODULE_UNLOAD_CAPABILITY_NEVER)
