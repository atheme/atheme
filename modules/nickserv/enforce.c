/*
 * Copyright (c) 2005-2007 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the NickServ RELEASE/ENFORCE functions.
 *
 * This does nickserv enforcement on registered nicks if the ENFORCE option
 * has been enabled. Users who do not identify within 30-60 seconds have
 * their nick changed to <guest prefix><num>.
 * If the ircd or protocol module do not support forced nick changes,
 * they are killed instead.
 * Enforcement of the nick uses holdnick_sts() style enforcers if supported
 * by the ircd, otherwise clients.
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"nickserv/enforce",false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

typedef struct {
	char nick[NICKLEN];
	char host[HOSTLEN];
	time_t timelimit;
	mowgli_node_t node;
} enforce_timeout_t;

mowgli_list_t enforce_list;
mowgli_heap_t *enforce_timeout_heap;
time_t enforce_next;

static void guest_nickname(user_t *u);

static void ns_cmd_set_enforce(sourceinfo_t *si, int parc, char *parv[]);
static void ns_cmd_release(sourceinfo_t *si, int parc, char *parv[]);
static void ns_cmd_regain(sourceinfo_t *si, int parc, char *parv[]);

static void enforce_timeout_check(void *arg);
static void show_enforce(hook_user_req_t *hdata);
static void check_registration(hook_user_register_check_t *hdata);
static void check_enforce(hook_nick_enforce_t *hdata);

command_t ns_set_enforce = { "ENFORCE", N_("Enables or disables automatic protection of a nickname."), AC_NONE, 1, ns_cmd_set_enforce, { .path = "nickserv/set_enforce" } };
command_t ns_release = { "RELEASE", N_("Releases a services enforcer."), AC_NONE, 2, ns_cmd_release, { .path = "nickserv/release" } };
command_t ns_regain = { "REGAIN", N_("Regain usage of a nickname."), AC_NONE, 2, ns_cmd_regain, { .path = "nickserv/regain" } };

mowgli_patricia_t **ns_set_cmdtree;

/* sends an FNC for the given user */
static void guest_nickname(user_t *u)
{
	char gnick[NICKLEN];
	int tries;

	/* Generate a new guest nickname and check if it already exists
	 * This will try to generate a new nickname 30 different times
	 * if nicks are in use. If it runs into 30 nicks in use, maybe 
	 * you shouldn't use this module. */
	for (tries = 0; tries < 30; tries++)
	{
		snprintf(gnick, sizeof gnick, "%s%d", nicksvs.enforce_prefix, arc4random()%100000);

		if (!user_find_named(gnick))
			break;
	}
	fnc_sts(nicksvs.me->me, u, gnick, FNC_FORCE);
}

static void check_enforce_all(myuser_t *mu)
{
	mowgli_node_t *n;
	mynick_t *mn;
	user_t *u;

	MOWGLI_ITER_FOREACH(n, mu->nicks.head)
	{
		mn = n->data;
		u = user_find(mn->nick);
		if (u != NULL && u->myuser != mn->owner &&
				!myuser_access_verify(u, mn->owner))
			check_enforce(&(hook_nick_enforce_t){ .u = u, .mn = mn });
	}
}

