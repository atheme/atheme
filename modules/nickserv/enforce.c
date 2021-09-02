/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2007 Atheme Project (http://atheme.org/)
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

#include <atheme.h>

struct enforce_timeout
{
	char nick[NICKLEN + 1];
	char host[HOSTLEN + 1];
	time_t timelimit;
	mowgli_node_t node;
};

static mowgli_heap_t *enforce_timeout_heap = NULL;
static mowgli_eventloop_timer_t *enforce_timeout_check_timer = NULL;
static mowgli_eventloop_timer_t *enforce_remove_enforcers_timer = NULL;

static mowgli_list_t enforce_list;
static time_t enforce_next;

static mowgli_patricia_t **ns_set_cmdtree;

// logs a released nickname out
static bool
log_enforce_victim_out(struct user *u, struct myuser *mu)
{
	struct mynick *mn;
	mowgli_node_t *n, *tn;

	return_val_if_fail(u != NULL, false);

	if (u->myuser == NULL || u->myuser != mu)
		return false;

	u->myuser->lastlogin = CURRTIME;

	if ((mn = mynick_find(u->nick)) != NULL)
		mn->lastseen = CURRTIME;

	if (!ircd_logout_or_kill(u, entity(u->myuser)->name))
	{
		MOWGLI_ITER_FOREACH_SAFE(n, tn, u->myuser->logins.head)
		{
			if (n->data == u)
			{
				mowgli_node_delete(n, &u->myuser->logins);
				mowgli_node_free(n);
				break;
			}
		}

		u->myuser = NULL;
		return false;
	}

	return true;
}

// sends an FNC for the given user
static void
guest_nickname(struct user *u)
{
	char gnick[NICKLEN + 1];
	int tries;

	/* Generate a new guest nickname and check if it already exists
	 * This will try to generate a new nickname 30 different times
	 * if nicks are in use. If it runs into 30 nicks in use, maybe
	 * you shouldn't use this module. */
	for (tries = 0; tries < 30; tries++)
	{
		snprintf(gnick, sizeof gnick, "%s%u", nicksvs.enforce_prefix, 1 + atheme_random_uniform(9999));

		if (!user_find_named(gnick))
			break;
	}
	fnc_sts(nicksvs.me->me, u, gnick, FNC_FORCE);
}

static void
enforce_timeout_check(void *arg)
{
	mowgli_node_t *n, *tn;
	struct enforce_timeout *timeout;
	struct user *u;
	struct mynick *mn;
	bool valid;

	enforce_next = 0;
	MOWGLI_ITER_FOREACH_SAFE(n, tn, enforce_list.head)
	{
		timeout = n->data;
		if (timeout->timelimit > CURRTIME)
		{
			enforce_next = timeout->timelimit;
			enforce_timeout_check_timer = mowgli_timer_add_once(base_eventloop, "enforce_timeout_check", enforce_timeout_check, NULL, enforce_next - CURRTIME);
			break; // assume sorted list
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
			holdnick_sts(nicksvs.me->me, u->flags & UF_WASENFORCED ? SECONDS_PER_HOUR : 30, u->nick, mn->owner);
		else
			u->flags |= UF_DOENFORCE;
		u->flags |= UF_WASENFORCED;
	}
}

static void
check_enforce(struct hook_nick_enforce *hdata)
{
	struct enforce_timeout *timeout, *timeout2;
	mowgli_node_t *n;
	struct metadata *md;

	// nick is a service, ignore it
	if (is_internal_client(hdata->u))
		return;

	if (!metadata_find(hdata->mn->owner, "private:doenforce"))
		return;
	// check if this nick has been used recently enough
	if (nicksvs.enforce_expiry > 0 &&
			!(hdata->mn->owner->flags & MU_HOLD) &&
			(unsigned int)(CURRTIME - hdata->mn->lastseen) > nicksvs.enforce_expiry)
		return;

	// check if it's already in enforce_list
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
		mowgli_strlcpy(timeout->nick, hdata->mn->nick, sizeof timeout->nick);
		mowgli_strlcpy(timeout->host, hdata->u->host, sizeof timeout->host);

		if (!metadata_find(hdata->mn->owner, "private:enforcetime"))
			timeout->timelimit = CURRTIME + nicksvs.enforce_delay;
		else
		{
			md = metadata_find(hdata->mn->owner, "private:enforcetime");
			int enforcetime = atoi(md->value);
			timeout->timelimit = CURRTIME + enforcetime;
		}

		// insert in sorted order
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
				mowgli_timer_destroy(base_eventloop, enforce_timeout_check_timer);
			enforce_next = timeout->timelimit;
			enforce_timeout_check_timer = mowgli_timer_add_once(base_eventloop, "enforce_timeout_check", enforce_timeout_check, NULL, enforce_next - CURRTIME);
		}
	}

	notice(nicksvs.nick, hdata->u->nick, "You have %u seconds to identify to your nickname before it is changed.", (unsigned int)(timeout->timelimit - CURRTIME));
}

