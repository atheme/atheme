/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2010 Atheme Project (http://atheme.org/)
 * Copyright (C) 2018-2019 Atheme Development Group (https://atheme.github.io/)
 */

#include "atheme.h"
#include "groupserv.h"

#define GDBV_VERSION            4

#define METADATA_PERSIST_NAME   "atheme.groupserv.main.persist"
#define METADATA_PERSIST_VERS   1

struct groupserv_persist_record
{
	mowgli_heap_t * groupacs_heap;
	mowgli_heap_t * mygroup_heap;
	unsigned int    version;
};

static mowgli_heap_t *groupacs_heap = NULL;
static mowgli_heap_t *mygroup_heap = NULL;
static struct service *groupsvs = NULL;

static mowgli_eventloop_timer_t *mygroup_expire_timer = NULL;
static struct groupserv_config gs_config;

static unsigned int loading_gdbv = -1;
static unsigned int their_ga_all = 0;

static mowgli_list_t *
myentity_get_membership_list(struct myentity *const restrict mt)
{
	return_null_if_fail(mt != NULL);

	mowgli_list_t *const elist = privatedata_get(mt, "groupserv:membership");

	if (elist)
		return elist;

	mowgli_list_t *const nlist = mowgli_list_create();

	return_null_if_fail(nlist != NULL);

	(void) privatedata_set(mt, "groupserv:membership", nlist);

	return nlist;
}

static void
groupacs_destroy(void *const restrict ga)
{
	return_if_fail(ga != NULL);

	(void) metadata_delete_all(ga);
	(void) mowgli_heap_free(groupacs_heap, ga);
}

static struct groupacs *
groupacs_add(struct mygroup *mg, struct myentity *mt, unsigned int flags)
{
	struct groupacs *ga;

	return_val_if_fail(mg != NULL, NULL);
	return_val_if_fail(mt != NULL, NULL);

	ga = mowgli_heap_alloc(groupacs_heap);
	atheme_object_init(atheme_object(ga), NULL, &groupacs_destroy);

	ga->mg = mg;
	ga->mt = mt;
	ga->flags = flags;

	mowgli_node_add(ga, &ga->gnode, &mg->acs);
	mowgli_node_add(ga, &ga->unode, myentity_get_membership_list(mt));

	return ga;
}

static struct groupacs *
groupacs_find(struct mygroup *mg, struct myentity *mt, unsigned int flags, bool allow_recurse)
{
	mowgli_node_t *n;
	struct groupacs *out = NULL;

	return_val_if_fail(mg != NULL, NULL);
	return_val_if_fail(mt != NULL, NULL);

	mg->visited = true;

	MOWGLI_ITER_FOREACH(n, mg->acs.head)
	{
		struct groupacs *ga = n->data;

		if (out != NULL)
			break;

		if (isgroup(ga->mt) && allow_recurse && !(group(ga->mt)->visited))
		{
			struct groupacs *ga2;

			ga2 = groupacs_find(group(ga->mt), mt, flags, allow_recurse);

			if (ga2 != NULL)
				out = ga;
		}
		else
		{
			if (flags)
			{
				if (ga->mt == mt && ga->mg == mg && (ga->flags & flags))
					out = ga;
			}
			else if (ga->mt == mt && ga->mg == mg)
				out = ga;
		}
	}

	mg->visited = false;

	return out;
}

static void
groupacs_delete(struct mygroup *mg, struct myentity *mt)
{
	struct groupacs *ga;

	ga = groupacs_find(mg, mt, 0, false);
	if (ga != NULL)
	{
		mowgli_node_delete(&ga->gnode, &mg->acs);
		mowgli_node_delete(&ga->unode, myentity_get_membership_list(mt));
		atheme_object_unref(ga);
	}
}

static unsigned int
groupacs_sourceinfo_flags(struct mygroup *mg, struct sourceinfo *si)
{
	struct groupacs *ga;

	ga = groupacs_find(mg, entity(si->smu), 0, true);
	if (ga == NULL)
		return 0;

	return ga->flags;
}

static bool
groupacs_sourceinfo_has_flag(struct mygroup *mg, struct sourceinfo *si, unsigned int flag)
{
	return groupacs_find(mg, entity(si->smu), flag, true) != NULL;
}

static unsigned int
gs_flags_parser(char *flagstring, bool allow_minus, unsigned int flags)
{
	char *c;
	unsigned int dir = 0;
	unsigned int flag;
	unsigned char n;

	c = flagstring;
	while (*c)
	{
		flag = 0;
		n = 0;
		switch(*c)
		{
		case '+':
			dir = 0;
			break;
		case '-':
			if (allow_minus)
				dir = 1;
			break;
		case '*':
			if (dir)
				flags = 0;
			else
			{
				// preserve existing flags except GA_BAN
				flags |= GA_ALL;
				flags &= ~GA_BAN;
			}
			break;
		default:
			while (ga_flags[n].ch != 0 && flag == 0)
			{
				if (ga_flags[n].ch == *c)
					flag = ga_flags[n].value;
				else
					n++;
			}
			if (flag == 0)
				break;
			if (dir)
				flags &= ~flag;
			else
				flags |= flag;
			break;
		}

		c++;
	}

	return flags;
}

static bool
mygroup_allow_foundership(struct myentity *mt)
{
	return true;
}

static bool
mygroup_can_register_channel(struct myentity *mt)
{
	struct mygroup *mg;

	mg = group(mt);

	return_val_if_fail(mg != NULL, false);

	if (mg->flags & MG_REGNOLIMIT)
		return true;

	return false;
}

static struct chanacs *
mygroup_chanacs_match_entity(struct chanacs *ca, struct myentity *mt)
{
	struct mygroup *mg;

	mg = group(ca->entity);

	return_val_if_fail(mg != NULL, NULL);

	if (!isuser(mt))
		return NULL;

	return groupacs_find(mg, mt, GA_CHANACS, true) != NULL ? ca : NULL;
}

static void
mygroup_rename(struct mygroup *mg, const char *name)
{
	stringref newname;
	char nb[GROUPLEN + 1];

	return_if_fail(mg != NULL);
	return_if_fail(name != NULL);
	return_if_fail(strlen(name) < sizeof nb);

	mowgli_strlcpy(nb, entity(mg)->name, sizeof nb);
	newname = strshare_get(name);

	myentity_del(entity(mg));

	strshare_unref(entity(mg)->name);
	entity(mg)->name = newname;

	myentity_put(entity(mg));
}

static void
mygroup_set_chanacs_validator(struct myentity *mt)
{
	static const struct entity_chanacs_validation_vtable mygroup_chanacs_validate = {
		.allow_foundership      = &mygroup_allow_foundership,
		.can_register_channel   = &mygroup_can_register_channel,
		.match_entity           = &mygroup_chanacs_match_entity,
	};

	mt->chanacs_validate = &mygroup_chanacs_validate;
}

static unsigned int
mygroup_count_flag(struct mygroup *mg, unsigned int flag)
{
	mowgli_node_t *n;
	unsigned int count = 0;

	return_val_if_fail(mg != NULL, 0);

	/* optimization: if flags = 0, then that means select everyone, so just
	 * return the list length.
	 */
	if (flag == 0)
		return MOWGLI_LIST_LENGTH(&mg->acs);

	MOWGLI_ITER_FOREACH(n, mg->acs.head)
	{
		struct groupacs *ga = n->data;

		if (ga->flags & flag)
			count++;
	}

	return count;
}

static void
mygroup_delete(struct mygroup *mg)
{
	mowgli_node_t *n, *tn;

	myentity_del(entity(mg));

	MOWGLI_ITER_FOREACH_SAFE(n, tn, mg->acs.head)
	{
		struct groupacs *ga = n->data;

		mowgli_node_delete(&ga->gnode, &mg->acs);
		mowgli_node_delete(&ga->unode, myentity_get_membership_list(ga->mt));
		atheme_object_unref(ga);
	}

	metadata_delete_all(mg);
	strshare_unref(entity(mg)->name);
	mowgli_heap_free(mygroup_heap, mg);
}

static struct mygroup *
mygroup_find(const char *name)
{
	struct myentity *mg = myentity_find(name);

	if (mg == NULL)
		return NULL;

	if (!isgroup(mg))
		return NULL;

	return group(mg);
}

static const char *
mygroup_founder_names(struct mygroup *mg)
{
        mowgli_node_t *n;
        struct groupacs *ga;
        static char names[512];

        names[0] = '\0';
        MOWGLI_ITER_FOREACH(n, mg->acs.head)
        {
                ga = n->data;
                if (ga->mt != NULL && ga->flags & GA_FOUNDER)
                {
                        if (names[0] != '\0')
                                mowgli_strlcat(names, ", ", sizeof names);
                        mowgli_strlcat(names, ga->mt->name, sizeof names);
                }
        }
        return names;
}

static unsigned int
myentity_count_group_flag(struct myentity *mt, unsigned int flagset)
{
	mowgli_list_t *l;
	mowgli_node_t *n;
	unsigned int count = 0;

	l = myentity_get_membership_list(mt);
	MOWGLI_ITER_FOREACH(n, l->head)
	{
		struct groupacs *ga = n->data;

		if (ga->mt == mt && ga->flags & flagset)
			count++;
	}

	return count;
}

static void
remove_group_chanacs(struct mygroup *mg)
{
	struct chanacs *ca;
	struct mychan *mc;
	struct myuser *successor;
	mowgli_node_t *n, *tn;

	// kill all their channels and chanacs
	MOWGLI_ITER_FOREACH_SAFE(n, tn, entity(mg)->chanacs.head)
	{
		ca = n->data;
		mc = ca->mychan;

		// attempt succession
		if (ca->level & CA_FOUNDER && mychan_num_founders(mc) == 1 && (successor = mychan_pick_successor(mc)) != NULL)
		{
			slog(LG_INFO, _("SUCCESSION: \2%s\2 to \2%s\2 from \2%s\2"), mc->name, entity(successor)->name, entity(mg)->name);
			slog(LG_VERBOSE, "myuser_delete(): giving channel %s to %s (unused %lds, founder %s, chanacs %zu)",
					mc->name, entity(successor)->name,
					(long)(CURRTIME - mc->used),
					entity(mg)->name,
					MOWGLI_LIST_LENGTH(&mc->chanacs));
			if (chansvs.me != NULL)
				verbose(mc, "Foundership changed to \2%s\2 because \2%s\2 was dropped.", entity(successor)->name, entity(mg)->name);

			chanacs_change_simple(mc, entity(successor), NULL, CA_FOUNDER_0, 0, NULL);
			if (chansvs.me != NULL)
				myuser_notice(chansvs.nick, successor, "You are now founder on \2%s\2 (as \2%s\2).", mc->name, entity(successor)->name);
			atheme_object_unref(ca);
		}
		// no successor found
		else if (ca->level & CA_FOUNDER && mychan_num_founders(mc) == 1)
		{
			slog(LG_REGISTER, _("DELETE: \2%s\2 from \2%s\2"), mc->name, entity(mg)->name);
			slog(LG_VERBOSE, "myuser_delete(): deleting channel %s (unused %lds, founder %s, chanacs %zu)",
					mc->name, (long)(CURRTIME - mc->used),
					entity(mg)->name,
					MOWGLI_LIST_LENGTH(&mc->chanacs));

			hook_call_channel_drop(mc);
			if (mc->chan != NULL && !(mc->chan->flags & CHAN_LOG))
				part(mc->name, chansvs.nick);
			atheme_object_unref(mc);
		}
		else // not founder
			atheme_object_unref(ca);
	}
}

static void
mygroup_expire(void *unused)
{
	struct myentity *mt;
	struct myentity_iteration_state state;

	MYENTITY_FOREACH_T(mt, &state, ENT_GROUP)
	{
		struct mygroup *mg = group(mt);

		continue_if_fail(mt != NULL);
		continue_if_fail(mg != NULL);

		if (!mygroup_count_flag(mg, GA_FOUNDER))
		{
			remove_group_chanacs(mg);
			atheme_object_unref(mg);
		}
	}
}

static void
grant_channel_access_hook(struct user *u)
{
	mowgli_node_t *n, *tn;
	mowgli_list_t *l;

	return_if_fail(u->myuser != NULL);

	l = myentity_get_membership_list(entity(u->myuser));

	MOWGLI_ITER_FOREACH_SAFE(n, tn, l->head)
	{
		struct groupacs *ga = n->data;

		if (!(ga->flags & GA_CHANACS))
			continue;

		MOWGLI_ITER_FOREACH(n, entity(ga->mg)->chanacs.head)
		{
			struct chanacs *ca;
			struct chanuser *cu;

			ca = (struct chanacs *)n->data;

			if (ca->mychan->chan == NULL)
				continue;

			cu = chanuser_find(ca->mychan->chan, u);
			if (cu && chansvs.me != NULL)
			{
				if (ca->level & CA_AKICK && !(ca->level & CA_EXEMPT))
				{
					// Stay on channel if this would empty it -- jilles
					if (ca->mychan->chan->nummembers - ca->mychan->chan->numsvcmembers == 1)
					{
						ca->mychan->flags |= MC_INHABIT;
						if (ca->mychan->chan->numsvcmembers == 0)
							join(cu->chan->name, chansvs.nick);
					}
					ban(chansvs.me->me, ca->mychan->chan, u);
					remove_ban_exceptions(chansvs.me->me, ca->mychan->chan, u);
					kick(chansvs.me->me, ca->mychan->chan, u, "User is banned from this channel");
					continue;
				}

				if (ca->level & CA_USEDUPDATE)
					ca->mychan->used = CURRTIME;

				if (ca->mychan->flags & MC_NOOP || u->myuser->flags & MU_NOOP)
					continue;

				if (ircd->uses_owner && !(cu->modes & ircd->owner_mode) && ca->level & CA_AUTOOP && ca->level & CA_USEOWNER)
				{
					modestack_mode_param(chansvs.nick, ca->mychan->chan, MTYPE_ADD, ircd->owner_mchar[1], CLIENT_NAME(u));
					cu->modes |= ircd->owner_mode;
				}

				if (ircd->uses_protect && !(cu->modes & ircd->protect_mode) && !(ircd->uses_owner && cu->modes & ircd->owner_mode) && ca->level & CA_AUTOOP && ca->level & CA_USEPROTECT)
				{
					modestack_mode_param(chansvs.nick, ca->mychan->chan, MTYPE_ADD, ircd->protect_mchar[1], CLIENT_NAME(u));
					cu->modes |= ircd->protect_mode;
				}

				if (!(cu->modes & CSTATUS_OP) && ca->level & CA_AUTOOP)
				{
					modestack_mode_param(chansvs.nick, ca->mychan->chan, MTYPE_ADD, 'o', CLIENT_NAME(u));
					cu->modes |= CSTATUS_OP;
				}

				if (ircd->uses_halfops && !(cu->modes & (CSTATUS_OP | ircd->halfops_mode)) && ca->level & CA_AUTOHALFOP)
				{
					modestack_mode_param(chansvs.nick, ca->mychan->chan, MTYPE_ADD, 'h', CLIENT_NAME(u));
					cu->modes |= ircd->halfops_mode;
				}

				if (!(cu->modes & (CSTATUS_OP | ircd->halfops_mode | CSTATUS_VOICE)) && ca->level & CA_AUTOVOICE)
				{
					modestack_mode_param(chansvs.nick, ca->mychan->chan, MTYPE_ADD, 'v', CLIENT_NAME(u));
					cu->modes |= CSTATUS_VOICE;
				}
			}
		}
	}
}

static void
user_info_hook(hook_user_req_t *req)
{
	static char buf[BUFSIZE];
	mowgli_node_t *n;
	mowgli_list_t *l;

	*buf = 0;

	l = myentity_get_membership_list(entity(req->mu));

	MOWGLI_ITER_FOREACH(n, l->head)
	{
		struct groupacs *ga = n->data;

		if (ga->flags & GA_BAN)
			continue;

		if ((ga->mg->flags & MG_PUBLIC) || (req->si->smu == req->mu || has_priv(req->si, PRIV_GROUP_AUSPEX)))
		{
			if (*buf != 0)
				mowgli_strlcat(buf, ", ", BUFSIZE);

			mowgli_strlcat(buf, entity(ga->mg)->name, BUFSIZE);
		}
	}

	if (*buf != 0)
		command_success_nodata(req->si, _("Groups     : %s"), buf);
}

static void
sasl_may_impersonate_hook(hook_sasl_may_impersonate_t *req)
{
	char priv[BUFSIZE];
	mowgli_list_t *l;
	mowgli_node_t *n;

	// if the request is already granted, don't bother doing any of this.
	if (req->allowed)
		return;

	l = myentity_get_membership_list(entity(req->target_mu));

	MOWGLI_ITER_FOREACH(n, l->head)
	{
		struct groupacs *ga = n->data;

		snprintf(priv, sizeof(priv), PRIV_IMPERSONATE_ENTITY_FMT, entity(ga->mg)->name);

		if (has_priv_myuser(req->source_mu, priv))
		{
			req->allowed = true;
			return;
		}
	}
}

static void
myuser_delete_hook(struct myuser *mu)
{
	mowgli_node_t *n, *tn;
	mowgli_list_t *l;

	l = myentity_get_membership_list(entity(mu));

	MOWGLI_ITER_FOREACH_SAFE(n, tn, l->head)
	{
		struct groupacs *ga = n->data;

		groupacs_delete(ga->mg, ga->mt);
	}

	mowgli_list_free(l);
}

static void
osinfo_hook(struct sourceinfo *si)
{
	return_if_fail(si != NULL);

	command_success_nodata(si, "Maximum number of groups one user can own: %u", gs_config.maxgroups);
	command_success_nodata(si, "Maximum number of ACL entries allowed for one group: %u", gs_config.maxgroupacs);
	command_success_nodata(si, "Are open groups allowed: %s", gs_config.enable_open_groups ? "Yes" : "No");
	command_success_nodata(si, "Default joinflags for open groups: %s", gs_config.join_flags);
}

static struct mygroup *
mygroup_add_id(const char *id, const char *name)
{
	struct mygroup *mg;

	mg = mowgli_heap_alloc(mygroup_heap);
	atheme_object_init(atheme_object(mg), NULL, (atheme_object_destructor_fn) mygroup_delete);

	entity(mg)->type = ENT_GROUP;

	if (id)
	{
		if (!myentity_find_uid(id))
			mowgli_strlcpy(entity(mg)->id, id, sizeof(entity(mg)->id));
		else
			entity(mg)->id[0] = '\0';
	}
	else
		entity(mg)->id[0] = '\0';

	entity(mg)->name = strshare_get(name);
	myentity_put(entity(mg));

	mygroup_set_chanacs_validator(entity(mg));

	mg->regtime = CURRTIME;

	return mg;
}

static struct mygroup *
mygroup_add(const char *name)
{
	return mygroup_add_id(NULL, name);
}

static void
write_groupdb_hook(struct database_handle *db)
{
	struct myentity *mt;
	struct myentity_iteration_state state;
	mowgli_patricia_iteration_state_t state2;
	struct metadata *md;

	db_start_row(db, "GDBV");
	db_write_uint(db, GDBV_VERSION);
	db_commit_row(db);

	db_start_row(db, "GFA");
	db_write_word(db, gflags_tostr(ga_flags, GA_ALL));
	db_commit_row(db);

	MYENTITY_FOREACH_T(mt, &state, ENT_GROUP)
	{
		continue_if_fail(mt != NULL);
		struct mygroup *mg = group(mt);
		continue_if_fail(mg != NULL);

		char *mgflags = gflags_tostr(mg_flags, mg->flags);

		db_start_row(db, "GRP");
		db_write_word(db, entity(mg)->id);
		db_write_word(db, entity(mg)->name);
		db_write_time(db, mg->regtime);
		db_write_word(db, mgflags);
		db_commit_row(db);

		if (atheme_object(mg)->metadata)
		{
			MOWGLI_PATRICIA_FOREACH(md, &state2, atheme_object(mg)->metadata)
			{
				db_start_row(db, "MDG");
				db_write_word(db, entity(mg)->name);
				db_write_word(db, md->name);
				db_write_str(db, md->value);
				db_commit_row(db);
			}
		}
	}

	MYENTITY_FOREACH_T(mt, &state, ENT_GROUP)
	{
		mowgli_node_t *n;

		continue_if_fail(mt != NULL);
		struct mygroup *mg = group(mt);
		continue_if_fail(mg != NULL);

		MOWGLI_ITER_FOREACH(n, mg->acs.head)
		{
			struct groupacs *ga = n->data;
			char *flags = gflags_tostr(ga_flags, ga->flags);

			db_start_row(db, "GACL");
			db_write_word(db, entity(mg)->name);
			db_write_word(db, ga->mt->name);
			db_write_word(db, flags);
			db_commit_row(db);
		}
	}
}

static void
db_h_gdbv(struct database_handle *db, const char *type)
{
	loading_gdbv = db_sread_uint(db);
	slog(LG_INFO, "groupserv: opensex data schema version is %d.", loading_gdbv);

	their_ga_all = GA_ALL_OLD;
}

static void
db_h_gfa(struct database_handle *db, const char *type)
{
	const char *flags = db_sread_word(db);

	gflags_fromstr(ga_flags, flags, &their_ga_all);
	if (their_ga_all & ~GA_ALL)
	{
		slog(LG_ERROR, "db-h-gfa: losing flags %s from file", gflags_tostr(ga_flags, their_ga_all & ~GA_ALL));
	}
	if (~their_ga_all & GA_ALL)
	{
		slog(LG_ERROR, "db-h-gfa: making up flags %s not present in file", gflags_tostr(ga_flags, ~their_ga_all & GA_ALL));
	}
}

static void
db_h_grp(struct database_handle *db, const char *type)
{
	struct mygroup *mg;
	const char *uid = NULL;
	const char *name;
	time_t regtime;
	const char *flagset;

	if (loading_gdbv >= 4)
		uid = db_sread_word(db);

	name = db_sread_word(db);

	if (mygroup_find(name))
	{
		slog(LG_INFO, "db-h-grp: line %d: skipping duplicate group %s", db->line, name);
		return;
	}
	if (uid && myentity_find_uid(uid))
	{
		slog(LG_INFO, "db-h-grp: line %d: skipping group %s with duplicate UID %s", db->line, name, uid);
		return;
	}

	regtime = db_sread_time(db);

	mg = mygroup_add_id(uid, name);
	mg->regtime = regtime;

	if (loading_gdbv >= 3)
	{
		flagset = db_sread_word(db);

		if (!gflags_fromstr(mg_flags, flagset, &mg->flags))
			slog(LG_INFO, "db-h-grp: line %d: confused by flags: %s", db->line, flagset);
	}
}

static void
db_h_gacl(struct database_handle *db, const char *type)
{
	struct mygroup *mg;
	struct myentity *mt;
	unsigned int flags = GA_ALL;	// GDBV 1 entires had full access

	const char *name = db_sread_word(db);
	const char *entity = db_sread_word(db);
	const char *flagset;

	mg = mygroup_find(name);
	mt = myentity_find(entity);

	if (mg == NULL)
	{
		slog(LG_INFO, "db-h-gacl: line %d: groupacs for nonexistent group %s", db->line, name);
		return;
	}

	if (mt == NULL)
	{
		slog(LG_INFO, "db-h-gacl: line %d: groupacs for nonexistent entity %s", db->line, entity);
		return;
	}

	if (loading_gdbv >= 2)
	{
		flagset = db_sread_word(db);

		if (!gflags_fromstr(ga_flags, flagset, &flags))
			slog(LG_INFO, "db-h-gacl: line %d: confused by flags: %s", db->line, flagset);

		/* ACL view permission was added, so make up the permission (#279), but only if the database
		 * is from atheme 7.1 or earlier. --kaniini
		 */
		if (!(their_ga_all & GA_ACLVIEW) && ((flags & GA_ALL_OLD) == their_ga_all))
			flags |= GA_ACLVIEW;
	}

	groupacs_add(mg, mt, flags);
}

static void
db_h_mdg(struct database_handle *db, const char *type)
{
	const char *name = db_sread_word(db);
	const char *prop = db_sread_word(db);
	const char *value = db_sread_str(db);
	void *obj = NULL;

	obj = mygroup_find(name);

	if (obj == NULL)
	{
		slog(LG_INFO, "db-h-mdg: attempting to add %s property to non-existant object %s",
		     prop, name);
		return;
	}

	metadata_add(obj, prop, value);
}

static void
mod_init(struct module *const restrict m)
{
	if (! (groupsvs = service_add("groupserv", NULL)))
	{
		(void) slog(LG_ERROR, "%s: service_add() failed", m->name);
		goto fail;
	}
	if (! (mygroup_expire_timer = mowgli_timer_add(base_eventloop, "mygroup_expire", &mygroup_expire, NULL, 3600)))
	{
		(void) slog(LG_ERROR, "%s: mowgli_timer_add() failed", m->name);
		goto fail;
	}

	struct groupserv_persist_record *const rec = mowgli_global_storage_get(METADATA_PERSIST_NAME);

	if (rec)
	{
		if (rec->version != METADATA_PERSIST_VERS)
		{
			(void) slog(LG_ERROR, "%s: persist version unrecognised!", m->name);
			goto fail;
		}

		groupacs_heap = rec->groupacs_heap;
		mygroup_heap = rec->mygroup_heap;

		(void) mowgli_global_storage_free(METADATA_PERSIST_NAME);
		(void) sfree(rec);

		struct myentity *grp;
		struct myentity_iteration_state iter;

		MYENTITY_FOREACH_T(grp, &iter, ENT_GROUP)
			(void) mygroup_set_chanacs_validator(grp);
	}
	else
	{
		if (! (groupacs_heap = mowgli_heap_create(sizeof(struct groupacs), HEAP_GROUPACS, BH_NOW)))
		{
			(void) slog(LG_ERROR, "%s: mowgli_heap_create() failed", m->name);
			goto fail;
		}
		if (! (mygroup_heap = mowgli_heap_create(sizeof(struct mygroup), HEAP_GROUP, BH_NOW)))
		{
			(void) slog(LG_ERROR, "%s: mowgli_heap_create() failed", m->name);
			goto fail;
		}
	}

	(void) add_bool_conf_item("ENABLE_OPEN_GROUPS", &groupsvs->conf_table, 0, &gs_config.enable_open_groups, false);
	(void) add_dupstr_conf_item("JOIN_FLAGS", &groupsvs->conf_table, 0, &gs_config.join_flags, "+");
	(void) add_uint_conf_item("MAXGROUPS", &groupsvs->conf_table, 0, &gs_config.maxgroups, 0, 65535, 5);
	(void) add_uint_conf_item("MAXGROUPACS", &groupsvs->conf_table, 0, &gs_config.maxgroupacs, 0, 65535, 0);

	(void) db_register_type_handler("GDBV", &db_h_gdbv);
	(void) db_register_type_handler("GFA", &db_h_gfa);
	(void) db_register_type_handler("GRP", &db_h_grp);
	(void) db_register_type_handler("MDG", &db_h_mdg);
	(void) db_register_type_handler("GACL", &db_h_gacl);

	(void) hook_add_event("grant_channel_access");
	(void) hook_add_grant_channel_access(&grant_channel_access_hook);

	(void) hook_add_event("sasl_may_impersonate");
	(void) hook_add_sasl_may_impersonate(&sasl_may_impersonate_hook);

	(void) hook_add_event("db_write_pre_ca");
	(void) hook_add_db_write_pre_ca(&write_groupdb_hook);

	(void) hook_add_event("myuser_delete");
	(void) hook_add_myuser_delete(&myuser_delete_hook);

	(void) hook_add_event("operserv_info");
	(void) hook_add_operserv_info(&osinfo_hook);

	(void) hook_add_event("user_info");
	(void) hook_add_user_info(&user_info_hook);

	m->mflags |= MODFLAG_DBHANDLER;
	return;

fail:
	(void) slog(LG_ERROR, "%s: exiting to avoid data loss!", m->name);

	exit(EXIT_FAILURE);
}

static void
mod_deinit(const enum module_unload_intent intent)
{
	(void) service_delete(groupsvs);

	(void) mowgli_timer_destroy(base_eventloop, mygroup_expire_timer);

	(void) del_conf_item("MAXGROUPS", &groupsvs->conf_table);
	(void) del_conf_item("MAXGROUPACS", &groupsvs->conf_table);
	(void) del_conf_item("ENABLE_OPEN_GROUPS", &groupsvs->conf_table);
	(void) del_conf_item("JOIN_FLAGS", &groupsvs->conf_table);

	(void) db_unregister_type_handler("GDBV");
	(void) db_unregister_type_handler("GFA");
	(void) db_unregister_type_handler("GRP");
	(void) db_unregister_type_handler("MDG");
	(void) db_unregister_type_handler("GACL");

	(void) hook_del_grant_channel_access(&grant_channel_access_hook);
	(void) hook_del_sasl_may_impersonate(&sasl_may_impersonate_hook);
	(void) hook_del_db_write_pre_ca(&write_groupdb_hook);
	(void) hook_del_myuser_delete(&myuser_delete_hook);
	(void) hook_del_operserv_info(&osinfo_hook);
	(void) hook_del_user_info(&user_info_hook);

	switch (intent)
	{
		case MODULE_UNLOAD_INTENT_RELOAD:
		{
			struct groupserv_persist_record *const rec = smalloc(sizeof *rec);

			rec->groupacs_heap = groupacs_heap;
			rec->mygroup_heap = mygroup_heap;
			rec->version = METADATA_PERSIST_VERS;

			(void) mowgli_global_storage_put(METADATA_PERSIST_NAME, rec);
			break;
		}

		case MODULE_UNLOAD_INTENT_PERM:
			(void) mowgli_heap_destroy(groupacs_heap);
			(void) mowgli_heap_destroy(mygroup_heap);
			break;
	}
}

// Imported by all other GroupServ modules
extern const struct groupserv_core_symbols groupserv_core_symbols;
const struct groupserv_core_symbols groupserv_core_symbols = {
	.config                         = &gs_config,
	.groupsvs                       = &groupsvs,
	.groupacs_add                   = &groupacs_add,
	.groupacs_delete                = &groupacs_delete,
	.groupacs_find                  = &groupacs_find,
	.groupacs_sourceinfo_flags      = &groupacs_sourceinfo_flags,
	.groupacs_sourceinfo_has_flag   = &groupacs_sourceinfo_has_flag,
	.gs_flags_parser                = &gs_flags_parser,
	.mygroup_add_id                 = &mygroup_add_id,
	.mygroup_add                    = &mygroup_add,
	.mygroup_count_flag             = &mygroup_count_flag,
	.mygroup_find                   = &mygroup_find,
	.mygroup_founder_names          = &mygroup_founder_names,
	.mygroup_rename                 = &mygroup_rename,
	.myentity_count_group_flag      = &myentity_count_group_flag,
	.myentity_get_membership_list   = &myentity_get_membership_list,
	.remove_group_chanacs           = &remove_group_chanacs,
};

SIMPLE_DECLARE_MODULE_V1("groupserv/main", MODULE_UNLOAD_CAPABILITY_RELOAD_ONLY)