static void ns_cmd_set_enforce(sourceinfo_t *si, int parc, char *parv[])
{
	char *setting = parv[0];

	if (!setting)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "ENFORCE");
		command_fail(si, fault_needmoreparams, _("Syntax: SET ENFORCE ON|OFF"));
		return;
	}

	if (!si->smu)
	{
		command_fail(si, fault_noprivs, _("You are not logged in."));
		return;
	}

	if (strcasecmp(setting, "ON") == 0)
	{
		if (metadata_find(si->smu, "private:doenforce"))
		{
			command_fail(si, fault_nochange, _("The \2%s\2 flag is already set for account \2%s\2."), "ENFORCE", entity(si->smu)->name);
		}
		else
		{
			logcommand(si, CMDLOG_SET, "SET:ENFORCE:ON");
			metadata_add(si->smu, "private:doenforce", "1");
			command_success_nodata(si, _("The \2%s\2 flag has been set for account \2%s\2."), "ENFORCE", entity(si->smu)->name);
			check_enforce_all(si->smu);
		}
	}
	else if (strcasecmp(setting, "OFF") == 0)
	{
		if (metadata_find(si->smu, "private:doenforce"))
		{
			logcommand(si, CMDLOG_SET, "SET:ENFORCE:OFF");
			metadata_delete(si->smu, "private:doenforce");
			command_success_nodata(si, _("The \2%s\2 flag has been removed for account \2%s\2."), "ENFORCE", entity(si->smu)->name);
		}
		else
		{
			command_fail(si, fault_nochange, _("The \2%s\2 flag is not set for account \2%s\2."), "ENFORCE", entity(si->smu)->name);
		}
	}
	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "ENFORCE");
	}
}

static void ns_cmd_release(sourceinfo_t *si, int parc, char *parv[])
{
	mynick_t *mn;
	char *target = parv[0];
	char *password = parv[1];
	user_t *u;
	mowgli_node_t *n, *tn;
	enforce_timeout_t *timeout;

	/* Absolutely do not do anything like this if nicks
	 * are not considered owned */
	if (nicksvs.no_nick_ownership)
	{
		command_fail(si, fault_noprivs, _("RELEASE is disabled."));
		return;
	}

	if (!target && si->smu != NULL)
		target = entity(si->smu)->name;
	if (!target)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "RELEASE");
		command_fail(si, fault_needmoreparams, _("Syntax: RELEASE <nick> [password]"));
		return;
	}

	u = user_find_named(target);
	mn = mynick_find(target);
	
	if (!mn)
	{
		command_fail(si, fault_nosuch_target, _("\2%s\2 is not a registered nickname."), target);
		return;
	}
	
	if (u == si->su)
	{
		command_fail(si, fault_noprivs, _("You cannot RELEASE yourself."));
		return;
	}
	if ((si->smu == mn->owner) || verify_password(mn->owner, password))
	{
		/* if this (nick, host) is waiting to be enforced, remove it */
		if (si->su != NULL)
		{
			MOWGLI_ITER_FOREACH_SAFE(n, tn, enforce_list.head)
			{
				timeout = n->data;
				if (!irccasecmp(mn->nick, timeout->nick) && (!strcmp(si->su->host, timeout->host) || !strcmp(si->su->vhost, timeout->host)))
				{
					mowgli_node_delete(&timeout->node, &enforce_list);
					mowgli_heap_free(enforce_timeout_heap, timeout);
				}
			}
		}
		if (u == NULL || is_internal_client(u))
		{
			logcommand(si, CMDLOG_DO, "RELEASE: \2%s\2", target);
			holdnick_sts(si->service->me, 0, target, mn->owner);
			if (u != NULL && u->flags & UF_ENFORCER)
			{
				quit_sts(u, "RELEASE command");
				user_delete(u, "RELEASE command");
			}
			command_success_nodata(si, _("\2%s\2 has been released."), target);
		}
		else
		{
			notice(nicksvs.nick, target, "%s has released your nickname.", get_source_mask(si));
			guest_nickname(u);
			if (ircd->flags & IRCD_HOLDNICK)
				holdnick_sts(nicksvs.me->me, 60 + arc4random() % 60, u->nick, mn->owner);
			else
				u->flags |= UF_DOENFORCE;
			command_success_nodata(si, _("%s has been released."), target);
			logcommand(si, CMDLOG_DO, "RELEASE: \2%s!%s@%s\2", u->nick, u->user, u->vhost);
		}
		return;
	}
	if (!password)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "RELEASE");
		command_fail(si, fault_needmoreparams, _("Syntax: RELEASE <nickname> [password]"));
		return;
	}
	else
	{
		logcommand(si, CMDLOG_DO, "failed RELEASE \2%s\2 (bad password)", target);
		command_fail(si, fault_authfail, _("Invalid password for \2%s\2."), target);
		bad_password(si, mn->owner);
	}
}

