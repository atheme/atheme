/* groupserv - group services.
 * Copyright (C) 2010 Atheme Development Group
 */

#include "groupserv_main.h"

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
		{
			remove_group_chanacs(mg);
			object_unref(mg);
		}
	}
}

static void grant_channel_access_hook(user_t *u)
{
	mowgli_node_t *n, *tn;
	mowgli_list_t *l;

	return_if_fail(u->myuser != NULL);

	l = myuser_get_membership_list(u->myuser);

	MOWGLI_ITER_FOREACH_SAFE(n, tn, l->head)
	{
		groupacs_t *ga = n->data;

		if (!(ga->flags & GA_CHANACS))
			continue;

		MOWGLI_ITER_FOREACH(n, entity(ga->mg)->chanacs.head)
		{
			chanacs_t *ca;
			chanuser_t *cu;

			ca = (chanacs_t *)n->data;

			if (ca->mychan->chan == NULL)
				continue;

			cu = chanuser_find(ca->mychan->chan, u);
			if (cu && chansvs.me != NULL)
			{
				if (ca->level & CA_AKICK && !(ca->level & CA_EXEMPT))
				{
					/* Stay on channel if this would empty it -- jilles */
					if (ca->mychan->chan->nummembers <= (ca->mychan->flags & MC_GUARD ? 2 : 1))
					{
						ca->mychan->flags |= MC_INHABIT;
						if (!(ca->mychan->flags & MC_GUARD))
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

static void user_info_hook(hook_user_req_t *req)
{
	static char buf[BUFSIZE];
	mowgli_node_t *n;
	mowgli_list_t *l;

	*buf = 0;

	l = myuser_get_membership_list(req->mu);

	MOWGLI_ITER_FOREACH(n, l->head)
	{
		groupacs_t *ga = n->data;

		if (groupacs_find(ga->mg, req->mu, GA_BAN) != NULL)
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

static void myuser_delete_hook(myuser_t *mu)
{
	mowgli_node_t *n, *tn;
	mowgli_list_t *l;

	l = myuser_get_membership_list(mu);

	MOWGLI_ITER_FOREACH_SAFE(n, tn, l->head)
	{
		groupacs_t *ga = n->data;

		groupacs_delete(ga->mg, ga->mu);
	}

	mowgli_list_free(l);
}

static void osinfo_hook(sourceinfo_t *si)
{
	return_if_fail(si != NULL);

	command_success_nodata(si, "Maximum number of groups one user can own: %u", gs_config.maxgroups);
	command_success_nodata(si, "Maximum number of ACL entries allowed for one group: %u", gs_config.maxgroupacs);
	command_success_nodata(si, "Are open groups allowed: %s", gs_config.enable_open_groups ? "Yes" : "No");
	command_success_nodata(si, "Default joinflags for open groups: %s", gs_config.join_flags);
}

static mowgli_eventloop_timer_t *mygroup_expire_timer = NULL;

void gs_hooks_init(void)
{
	mygroup_expire_timer = mowgli_timer_add(base_eventloop, "mygroup_expire", mygroup_expire, NULL, 3600);

	hook_add_event("myuser_delete");
	hook_add_event("user_info");
	hook_add_event("grant_channel_access");
	hook_add_event("operserv_info");

	hook_add_user_info(user_info_hook);
	hook_add_myuser_delete(myuser_delete_hook);
	hook_add_grant_channel_access(grant_channel_access_hook);
	hook_add_operserv_info(osinfo_hook);
}

void gs_hooks_deinit(void)
{
	mowgli_timer_destroy(base_eventloop, mygroup_expire_timer);

	hook_del_user_info(user_info_hook);
	hook_del_myuser_delete(myuser_delete_hook);
	hook_del_grant_channel_access(grant_channel_access_hook);
	hook_del_operserv_info(osinfo_hook);
}
