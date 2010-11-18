/*
 * Copyright (c) 2005 William Pitcock <nenolod -at- nenolod.net>
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Allows opers to offer vhosts to users
 *
 */

#include "atheme.h"
#include "hostserv.h"
#include "../groupserv/groupserv.h"

DECLARE_MODULE_V1
(
	"hostserv/offer", true, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.net>"
);

mowgli_list_t *conf_hs_table;

static void hs_cmd_offer(sourceinfo_t *si, int parc, char *parv[]);
static void hs_cmd_unoffer(sourceinfo_t *si, int parc, char *parv[]);
static void hs_cmd_offerlist(sourceinfo_t *si, int parc, char *parv[]);
static void hs_cmd_take(sourceinfo_t *si, int parc, char *parv[]);

static void write_hsofferdb(database_handle_t *db);
static void db_h_ho(database_handle_t *db, const char *type);

command_t hs_offer = { "OFFER", N_("Sets vhosts available for users to take."), PRIV_USER_VHOST, 2, hs_cmd_offer, { .path = "hostserv/offer" } };
command_t hs_unoffer = { "UNOFFER", N_("Removes a vhost from the list that users can take."), PRIV_USER_VHOST, 2, hs_cmd_unoffer, { .path = "hostserv/unoffer" } };
command_t hs_offerlist = { "OFFERLIST", N_("Lists all available vhosts."), AC_NONE, 1, hs_cmd_offerlist, { .path = "hostserv/offerlist" } };
command_t hs_take = { "TAKE", N_("Take an offered vhost for use."), AC_NONE, 2, hs_cmd_take, { .path = "hostserv/take" } };

struct hsoffered_ {
	char *vhost;
	time_t vhost_ts;
	char *creator;
	myentity_t *group;
};

typedef struct hsoffered_ hsoffered_t;

mowgli_list_t hs_offeredlist;

void _modinit(module_t *m)
{
	if (!module_find_published("backend/opensex"))
	{
		slog(LG_INFO, "Module %s requires use of the OpenSEX database backend, refusing to load.", m->header->name);
		m->mflags = MODTYPE_FAIL;
		return;
	}

	MODULE_USE_SYMBOL(conf_hs_table, "hostserv/main", "conf_hs_table");

	hook_add_db_write(write_hsofferdb);
	db_register_type_handler("HO", db_h_ho);

 	service_named_bind_command("hostserv", &hs_offer);
	service_named_bind_command("hostserv", &hs_unoffer);
	service_named_bind_command("hostserv", &hs_offerlist);
	service_named_bind_command("hostserv", &hs_take);
}

void _moddeinit(void)
{
	hook_del_db_write(write_hsofferdb);
	db_unregister_type_handler("HO");

 	service_named_unbind_command("hostserv", &hs_offer);
	service_named_unbind_command("hostserv", &hs_unoffer);
	service_named_unbind_command("hostserv", &hs_offerlist);
	service_named_unbind_command("hostserv", &hs_take);
}

static void hs_sethost_all(myuser_t *mu, const char *host)
{
	mowgli_node_t *n;
	mynick_t *mn;
	char buf[BUFSIZE];

	MOWGLI_ITER_FOREACH(n, mu->nicks.head)
	{
		mn = n->data;
		snprintf(buf, BUFSIZE, "%s:%s", "private:usercloak", mn->nick);
		metadata_delete(mu, buf);
	}
	if (host != NULL)
		metadata_add(mu, "private:usercloak", host);
	else
		metadata_delete(mu, "private:usercloak");
}

static void write_hsofferdb(database_handle_t *db)
{
	mowgli_node_t *n;

	MOWGLI_ITER_FOREACH(n, hs_offeredlist.head)
	{
		hsoffered_t *l = n->data;

		db_start_row(db, "HO");

		if (l->group != NULL)
			db_write_word(db, l->group->name);

		db_write_word(db, l->vhost);
		db_write_time(db, l->vhost_ts);
		db_write_word(db, l->creator);
		db_commit_row(db);
	}

}

static void db_h_ho(database_handle_t *db, const char *type)
{
	hsoffered_t *l;
	const char *buf;
	time_t vhost_ts;
	const char *creator;
	myentity_t *mt = NULL;

	buf = db_sread_word(db);
	if (*buf == '!')
	{
		mt = myentity_find(buf);
		buf = db_sread_word(db);
	}

	vhost_ts = db_sread_time(db);
	creator = db_sread_word(db);

	l = smalloc(sizeof(hsoffered_t));
	l->group = mt;
	l->vhost = sstrdup(buf);
	l->vhost_ts = vhost_ts;
	l->creator = sstrdup(creator);

	mowgli_node_add(l, mowgli_node_create(), &hs_offeredlist);
}