static void
check_enforce_all(struct myuser *mu)
{
	mowgli_node_t *n;
	struct mynick *mn;
	struct user *u;

	MOWGLI_ITER_FOREACH(n, mu->nicks.head)
	{
		mn = n->data;
		u = user_find(mn->nick);
		if (u != NULL && u->myuser != mn->owner &&
				!myuser_access_verify(u, mn->owner))
			check_enforce(&(struct hook_nick_enforce){ .u = u, .mn = mn });
	}
}

static void
ns_cmd_set_enforce(struct sourceinfo *si, int parc, char *parv[])
{
	char *setting = parv[0];

	if (!setting)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "ENFORCE");
		command_fail(si, fault_needmoreparams, _("Syntax: SET ENFORCE ON|OFF"));
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

static void
ns_cmd_release(struct sourceinfo *si, int parc, char *parv[])
{
	struct mynick *mn;
	const char *target = parv[0];
	const char *password = parv[1];
	struct user *u;
	mowgli_node_t *n, *tn;
	struct enforce_timeout *timeout;

	// Absolutely do not do anything like this if nicks are not considered owned
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

	// The != NULL check is required to make releasing an enforcer via xmlrpc work
	if (u != NULL && u == si->su)
	{
		command_fail(si, fault_noprivs, _("You cannot RELEASE yourself."));
		return;
	}
	if (password && metadata_find(mn->owner, "private:freeze:freezer"))
	{
		command_fail(si, fault_authfail, _("You cannot release \2%s\2 because the account has been frozen."), target);
		logcommand(si, CMDLOG_DO, "failed RELEASE \2%s\2 (frozen)", target);
		return;
	}
	if (password && (mn->owner->flags & MU_NOPASSWORD))
	{
		command_fail(si, fault_authfail, _("Password authentication is disabled for this account."));
		logcommand(si, CMDLOG_DO, "failed RELEASE \2%s\2 (password authentication disabled)", target);
		return;
	}
	if ((si->smu == mn->owner) || verify_password(mn->owner, password))
	{
		// if this (nick, host) is waiting to be enforced, remove it
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

			if (!log_enforce_victim_out(u, mn->owner))
			{
				guest_nickname(u);
				if (ircd->flags & IRCD_HOLDNICK)
					holdnick_sts(nicksvs.me->me, SECONDS_PER_MINUTE + atheme_random_uniform(SECONDS_PER_MINUTE), u->nick, mn->owner);
				else
					u->flags |= UF_DOENFORCE;
				command_success_nodata(si, _("\2%s\2 has been released."), target);
				logcommand(si, CMDLOG_DO, "RELEASE: \2%s!%s@%s\2", u->nick, u->user, u->vhost);
			}
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

static void
ns_cmd_regain(struct sourceinfo *si, int parc, char *parv[])
{
	struct mynick *mn;
	const char *target = parv[0];
	const char *password = parv[1];
	struct user *u;
	mowgli_node_t *n, *tn;
	struct enforce_timeout *timeout;
	char lau[BUFSIZE];

	// Absolutely do not do anything like this if nicks are not considered owned
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
	if (password && metadata_find(mn->owner, "private:freeze:freezer"))
	{
		command_fail(si, fault_authfail, _("You cannot regain \2%s\2 because the account has been frozen."), target);
		logcommand(si, CMDLOG_DO, "failed REGAIN \2%s\2 (frozen)", target);
		return;
	}
	if (password && (mn->owner->flags & MU_NOPASSWORD))
	{
		command_fail(si, fault_authfail, _("Password authentication is disabled for this account."));
		logcommand(si, CMDLOG_DO, "failed REGAIN \2%s\2 (password authentication disabled)", target);
		return;
	}
	if (!is_valid_nick(target))
	{
		command_fail(si, fault_badparams, _("\2%s\2 is not a valid nick."), target);
		logcommand(si, CMDLOG_DO, "failed REGAIN \2%s\2 (not a valid nickname)", target);
		return;
	}
	if ((si->smu == mn->owner) || verify_password(mn->owner, password))
	{
		if (user_loginmaxed(mn->owner))
		{
			command_fail(si, fault_toomany, _("You were not logged in."));
			command_fail(si, fault_toomany, _("There are already \2%zu\2 sessions logged in to \2%s\2 (maximum allowed: %u)."), MOWGLI_LIST_LENGTH(&mn->owner->logins), entity(mn->owner)->name, me.maxlogins);

			lau[0] = '\0';
			MOWGLI_ITER_FOREACH(n, mn->owner->logins.head)
			{
				if (lau[0] != '\0')
					mowgli_strlcat(lau, ", ", sizeof lau);
				mowgli_strlcat(lau, ((struct user *)n->data)->nick, sizeof lau);
			}
			command_fail(si, fault_toomany, _("Logged in nicks are: %s"), lau);

			return;
		}
		// identify them to the target nick's account if they aren't yet
		if (si->smu != mn->owner)
		{
			// if they are identified to another account, nuke their session first
			if (si->smu != NULL)
			{
				command_success_nodata(si, _("You have been logged out of \2%s\2."), entity(si->smu)->name);

				if (ircd_logout_or_kill(si->su, entity(si->smu)->name))
					// logout killed the user...
					return;
				si->smu->lastlogin = CURRTIME;
				MOWGLI_ITER_FOREACH_SAFE(n, tn, si->smu->logins.head)
				{
					if (n->data == si->su)
					{
						mowgli_node_delete(n, &si->smu->logins);
						mowgli_node_free(n);
						break;
					}
				}
				si->su->myuser = NULL;
			}

			myuser_login(si->service, si->su, mn->owner, true);
		}

		struct chanuser *cu = NULL;
		if (si->su != NULL && ((cu = find_user_banned_channel(si->su, 'b')) || (cu = find_user_banned_channel(si->su, 'q'))))
		{
			command_fail(si, fault_noprivs, _("You can not regain your nickname while banned or quieted on a channel: \2%s\2"), cu->chan->name);
			return;
		}

		if (qline_find_match(target) && !has_priv(si, PRIV_MASS_AKILL))
		{
			command_fail(si, fault_noprivs, _("You can not regain a reserved nickname."));
			return;
		}

		// if this (nick, host) is waiting to be enforced, remove it
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
		if (u != NULL && is_service(u))
		{
			command_fail(si, fault_badparams, _("You cannot regain a network service."));
			return;
		}
		else if (u == NULL || is_internal_client(u))
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
			logcommand(si, CMDLOG_DO, "REGAIN: \2%s!%s@%s\2", u->nick, u->user, u->vhost);
			if (!log_enforce_victim_out(u, mn->owner))
			{
				if (ircd->flags & IRCD_HOLDNICK)
					holdnick_sts(nicksvs.me->me, SECONDS_PER_MINUTE + atheme_random_uniform(SECONDS_PER_MINUTE), u->nick, mn->owner);
				guest_nickname(u);
				command_success_nodata(si, _("\2%s\2 has been regained."), target);
			}
			fnc_sts(nicksvs.me->me, si->su, target, FNC_FORCE);
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

static void
enforce_remove_enforcers(void *arg)
{
	mowgli_node_t *n, *tn;
	struct user *u;

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

static void
show_enforce(struct hook_user_req *hdata)
{
	if (metadata_find(hdata->mu, "private:doenforce"))
		command_success_nodata(hdata->si, _("%s has enabled nick protection"), entity(hdata->mu)->name);
}

static void
check_registration(struct hook_user_register_check *hdata)
{
	int prefixlen;

	return_if_fail(nicksvs.enforce_prefix != NULL);

	prefixlen = strlen(nicksvs.enforce_prefix);

	if (hdata->approved)
		return;

	if (!strncasecmp(hdata->account, nicksvs.enforce_prefix, prefixlen) && isdigit((unsigned char)hdata->account[prefixlen]))
	{
		command_fail(hdata->si, fault_badparams, _("The nickname \2%s\2 is reserved and cannot be registered."), hdata->account);
		hdata->approved = 1;
	}
}

static int
idcheck_foreach_cb(struct myentity *mt, void *privdata)
{
	struct myuser *mu = user(mt);

	if (metadata_find(mu, "private:idcheck"))
		metadata_delete(mu, "private:idcheck");
	if (metadata_find(mu, "private:enforcer"))
		metadata_delete(mu, "private:enforcer");

	return 0;
}

static struct command ns_set_enforce = {
	.name           = "ENFORCE",
	.desc           = N_("Enables or disables automatic protection of a nickname."),
	.access         = AC_NONE,
	.maxparc        = 1,
	.cmd            = &ns_cmd_set_enforce,
	.help           = { .path = "nickserv/set_enforce" },
};

static struct command ns_release = {
	.name           = "RELEASE",
	.desc           = N_("Releases a services enforcer."),
	.access         = AC_NONE,
	.maxparc        = 2,
	.cmd            = &ns_cmd_release,
	.help           = { .path = "nickserv/release" },
};

static struct command ns_regain = {
	.name           = "REGAIN",
	.desc           = N_("Regain usage of a nickname."),
	.access         = AC_NONE,
	.maxparc        = 2,
	.cmd            = &ns_cmd_regain,
	.help           = { .path = "nickserv/regain" },
};

static void
mod_init(struct module *const restrict m)
{
	/* Leave this for compatibility with old versions of this code
	 * -- jilles
	 */
	myentity_foreach_t(ENT_USER, idcheck_foreach_cb, NULL);

	MODULE_TRY_REQUEST_DEPENDENCY(m, "nickserv/main")
	MODULE_TRY_REQUEST_SYMBOL(m, ns_set_cmdtree, "nickserv/set_core", "ns_set_cmdtree")

	// Absolutely do not do anything like this if nicks are not considered owned
	if (nicksvs.no_nick_ownership)
	{
		slog(LG_ERROR, "modules/nickserv/enforce: nicks are not configured to be owned");
		m->mflags |= MODFLAG_FAIL;
		return;
	}

	enforce_timeout_heap = mowgli_heap_create(sizeof(struct enforce_timeout), 128, BH_NOW);
	if (enforce_timeout_heap == NULL)
	{
		m->mflags |= MODFLAG_FAIL;
		return;
	}

	enforce_remove_enforcers_timer = mowgli_timer_add(base_eventloop, "enforce_remove_enforcers", enforce_remove_enforcers, NULL, 5 * SECONDS_PER_MINUTE);

	service_named_bind_command("nickserv", &ns_release);
	service_named_bind_command("nickserv", &ns_regain);
	command_add(&ns_set_enforce, *ns_set_cmdtree);
	hook_add_user_info(show_enforce);
	hook_add_nick_can_register(check_registration);
	hook_add_nick_enforce(check_enforce);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	enforce_remove_enforcers(NULL);

	mowgli_timer_destroy(base_eventloop, enforce_remove_enforcers_timer);

	if (enforce_next)
		mowgli_timer_destroy(base_eventloop, enforce_timeout_check_timer);

	service_named_unbind_command("nickserv", &ns_release);
	service_named_unbind_command("nickserv", &ns_regain);
	command_delete(&ns_set_enforce, *ns_set_cmdtree);
	hook_del_user_info(show_enforce);
	hook_del_nick_can_register(check_registration);
	hook_del_nick_enforce(check_enforce);
	mowgli_heap_destroy(enforce_timeout_heap);
}

SIMPLE_DECLARE_MODULE_V1("nickserv/enforce", MODULE_UNLOAD_CAPABILITY_OK)