static void ns_cmd_regain(sourceinfo_t *si, int parc, char *parv[])
{
	mynick_t *mn;
	char *target = parv[0];
	char *password = parv[1];
	user_t *u;
	mowgli_node_t *n, *tn;
	enforce_timeout_t *timeout;

	/* Absolutely do not do anything like this if nicks
	 * are not considered owned */
	if (nicksvs.no_nick_ownership)
	{
		command_fail(si, fault_noprivs, _("REGAIN is disabled."));
		return;
	}

	if (!target && si->smu != NULL)
		target = entity(si->smu)->name;
	if (!target)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "REGAIN");
		command_fail(si, fault_needmoreparams, _("Syntax: REGAIN <nick> [password]"));
		return;
	}

	u = user_find_named(target);
	mn = mynick_find(target);
	
	if (!mn)
	{
		command_fail(si, fault_nosuch_target, _("\2%s\2 is not a registered nickname."), target);
		return;
	}
	
	if (u == si->su)
	{
		command_fail(si, fault_noprivs, _("You cannot REGAIN yourself."));
		return;
	}
	if ((si->smu == mn->owner) || verify_password(mn->owner, password))
	{
		/* if this (nick, host) is waiting to be enforced, remove it */
		if (si->su != NULL)
		{
			MOWGLI_ITER_FOREACH_SAFE(n, tn, enforce_list.head)
			{
				timeout = n->data;
				if (!irccasecmp(mn->nick, timeout->nick) && (!strcmp(si->su->host, timeout->host) || !strcmp(si->su->vhost, timeout->host)))
				{
					mowgli_node_delete(&timeout->node, &enforce_list);
					mowgli_heap_free(enforce_timeout_heap, timeout);
				}
			}
		}
		if (u == NULL || is_internal_client(u))
		{
			logcommand(si, CMDLOG_DO, "REGAIN: \2%s\2", target);
			holdnick_sts(si->service->me, 0, target, mn->owner);
			if (u != NULL && u->flags & UF_ENFORCER)
			{
				quit_sts(u, "REGAIN command");
				user_delete(u, "REGAIN command");
			}
			fnc_sts(nicksvs.me->me, si->su, target, FNC_FORCE);
			command_success_nodata(si, _("\2%s\2 has been regained."), target);
		}
		else
		{
			notice(nicksvs.nick, target, "\2%s\2 has regained your nickname.", get_source_mask(si));
			guest_nickname(u);
			fnc_sts(nicksvs.me->me, si->su, target, FNC_FORCE);
			command_success_nodata(si, _("\2%s\2 has been regained."), target);
			logcommand(si, CMDLOG_DO, "REGAIN: \2%s!%s@%s\2", u->nick, u->user, u->vhost);
		}
		return;
	}
	if (!password)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "REGAIN");
		command_fail(si, fault_needmoreparams, _("Syntax: REGAIN <nickname> [password]"));
		return;
	}
	else
	{
		logcommand(si, CMDLOG_DO, "failed REGAIN \2%s\2 (bad password)", target);
		command_fail(si, fault_authfail, _("Invalid password for \2%s\2."), target);
		bad_password(si, mn->owner);
	}
}

static void enforce_remove_enforcers(void *arg)
{
	mowgli_node_t *n, *tn;
	user_t *u;

	MOWGLI_ITER_FOREACH_SAFE(n, tn, me.me->userlist.head)
	{
		u = n->data;
		if (u->flags & UF_ENFORCER)
		{
			quit_sts(u, "Timed out");
			user_delete(u, "Timed out");
		}
	}
}

