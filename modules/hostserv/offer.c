/*
 * Copyright (c) 2005 William Pitcock <nenolod -at- nenolod.net>
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Allows opers to offer vhosts to users
 *
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"hostserv/offer", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.net>"
);

list_t *hs_cmdtree, *hs_helptree, *conf_hs_table;

static void hs_cmd_offer(sourceinfo_t *si, int parc, char *parv[]);
static void hs_cmd_unoffer(sourceinfo_t *si, int parc, char *parv[]);
static void hs_cmd_offerlist(sourceinfo_t *si, int parc, char *parv[]);
static void hs_cmd_take(sourceinfo_t *si, int parc, char *parv[]);

static void write_hsofferdb(database_handle_t *db);
static void db_h_ho(database_handle_t *db, const char *type);

command_t hs_offer = { "OFFER", N_("Sets vhosts available for users to take."), PRIV_USER_VHOST, 2, hs_cmd_offer };
command_t hs_unoffer = { "UNOFFER", N_("Removes a vhost from the list that users can take."), PRIV_USER_VHOST, 2, hs_cmd_unoffer };
command_t hs_offerlist = { "OFFERLIST", N_("Lists all available vhosts."), AC_NONE, 1, hs_cmd_offerlist };
command_t hs_take = { "TAKE", N_("Take an offered vhost for use."), AC_NONE, 2, hs_cmd_take };

struct hsoffered_ {
	char *vhost;
	time_t vhost_ts;
	char *creator;
};

typedef struct hsoffered_ hsoffered_t;

list_t hs_offeredlist;

void _modinit(module_t *m)
{
	if (!module_find_published("backend/opensex"))
	{
		slog(LG_INFO, "Module %s requires use of the OpenSEX database backend, refusing to load.", m->header->name);
		m->mflags = MODTYPE_FAIL;
		return;
	}

	MODULE_USE_SYMBOL(hs_cmdtree, "hostserv/main", "hs_cmdtree");
	MODULE_USE_SYMBOL(hs_helptree, "hostserv/main", "hs_helptree");
	MODULE_USE_SYMBOL(conf_hs_table, "hostserv/main", "conf_hs_table");

	hook_add_db_write(write_hsofferdb);
	db_register_type_handler("HO", db_h_ho);

 	command_add(&hs_offer, hs_cmdtree);
	command_add(&hs_unoffer, hs_cmdtree);
	command_add(&hs_offerlist, hs_cmdtree);
	command_add(&hs_take, hs_cmdtree);
	help_addentry(hs_helptree, "OFFER", "help/hostserv/offer", NULL);
	help_addentry(hs_helptree, "UNOFFER", "help/hostserv/unoffer", NULL);
	help_addentry(hs_helptree, "OFFERLIST", "help/hostserv/offerlist", NULL);
	help_addentry(hs_helptree, "TAKE", "help/hostserv/take", NULL);
}

void _moddeinit(void)
{
	hook_del_db_write(write_hsofferdb);
	db_unregister_type_handler("HO");

	command_delete(&hs_offer, hs_cmdtree);
	command_delete(&hs_unoffer, hs_cmdtree);
	command_delete(&hs_offerlist, hs_cmdtree);
	command_delete(&hs_take, hs_cmdtree);
	help_delentry(hs_helptree, "OFFER");
	help_delentry(hs_helptree, "UNOFFER");
	help_delentry(hs_helptree, "OFFERLIST");
	help_delentry(hs_helptree, "TAKE");
}

static void do_sethost(user_t *u, char *host)
{
	if (!strcmp(u->vhost, host ? host : u->host))
		return;
	strlcpy(u->vhost, host ? host : u->host, HOSTLEN);
	sethost_sts(hostsvs.me->me, u, u->vhost);
}

static void do_sethost_all(myuser_t *mu, char *host)
{
	node_t *n;
	user_t *u;

	LIST_FOREACH(n, mu->logins.head)
	{
		u = n->data;

		do_sethost(u, host);
	}
}

static void hs_sethost_all(myuser_t *mu, const char *host)
{
	node_t *n;
	mynick_t *mn;
	char buf[BUFSIZE];

	LIST_FOREACH(n, mu->nicks.head)
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
	node_t *n;

	LIST_FOREACH(n, hs_offeredlist.head)
	{
		hsoffered_t *l = n->data;

		db_start_row(db, "HO");
		db_write_word(db, l->vhost);
		db_write_time(db, l->vhost_ts);
		db_write_word(db, l->creator);
		db_commit_row(db);
	}

}

static void db_h_ho(database_handle_t *db, const char *type)
{
	const char *vhost = db_sread_word(db);
	time_t vhost_ts = db_sread_time(db);
	const char *creator = db_sread_word(db);

	hsoffered_t *l = smalloc(sizeof(hsoffered_t));
	l->vhost = sstrdup(vhost);
	l->vhost_ts = vhost_ts;
	l->creator = sstrdup(creator);
	node_add(l, node_create(), &hs_offeredlist);
}

/* OFFER <host> */
static void hs_cmd_offer(sourceinfo_t *si, int parc, char *parv[])
{
	char *host = parv[0];
	node_t *n;
	hsoffered_t *l;

	if (!host)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "OFFER");
		command_fail(si, fault_needmoreparams, _("Syntax: OFFER <vhost>"));
		return;
	}

	if (si->smu == NULL)
	{
		command_fail(si, fault_noprivs, _("You are not logged in."));
		return;
	}

	l = smalloc(sizeof(hsoffered_t));
	l->vhost = sstrdup(host);
	l->vhost_ts = CURRTIME;;
	l->creator = sstrdup(get_source_name(si));

	n = node_create();
	node_add(l, n, &hs_offeredlist);

	command_success_nodata(si, _("You have offered vhost \2%s\2."), host);
	logcommand(si, CMDLOG_ADMIN, "OFFER: \2%s\2", host);

	return;
}

