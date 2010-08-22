/*
 * Copyright (c) 2005-2006 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the NickServ IDENTIFY and LOGIN functions.
 *
 */

#include "atheme.h"

/* Check whether we are compiling IDENTIFY or LOGIN */
#ifdef NICKSERV_LOGIN
#define COMMAND_UC "LOGIN"
#define COMMAND_LC "login"
#else
#define COMMAND_UC "IDENTIFY"
#define COMMAND_LC "identify"
#endif

DECLARE_MODULE_V1
(
	"nickserv/" COMMAND_LC, false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void ns_cmd_login(sourceinfo_t *si, int parc, char *parv[]);

#ifdef NICKSERV_LOGIN
command_t ns_login = { "LOGIN", N_("Authenticates to a services account."), AC_NONE, 2, ns_cmd_login };
#else
command_t ns_identify = { "IDENTIFY", N_("Identifies to services for a nickname."), AC_NONE, 2, ns_cmd_login };
#endif

list_t *ns_cmdtree, *ns_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(ns_cmdtree, "nickserv/main", "ns_cmdtree");
	MODULE_USE_SYMBOL(ns_helptree, "nickserv/main", "ns_helptree");

#ifdef NICKSERV_LOGIN
	command_add(&ns_login, ns_cmdtree);
	help_addentry(ns_helptree, "LOGIN", "help/nickserv/login", NULL);
#else
	command_add(&ns_identify, ns_cmdtree);
	help_addentry(ns_helptree, "IDENTIFY", "help/nickserv/identify", NULL);
#endif
}

void _moddeinit()
{
#ifdef NICKSERV_LOGIN
	command_delete(&ns_login, ns_cmdtree);
	help_delentry(ns_helptree, "LOGIN");
#else
	command_delete(&ns_identify, ns_cmdtree);
	help_delentry(ns_helptree, "IDENTIFY");
#endif
}

static void ns_cmd_login(sourceinfo_t *si, int parc, char *parv[])
{
	user_t *u = si->su;
	myuser_t *mu;
	mynick_t *mn;
	chanuser_t *cu;
	chanacs_t *ca;
	node_t *n, *tn;
	char *target = parv[0];
	char *password = parv[1];
	char lau[BUFSIZE];

	if (si->su == NULL)
	{
		command_fail(si, fault_noprivs, _("\2%s\2 can only be executed via IRC."), COMMAND_UC);
		return;
	}

#ifndef NICKSERV_LOGIN
	if (!nicksvs.no_nick_ownership && target && !password)
	{
		password = target;
		target = si->su->nick;
	}
#endif

	if (!target || !password)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, COMMAND_UC);
		command_fail(si, fault_needmoreparams, nicksvs.no_nick_ownership ? "Syntax: " COMMAND_UC " <account> <password>" : "Syntax: " COMMAND_UC " [nick] <password>");
		return;
	}

	if (nicksvs.no_nick_ownership)
		mu = myuser_find(target);
	else
	{
		mn = mynick_find(target);
		mu = mn != NULL ? mn->owner : NULL;
	}

	if (!mu)
	{
		command_fail(si, fault_nosuch_target, _("\2%s\2 is not a registered nickname."), target);
		return;
	}

	if (metadata_find(mu, "private:freeze:freezer"))
	{
		command_fail(si, fault_authfail, nicksvs.no_nick_ownership ? "You cannot login as \2%s\2 because the account has been frozen." : "You cannot identify to \2%s\2 because the nickname has been frozen.", entity(mu)->name);
		logcommand(si, CMDLOG_LOGIN, "failed " COMMAND_UC " to \2%s\2 (frozen)", entity(mu)->name);
		return;
	}

	if (u->myuser == mu)
	{
		command_fail(si, fault_nochange, _("You are already logged in as \2%s\2."), entity(u->myuser)->name);
		if (mu->flags & MU_WAITAUTH)
			command_fail(si, fault_nochange, _("Please check your email for instructions to complete your registration."));
		return;
	}
	else if (u->myuser != NULL && !command_find(si->service->cmdtree, "LOGOUT"))
	{
		command_fail(si, fault_alreadyexists, _("You are already logged in as \2%s\2."), entity(u->myuser)->name);
		return;
	}

	if (verify_password(mu, password))
	{
		if (LIST_LENGTH(&mu->logins) >= me.maxlogins)
		{
			command_fail(si, fault_toomany, _("There are already \2%d\2 sessions logged in to \2%s\2 (maximum allowed: %d)."), LIST_LENGTH(&mu->logins), entity(mu)->name, me.maxlogins);
			lau[0] = '\0';
			LIST_FOREACH(n, mu->logins.head)
			{
				if (lau[0] != '\0')
					strlcat(lau, ", ", sizeof lau);
				strlcat(lau, ((user_t *)n->data)->nick, sizeof lau);
			}
			command_fail(si, fault_toomany, _("Logged in nicks are: %s"), lau);
			logcommand(si, CMDLOG_LOGIN, "failed " COMMAND_UC " to \2%s\2 (too many logins)", entity(mu)->name);
			return;
		}

		/* if they are identified to another account, nuke their session first */
		if (u->myuser)
		{
			if (ircd_on_logout(u, entity(u->myuser)->name))
				/* logout killed the user... */
				return;
		        u->myuser->lastlogin = CURRTIME;
		        LIST_FOREACH_SAFE(n, tn, u->myuser->logins.head)
		        {
			        if (n->data == u)
		                {
		                        node_del(n, &u->myuser->logins);
		                        node_free(n);
		                        break;
		                }
		        }
		        u->myuser = NULL;
		}

		command_success_nodata(si, nicksvs.no_nick_ownership ? _("You are now logged in as \2%s\2.") : _("You are now identified for \2%s\2."), entity(mu)->name);

		myuser_login(si->service, u, mu, true);
		logcommand(si, CMDLOG_LOGIN, COMMAND_UC);

		/* now we get to check for xOP */
		/* we don't check for host access yet (could match different
		 * entries because of services cloaks) */
		LIST_FOREACH(n, entity(mu)->chanacs.head)
		{
			ca = (chanacs_t *)n->data;

			cu = chanuser_find(ca->mychan->chan, u);
			if (cu && chansvs.me != NULL)
			{
				if (ca->level & CA_AKICK && !(ca->level & CA_REMOVE))
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

				if (ca->mychan->flags & MC_NOOP || mu->flags & MU_NOOP)
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

		return;
	}

	logcommand(si, CMDLOG_LOGIN, "failed " COMMAND_UC " to \2%s\2 (bad password)", entity(mu)->name);

	command_fail(si, fault_authfail, _("Invalid password for \2%s\2."), entity(mu)->name);
	bad_password(si, mu);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
