/*
 * Copyright (c) 2005-2006 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the NickServ RELEASE/ENFORCE functions.
 *
 * This does nickserv enforcement on registered nicks if the ENFORCE option
 * has been enabled. Users who do not identify within 30-60 seconds have
 * their nick changed to Guest<num>.
 * If the ircd or protocol module do not support forced nick changes,
 * they are killed instead.
 * Enforcement of the nick is only supported for ircds that support
 * holdnick_sts(), currently bahamut, charybdis, hybrid, inspircd11,
 * solidircd, ratbox and unreal (i.e. making sure they can't change back
 * immediately). Consequently this module is of little use for other ircds.
 * Note: For hybrid and ratbox, and charybdis before 2.1, the
 * RELEASE command to remove an enforcer prematurely is not supported,
 * although it pretends to be successful.
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"nickserv/enforce",FALSE, _modinit, _moddeinit,
	"$Id: enforce.c 7667 2007-02-15 12:06:12Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void ns_cmd_set_enforce(sourceinfo_t *si, int parc, char *parv[]);
static void ns_cmd_release(sourceinfo_t *si, int parc, char *parv[]);
static void reg_check(void *arg);
static void remove_idcheck(void *vuser);
static void show_enforce(void *vdata);

command_t ns_set_enforce = { "ENFORCE", "Enables or disables automatic protection of a nickname.", AC_NONE, 1, ns_cmd_set_enforce }; 
command_t ns_release = { "RELEASE", "Releases a services enforcer.", AC_NONE, 2, ns_cmd_release };

list_t *ns_cmdtree, *ns_set_cmdtree, *ns_helptree;

#if 0
void manage_bots(void *arg)
{
	user_t *u;
	node_t *n, *tn;
	myuser_t *mu;
	metadata_t *md;
	int i = 0;
	
	for (i = 0; i < HASHSIZE; i++) 
	{
		LIST_FOREACH_SAFE(n, tn, userlist[i].head)
		{
			u = (user_t *)n->data;
			if ((mu = myuser_find(u->nick)))
			{
			if (is_internal_client(u))
			{
				if (md = metadata_find(mu, METADATA_USER, "private:enforcer"))
				{
					int x = atoi(md->value);
					if(x < 5)
					{
						x++;
						char buf[32];
						sprintf(buf, "%d", x);
						metadata_add(mu, METADATA_USER, "private:enforcer", buf);
					}
					else
					{
						metadata_delete(mu, METADATA_USER, "private:enforcer");
						quit_sts(u, "timed out svs enforcer");
					}
				}
			}
			}
		}
	}
}
#endif

static void ns_cmd_set_enforce(sourceinfo_t *si, int parc, char *parv[])
{
	metadata_t *md;
	char *setting = parv[0];

	if (!setting)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "ENFORCE");
		command_fail(si, fault_needmoreparams, "Syntax: SET ENFORCE ON|OFF");
		return;
	}

	if (!si->smu)
	{
		command_fail(si, fault_noprivs, "You are not logged in.");
		return;
	}

	if (strcasecmp(setting, "ON") == 0)
	{
		if ((md = metadata_find(si->smu, METADATA_USER, "private:doenforce")) != NULL)
		{
			command_fail(si, fault_nochange, "ENFORCE is already enabled.");
		}
		else
		{
			metadata_add(si->smu, METADATA_USER, "private:doenforce", "1");
			command_success_nodata(si, "ENFORCE is now enabled.");
		}
	}
	else if (strcasecmp(setting, "OFF") == 0)
	{
		if ((md = metadata_find(si->smu, METADATA_USER, "private:doenforce")) != NULL)
		{
			metadata_delete(si->smu, METADATA_USER, "private:doenforce");
			command_success_nodata(si, "ENFORCE is now disabled.");
		}
		else
		{
			command_fail(si, fault_nochange, "ENFORCE is already disabled.");
		}
	}
	else
	{
		command_fail(si, fault_badparams, "Unknown value for ENFORCE. Expected values are ON or OFF.");
	}
}

static void ns_cmd_release(sourceinfo_t *si, int parc, char *parv[])
{
	mynick_t *mn;
	metadata_t *md;
	char *target = parv[0];
	char *password = parv[1];
	char *gnick;
	int i;
	user_t *u;
	char ign[BUFSIZE];

	/* Absolutely do not do anything like this if nicks
	 * are not considered owned */
	if (nicksvs.no_nick_ownership)
	{
		command_fail(si, fault_noprivs, "RELEASE is disabled.");
		return;
	}

	if (!target && si->smu != NULL)
		target = si->smu->name;
	if (!target)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "RELEASE");
		command_fail(si, fault_needmoreparams, "Syntax: RELEASE <nick> [password]");
		return;
	}

	u = user_find_named(target);
	mn = mynick_find(target);
	
	if (!mn)
	{
		command_fail(si, fault_nosuch_target, "\2%s\2 is not a registered nickname.", target);
		return;
	}
	
	if (u == si->su)
	{
		command_fail(si, fault_noprivs, "You cannot RELEASE yourself.");
		return;
	}
	if ((si->smu == mn->owner) || verify_password(mn->owner, password))
	{
		if (u == NULL || is_internal_client(u))
		{
			if ((md = metadata_find(mn->owner, METADATA_USER, "private:enforcer")) != NULL)
				metadata_delete(mn->owner, METADATA_USER, "private:enforcer");
			logcommand(si, CMDLOG_DO, "RELEASE %s", target);
			holdnick_sts(si->service->me, 0, target, mn->owner);
			command_success_nodata(si, "\2%s\2 has been released.", target);
			/*hook_call_event("user_identify", u);*/
		}
		else
		{
			if ((md = metadata_find(mn->owner, METADATA_USER, "private:enforcer")) != NULL)
				metadata_delete(mn->owner, METADATA_USER, "private:enforcer");
			
			notice(nicksvs.nick, target, "%s has released your nickname.", get_source_mask(si));
			i = 0;
			for (i = 0; i < 30; i++)
			{
				snprintf( ign, BUFSIZE, "Guest%d", arc4random()%99999 );
				gnick = ign;
				if (!user_find_named(ign))
					break;
			}
			fnc_sts(si->service->me, u, gnick, FNC_FORCE);
			command_success_nodata(si, "%s has been released.", target);
			logcommand(si, CMDLOG_DO, "RELEASE %s!%s@%s", u->nick, u->user, u->vhost);
			/*hook_call_event("user_identify", u);*/
		}
		return;
	}
	if (!password)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "RELEASE");
		command_fail(si, fault_needmoreparams, "Syntax: RELEASE <nickname> [password]");
		return;
	}
	else
	{
		logcommand(si, CMDLOG_DO, "failed RELEASE %s (bad password)", target);
		command_fail(si, fault_authfail, "Invalid password for \2%s\2.", target);
	}
}