/* OFFER <host> */
static void hs_cmd_offer(sourceinfo_t *si, int parc, char *parv[])
{
	char *group = parv[0];
	char *host;
	hsoffered_t *l;

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
		command_fail(si, fault_noprivs, _("You are not logged in."));
		return;
	}

	l = smalloc(sizeof(hsoffered_t));

	if (group != NULL)
		l->group = myentity_find(group);

	l->vhost = sstrdup(host);
	l->vhost_ts = CURRTIME;;
	l->creator = sstrdup(entity(si->smu)->name);

	mowgli_node_add(l, mowgli_node_create(), &hs_offeredlist);

	command_success_nodata(si, _("You have offered vhost \2%s\2."), host);
	logcommand(si, CMDLOG_ADMIN, "OFFER: \2%s\2", host);

	return;
}

/* UNOFFER <vhost> */
static void hs_cmd_unoffer(sourceinfo_t *si, int parc, char *parv[])
{
	char *host = parv[0];
	hsoffered_t *l;
	mowgli_node_t *n;

	if (!host)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "UNOFFER");
		command_fail(si, fault_needmoreparams, _("Syntax: UNOFFER <vhost>"));
		return;
	}

	MOWGLI_ITER_FOREACH(n, hs_offeredlist.head)
	{
		l = n->data;
		if (!irccasecmp(l->vhost, host))
		{
			logcommand(si, CMDLOG_ADMIN, "UNOFFER: \2%s\2", host);

			mowgli_node_delete(n, &hs_offeredlist);

			free(l->vhost);
			free(l->creator);
			free(l);

			command_success_nodata(si, _("You have unoffered vhost \2%s\2."), host);
			return;
		}
	}
	command_success_nodata(si, _("vhost \2%s\2 not found in vhost offer database."), host);
}

static bool myuser_is_in_group(myuser_t *mu, myentity_t *mt)
{
	mygroup_t *mg;
	mowgli_node_t *n;

	return_val_if_fail(mu != NULL, false);
	return_val_if_fail(mt != NULL, false);
	return_val_if_fail((mg = group(mt)) != NULL, false);

	MOWGLI_ITER_FOREACH(n, mg->acs.head)
	{
		groupacs_t *ga = n->data;

		if (ga->mu == mu && ga->flags & GA_VHOST)
			return true;
	}

	return false;
}

/* TAKE <vhost> */
static void hs_cmd_take(sourceinfo_t *si, int parc, char *parv[])
{
	char *host = parv[0];
	hsoffered_t *l;
	mowgli_node_t *n;

	if (!host)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "TAKE");
		command_fail(si, fault_needmoreparams, _("Syntax: TAKE <vhost>"));
		return;
	}

	if (si->smu == NULL)
	{
		command_fail(si, fault_noprivs, _("You are not logged in."));
		return;
	}

	MOWGLI_ITER_FOREACH(n, hs_offeredlist.head)
	{
		l = n->data;

		if (l->group != NULL && !myuser_is_in_group(si->smu, l->group))
			continue;

		if (!irccasecmp(l->vhost, host))
		{
			if (strstr(host, "$account"))
				replace(host, BUFSIZE, "$account", entity(si->smu)->name);

			if (!check_vhost_validity(si, host))
				return;

			logcommand(si, CMDLOG_GET, "TAKE: \2%s\2 for \2%s\2", host, entity(si->smu)->name);

			command_success_nodata(si, _("You have taken vhost \2%s\2."), host);
			hs_sethost_all(si->smu, host);
			do_sethost_all(si->smu, host);
			
			return;
		}
	}

	command_success_nodata(si, _("vhost \2%s\2 not found in vhost offer database."), host);
}

/* OFFERLIST */
static void hs_cmd_offerlist(sourceinfo_t *si, int parc, char *parv[])
{
	hsoffered_t *l;
	mowgli_node_t *n;
	char buf[BUFSIZE];
	struct tm tm;

	MOWGLI_ITER_FOREACH(n, hs_offeredlist.head)
	{
		l = n->data;

		if (l->group != NULL && !myuser_is_in_group(si->smu, l->group) && !has_priv(si, PRIV_GROUP_ADMIN))
			continue;

		tm = *localtime(&l->vhost_ts);

		strftime(buf, BUFSIZE, "%b %d %T %Y %Z", &tm);

		if(l->group != NULL)
			command_success_nodata(si, "vhost:\2%s\2, group:\2%s\2 creator:\2%s\2 (%s)",
						l->vhost, entity(l->group)->name, l->creator, buf);
		else
			command_success_nodata(si, "vhost:\2%s\2, creator:\2%s\2 (%s)",
						l->vhost, l->creator, buf);
	}
	command_success_nodata(si, "End of list.");
	logcommand(si, CMDLOG_GET, "OFFERLIST");
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
