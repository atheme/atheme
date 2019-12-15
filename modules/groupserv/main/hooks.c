/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2010 Atheme Project (http://atheme.org/)
 *
 * groupserv - group services.
 */

#include <atheme.h>
#include "groupserv_main.h"

static mowgli_eventloop_timer_t *mygroup_expire_timer = NULL;

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
user_info_hook(struct hook_user_req *req)
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
sasl_may_impersonate_hook(struct hook_sasl_may_impersonate *req)
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

	command_success_nodata(si, _("Maximum number of groups one user can own: %u"), gs_config.maxgroups);
	command_success_nodata(si, _("Maximum number of ACL entries allowed for one group: %u"), gs_config.maxgroupacs);
	command_success_nodata(si, _("Are open groups allowed: %s"), gs_config.enable_open_groups ? _("Yes") : _("No"));
	command_success_nodata(si, _("Default joinflags for open groups: %s"), gs_config.join_flags);
}

void
gs_hooks_init(void)
{
	mygroup_expire_timer = mowgli_timer_add(base_eventloop, "mygroup_expire", mygroup_expire, NULL, SECONDS_PER_HOUR);

	hook_add_user_info(user_info_hook);
	hook_add_myuser_delete(myuser_delete_hook);
	hook_add_operserv_info(osinfo_hook);
	hook_add_sasl_may_impersonate(sasl_may_impersonate_hook);
}

void
gs_hooks_deinit(void)
{
	mowgli_timer_destroy(base_eventloop, mygroup_expire_timer);

	hook_del_user_info(user_info_hook);
	hook_del_myuser_delete(myuser_delete_hook);
	hook_del_operserv_info(osinfo_hook);
	hook_del_sasl_may_impersonate(sasl_may_impersonate_hook);
}