void reg_check(void *arg)
{
	user_t *u;
	mynick_t *mn;
	myuser_t *mu;
	char *gnick;
	int x = 0;
	char ign[BUFSIZE];
	dictionary_iteration_state_t state;

	/* Absolutely do not do anything like this if nicks
	 * are not considered owned */
	if (nicksvs.no_nick_ownership)
		return;
	
	DICTIONARY_FOREACH(u, &state, userlist)
	{
		/* nick is a service, ignore it */
		if (is_internal_client(u))
			continue;
		if ((mn = mynick_find(u->nick)))
		{
			mu = mn->owner;
			if (u->myuser == mu)
				continue;
			if (!metadata_find(mu, METADATA_USER, "private:doenforce"))
				;
			else if (myuser_access_verify(u, mu))
				;
			else if (u->flags & UF_NICK_WARNED)
			{
				notice(nicksvs.nick, u->nick, "You failed to authenticate in time for the nickname %s", u->nick);
				/* Generate a new guest nickname and check if it already exists
				 * This will try to generate a new nickname 30 different times
				 * if nicks are in use. If it runs into 30 nicks in use, maybe 
				 * you shouldn't use this module. */
				for (x = 0; x < 30; x++)
				{
					snprintf( ign, BUFSIZE, "Guest%d", arc4random()%99999 );
					gnick = ign;
					if (!user_find_named(ign))
						break;
				}
				fnc_sts(nicksvs.me->me, u, gnick, FNC_FORCE);
				holdnick_sts(nicksvs.me->me, 3600, u->nick, mu);
#if 0 /* can't do this, need to wait for SVSNICK to complete! -- jilles */
				uid = uid_get();
				introduce_nick(u->nick, "enforcer", "services.hold", "Services Enforcer", uid);
				user_add(u->nick, "enforcer", "services.hold", NULL, NULL, uid, "Services Enforcer", me.me, CURRTIME);
#endif
				u->flags &= ~UF_NICK_WARNED;
				metadata_add(mu, METADATA_USER, "private:enforcer", "1");
			}
			else
			{
				u->flags |= UF_NICK_WARNED;
				notice(nicksvs.nick, u->nick, "You have 30 seconds to identify to your nickname before it is changed.");
			}
		}
	}
}