void enforce_timeout_check(void *arg)
{
	mowgli_node_t *n, *tn;
	enforce_timeout_t *timeout;
	user_t *u;
	mynick_t *mn;
	bool valid;

	enforce_next = 0;
	MOWGLI_ITER_FOREACH_SAFE(n, tn, enforce_list.head)
	{
		timeout = n->data;
		if (timeout->timelimit > CURRTIME)
		{
			enforce_next = timeout->timelimit;
			event_add_once("enforce_timeout_check", enforce_timeout_check, NULL, enforce_next - CURRTIME);
			break; /* assume sorted list */
		}
		u = user_find_named(timeout->nick);
		mn = mynick_find(timeout->nick);
		valid = u != NULL && mn != NULL && (!strcmp(u->host, timeout->host) || !strcmp(u->vhost, timeout->host));
		mowgli_node_delete(&timeout->node, &enforce_list);
		mowgli_heap_free(enforce_timeout_heap, timeout);
		if (!valid)
			continue;
		if (is_internal_client(u))
			continue;
		if (u->myuser == mn->owner)
			continue;
		if (myuser_access_verify(u, mn->owner))
			continue;
		if (!metadata_find(mn->owner, "private:doenforce"))
			continue;

		notice(nicksvs.nick, u->nick, "You failed to identify in time for the nickname %s", mn->nick);
		guest_nickname(u);
		if (ircd->flags & IRCD_HOLDNICK)
			holdnick_sts(nicksvs.me->me, u->flags & UF_WASENFORCED ? 3600 : 30, u->nick, mn->owner);
		else
			u->flags |= UF_DOENFORCE;
		u->flags |= UF_WASENFORCED;
	}
}

static void show_enforce(hook_user_req_t *hdata)
{

	if (metadata_find(hdata->mu, "private:doenforce"))
		command_success_nodata(hdata->si, "%s has enabled nick protection", entity(hdata->mu)->name);
}

static void check_registration(hook_user_register_check_t *hdata)
{
	int prefixlen = strlen(nicksvs.enforce_prefix);

	if (hdata->approved)
		return;
	
	if (!strncasecmp(hdata->account, nicksvs.enforce_prefix, prefixlen) && isdigit(hdata->account[prefixlen]))
	{
		command_fail(hdata->si, fault_badparams, "The nick \2%s\2 is reserved and cannot be registered.", hdata->account);
		hdata->approved = 1;
	}
}