/* UNOFFER <vhost> */
static void hs_cmd_unoffer(sourceinfo_t *si, int parc, char *parv[])
{
	char *host = parv[0];
	hsoffered_t *l;
	node_t *n;

	if (!host)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "UNOFFER");
		command_fail(si, fault_needmoreparams, _("Syntax: UNOFFER <vhost>"));
		return;
	}

	LIST_FOREACH(n, hs_offeredlist.head)
	{
		l = n->data;
		if (!irccasecmp(l->vhost, host))
		{
			logcommand(si, CMDLOG_ADMIN, "UNOFFER: \2%s\2", host);

			node_del(n, &hs_offeredlist);

			free(l->vhost);
			free(l->creator);
			free(l);

			command_success_nodata(si, _("You have unoffered vhost \2%s\2."), host);
			return;
		}
	}
	command_success_nodata(si, _("vhost \2%s\2 not found in vhost offer database."), host);
}

/* TAKE <vhost> */
static void hs_cmd_take(sourceinfo_t *si, int parc, char *parv[])
{
	char *host = parv[0];
	hsoffered_t *l;
	node_t *n;

	if (!host)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "TAKE");
		command_fail(si, fault_needmoreparams, _("Syntax: TAKE <vhost>"));
		return;
	}


	LIST_FOREACH(n, hs_offeredlist.head)
	{
		l = n->data;
		if (!irccasecmp(l->vhost, host))
		{
			if (strstr(host, "$account"))
				replace(host, BUFSIZE, "$account", si->smu->name);

			if (!check_vhost_validity(si, host))
				return;

			logcommand(si, CMDLOG_GET, "TAKE: \2%s\2 for \2%s\2", host, si->smu->name);

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
	node_t *n;
	int x = 0;
	char buf[BUFSIZE];
	struct tm tm;

	LIST_FOREACH(n, hs_offeredlist.head)
	{
		l = n->data;
		x++;

		tm = *localtime(&l->vhost_ts);

		strftime(buf, BUFSIZE, "%b %d %T %Y %Z", &tm);
			command_success_nodata(si, "#%d vhost:\2%s\2, creator:\2%s\2 (%s)",
				x, l->vhost, l->creator, buf);
	}
	command_success_nodata(si, "End of list.");
	logcommand(si, CMDLOG_GET, "OFFERLIST");
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
