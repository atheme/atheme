/* groupserv - group services.
 * Copyright (C) 2010 Atheme Development Group
 */

#include "groupserv.h"

static void mygroup_expire(void *unused)
{
	myentity_t *mt;
	myentity_iteration_state_t state;

	MYENTITY_FOREACH_T(mt, &state, ENT_GROUP)
	{
		mygroup_t *mg = group(mt);

		continue_if_fail(mt != NULL);
		continue_if_fail(mg != NULL);

		if (!mygroup_count_flag(mg, GA_FOUNDER))
			object_unref(mg);
	}
}

static void user_info_hook(hook_user_req_t *req)
{
	static char buf[BUFSIZE];
	node_t *n;
	list_t *l;

	*buf = 0;

	l = myuser_get_membership_list(req->mu);
		
	LIST_FOREACH(n, l->head)
	{
		groupacs_t *ga = n->data;

		if ((ga->mg->flags & MG_PUBLIC) || (req->si->smu == req->mu || has_priv(req->si, PRIV_GROUP_AUSPEX)))
		{
			if (*buf != 0)
				strlcat(buf, ", ", BUFSIZE);

			strlcat(buf, entity(ga->mg)->name, BUFSIZE);
		}
	}

	if (*buf != 0)
		command_success_nodata(req->si, _("Groups     : %s"), buf);
}

static void myuser_delete_hook(myuser_t *mu)
{
	node_t *n;
	list_t *l;

	l = myuser_get_membership_list(mu);

	LIST_FOREACH(n, l->head)
	{
		groupacs_t *ga = n->data;

		groupacs_delete(ga->mg, ga->mu);
	}

	list_free(l);
}

void gs_hooks_init(void)
{
	event_add("mygroup_expire", mygroup_expire, NULL, 3600);

	hook_add_event("myuser_delete");
	hook_add_user_info(user_info_hook);
	hook_add_myuser_delete(myuser_delete_hook);
}

void gs_hooks_deinit(void)
{
	event_delete(mygroup_expire, NULL);

	hook_del_user_info(user_info_hook);
	hook_del_myuser_delete(myuser_delete_hook);
}