static void remove_idcheck(void *vuser)
{
	user_t *u;

	u = vuser;
	u->flags &= ~UF_NICK_WARNED;
}

static void show_enforce(void *vdata)
{
	hook_user_req_t *hdata = vdata;

	if (metadata_find(hdata->mu, METADATA_USER, "private:doenforce"))
		command_success_nodata(hdata->si, "%s has enabled nick protection", hdata->mu->name);
}

static void check_registration(void *vdata)
{
	hook_user_register_check_t *hdata = vdata;

	if (hdata->approved)
		return;
	if (!strncasecmp(hdata->account, "Guest", 5) && isdigit(hdata->account[5]))
	{
		command_fail(hdata->si, fault_badparams, "The nick \2%s\2 is reserved and cannot be registered.", hdata->account);
		hdata->approved = 1;
	}
}

static int idcheck_foreach_cb(dictionary_elem_t *delem, void *privdata)
{
	metadata_t *md;
	myuser_t *mu = (myuser_t *) delem->node.data;

	if ((md = metadata_find(mu, METADATA_USER, "private:idcheck")))
		metadata_delete(mu, METADATA_USER, "private:idcheck");

	return 0;
}

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(ns_cmdtree, "nickserv/main", "ns_cmdtree");
	MODULE_USE_SYMBOL(ns_helptree, "nickserv/main", "ns_helptree");
	MODULE_USE_SYMBOL(ns_set_cmdtree, "nickserv/set", "ns_set_cmdtree");

	/* Leave this for compatibility with old versions of this code
	 * -- jilles
	 */
	dictionary_foreach(mulist, idcheck_foreach_cb, NULL);

	event_add("reg_check", reg_check, NULL, 30);
	/*event_add("manage_bots", manage_bots, NULL, 30);*/
	command_add(&ns_release, ns_cmdtree);
	command_add(&ns_set_enforce, ns_set_cmdtree);
	help_addentry(ns_helptree, "RELEASE", "help/nickserv/release", NULL);
	help_addentry(ns_helptree, "SET ENFORCE", "help/nickserv/set_enforce", NULL);
	hook_add_event("user_identify");
	hook_add_hook("user_identify", remove_idcheck);
	hook_add_event("user_info");
	hook_add_hook("user_info", show_enforce);
	hook_add_event("user_can_register");
	hook_add_hook("user_can_register", check_registration);
}

void _moddeinit()
{
	event_delete(reg_check, NULL);
	/*event_delete(manage_bots, NULL);*/
	command_delete(&ns_release, ns_cmdtree);
	command_delete(&ns_set_enforce, ns_set_cmdtree);
	help_delentry(ns_helptree, "RELEASE");
	help_delentry(ns_helptree, "SET ENFORCE");
	hook_del_hook("user_identify", remove_idcheck);
	hook_del_hook("user_info", show_enforce);
	hook_del_hook("user_can_register", check_registration);
}