static void check_enforce(hook_nick_enforce_t *hdata)
{
	enforce_timeout_t *timeout, *timeout2;
	mowgli_node_t *n;
	metadata_t *md;

	/* nick is a service, ignore it */
	if (is_internal_client(hdata->u))
		return;

	if (!metadata_find(hdata->mn->owner, "private:doenforce"))
		return;
	/* check if this nick has been used recently enough */
	if (nicksvs.enforce_expiry > 0 &&
			!(hdata->mn->owner->flags & MU_HOLD) &&
			(unsigned int)(CURRTIME - hdata->mn->lastseen) > nicksvs.enforce_expiry)
		return;

	/* check if it's already in enforce_list */
	timeout = NULL;
#ifdef SHOW_CORRECT_TIMEOUT_BUT_BE_SLOW
	/* don't do this now, it's O(n^2) in the number of users using
	 * a nick without access at a time */
	MOWGLI_ITER_FOREACH(n, enforce_list.head)
	{
		timeout2 = n->data;
		if (!irccasecmp(hdata->mn->nick, timeout2->nick) && (!strcmp(hdata->u->host, timeout2->host) || !strcmp(hdata->u->vhost, timeout2->host)))
		{
			timeout = timeout2;
			break;
		}
	}
#endif

	if (timeout == NULL)
	{
		timeout = mowgli_heap_alloc(enforce_timeout_heap);
		strlcpy(timeout->nick, hdata->mn->nick, sizeof timeout->nick);
		strlcpy(timeout->host, hdata->u->host, sizeof timeout->host);

		if (!metadata_find(hdata->mn->owner, "private:enforcetime"))
			timeout->timelimit = CURRTIME + nicksvs.enforce_delay;
		else
		{
			md = metadata_find(hdata->mn->owner, "private:enforcetime");
			int enforcetime = atoi(md->value);
			timeout->timelimit = CURRTIME + enforcetime;
		}

		/* insert in sorted order */
		MOWGLI_ITER_FOREACH_PREV(n, enforce_list.tail)
		{
			timeout2 = n->data;
			if (timeout2->timelimit <= timeout->timelimit)
				break;
		}
		if (n == NULL)
			mowgli_node_add_head(timeout, &timeout->node, &enforce_list);
		else if (n->next == NULL)
			mowgli_node_add(timeout, &timeout->node, &enforce_list);
		else
			mowgli_node_add_before(timeout, &timeout->node, &enforce_list, n->next);

		if (enforce_next == 0 || enforce_next > timeout->timelimit)
		{
			if (enforce_next != 0)
				event_delete(enforce_timeout_check, NULL);
			enforce_next = timeout->timelimit;
			event_add_once("enforce_timeout_check", enforce_timeout_check, NULL, enforce_next - CURRTIME);
		}
	}

	notice(nicksvs.nick, hdata->u->nick, "You have %d seconds to identify to your nickname before it is changed.", (int)(timeout->timelimit - CURRTIME));
}

static int idcheck_foreach_cb(myentity_t *mt, void *privdata)
{
	myuser_t *mu = user(mt);

	if (metadata_find(mu, "private:idcheck"))
		metadata_delete(mu, "private:idcheck");
	if (metadata_find(mu, "private:enforcer"))
		metadata_delete(mu, "private:enforcer");

	return 0;
}

void _modinit(module_t *m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "nickserv/set_core");
	MODULE_USE_SYMBOL(ns_set_cmdtree, "nickserv/set_core", "ns_set_cmdtree");

	/* Leave this for compatibility with old versions of this code
	 * -- jilles
	 */
	myentity_foreach_t(ENT_USER, idcheck_foreach_cb, NULL);

	/* Absolutely do not do anything like this if nicks
	 * are not considered owned */
	if (nicksvs.no_nick_ownership)
	{
		slog(LG_ERROR, "modules/nickserv/enforce: nicks are not configured to be owned");
		m->mflags = MODTYPE_FAIL;
		return;
	}
	
	enforce_timeout_heap = mowgli_heap_create(sizeof(enforce_timeout_t), 128, BH_NOW);
	if (enforce_timeout_heap == NULL)
	{
		m->mflags = MODTYPE_FAIL;
		return;
	}

	event_add("enforce_remove_enforcers", enforce_remove_enforcers, NULL, 300);
	service_named_bind_command("nickserv", &ns_release);
	service_named_bind_command("nickserv", &ns_regain);
	command_add(&ns_set_enforce, *ns_set_cmdtree);
	hook_add_event("user_info");
	hook_add_user_info(show_enforce);
	hook_add_event("nick_can_register");
	hook_add_nick_can_register(check_registration);
	hook_add_event("nick_enforce");
	hook_add_nick_enforce(check_enforce);
}

void _moddeinit()
{
	enforce_remove_enforcers(NULL);
	event_delete(enforce_remove_enforcers, NULL);
	if (enforce_next)
		event_delete(enforce_timeout_check, NULL);
	service_named_unbind_command("nickserv", &ns_release);
	service_named_unbind_command("nickserv", &ns_regain);
	command_delete(&ns_set_enforce, *ns_set_cmdtree);
	hook_del_user_info(show_enforce);
	hook_del_nick_can_register(check_registration);
	hook_del_nick_enforce(check_enforce);
	mowgli_heap_destroy(enforce_timeout_heap);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
