/* groupserv - group services.
 * Copyright (C) 2010 Atheme Development Group
 */

#include "groupserv.h"

static void user_info_hook(hook_user_req_t *req)
{
	static char buf[BUFSIZE];
	myentity_t *mt;
	myentity_iteration_state_t state;

	*buf = 0;

	/* XXX: we need a way to stash a list in myuser showing the list of
	 * groups we're in.  it would speed this up dramatically.  --nenolod
	 */
	MYENTITY_FOREACH_T(mt, &state, ENT_GROUP)
	{
		mygroup_t *mg;

		mg = group(mt);
		continue_if_fail(mg != NULL);

		if (groupacs_find(mg, req->mu, 0) == NULL)
			continue;

		if (*buf != 0)
			strlcat(buf, ", ", BUFSIZE);

		strlcat(buf, entity(mg)->name, BUFSIZE);
	}

	command_success_nodata(req->si, _("Groups     : %s"), buf);
}

static void myuser_delete_hook(myuser_t *mu)
{
	myentity_t *mt;
	myentity_iteration_state_t state;

	/* XXX: we need a way to stash a list in myuser showing the list of
	 * groups we're in.  it would speed this up dramatically.  --nenolod
	 */
	MYENTITY_FOREACH_T(mt, &state, ENT_GROUP)
	{
		mygroup_t *mg;

		mg = group(mt);
		continue_if_fail(mg != NULL);

		groupacs_delete(mg, mu);
	}
}

void gs_hooks_init(void)
{
	hook_add_user_info(user_info_hook);
	hook_add_myuser_delete(myuser_delete_hook);
}

void gs_hooks_deinit(void)
{
	hook_del_user_info(user_info_hook);
	hook_del_myuser_delete(myuser_delete_hook);
}
