/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains the main() routine.
 *
 */

#include "atheme.h"
#include "conf.h" /* XXX conf_ni_table */

DECLARE_MODULE_V1
(
	"nickserv/main", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

struct
{
	const char *nickstring, *accountstring;
} nick_account_trans[] =
{
	/* command descriptions */
	{ "Reclaims use of a nickname.", N_("Disconnects an old session.") },
	{ "Registers a nickname.", N_("Registers an account.") },
	{ "Lists nicknames registered matching a given pattern.", N_("Lists accounts matching a given pattern.") },

	/* messages */
	{ "\2%s\2 is not a registered nickname.", N_("\2%s\2 is not a registered account.") },
	{ "Syntax: INFO <nickname>", N_("Syntax: INFO <account>") },
	{ "No nicknames matched pattern \2%s\2", N_("No accounts matched pattern \2%s\2") },
	{ "An email containing nickname activation instructions has been sent to \2%s\2.", N_("An email containing account activation instructions has been sent to \2%s\2.") },
	{ "If you do not complete registration within one day your nickname will expire.", N_("If you do not complete registration within one day your account will expire.") },
	{ "%s registered the nick \2%s\2 and gained services operator privileges.", N_("%s registered the account \2%s\2 and gained services operator privileges.") },
	{ "You cannot use your nickname as a password.", N_("You cannot use your account name as a password.") },
	{ NULL, NULL }
};

static void nickserv_handle_nickchange(user_t *u)
{
	mynick_t *mn;
	hook_nick_enforce_t hdata;

	if (nicksvs.me == NULL || nicksvs.no_nick_ownership)
		return;

	/* They're logged in, don't send them spam -- jilles */
	if (u->myuser)
		u->flags |= UF_SEENINFO;

	/* Also don't send it if they came back from a split -- jilles */
	if (!(u->server->flags & SF_EOB))
		u->flags |= UF_SEENINFO;

	if (!(mn = mynick_find(u->nick)))
	{
		if (!nicksvs.spam)
			return;

		if (!(u->flags & UF_SEENINFO))
		{
			notice(nicksvs.nick, u->nick, "Welcome to %s, %s! Here on %s, we provide services to enable the "
			       "registration of nicknames and channels! For details, type \2/%s%s help\2 and \2/%s%s help\2.",
			       me.netname, u->nick, me.netname, (ircd->uses_rcommand == false) ? "msg " : "", nicksvs.me->disp, (ircd->uses_rcommand == false) ? "msg " : "", chansvs.me->disp);

			u->flags |= UF_SEENINFO;
		}

		return;
	}

	if (u->myuser == mn->owner)
	{
		mn->lastseen = CURRTIME;
		return;
	}

	/* OpenServices: is user on access list? -nenolod */
	if (myuser_access_verify(u, mn->owner))
	{
		notice(nicksvs.nick, u->nick, _("Please identify via \2/%s%s identify <password>\2."),
			(ircd->uses_rcommand == false) ? "msg " : "", nicksvs.me->disp);
		return;
	}

	notice(nicksvs.nick, u->nick, _("This nickname is registered. Please choose a different nickname, or identify via \2/%s%s identify <password>\2."),
		(ircd->uses_rcommand == false) ? "msg " : "", nicksvs.me->disp);
	hdata.u = u;
	hdata.mn = mn;
	hook_call_nick_enforce(&hdata);
}

static void nickserv_config_ready(void *unused)
{
	int i;

	nicksvs.nick = nicksvs.me->nick;
	nicksvs.user = nicksvs.me->user;
	nicksvs.host = nicksvs.me->host;
	nicksvs.real = nicksvs.me->real;

	if (nicksvs.no_nick_ownership)
		for (i = 0; nick_account_trans[i].nickstring != NULL; i++)
			itranslation_create(_(nick_account_trans[i].nickstring),
					_(nick_account_trans[i].accountstring));
	else
		for (i = 0; nick_account_trans[i].nickstring != NULL; i++)
			itranslation_destroy(_(nick_account_trans[i].nickstring));
}

void _modinit(module_t *m)
{
        hook_add_event("config_ready");
        hook_add_config_ready(nickserv_config_ready);

        hook_add_event("nick_check");
        hook_add_nick_check(nickserv_handle_nickchange);

	nicksvs.me = service_add("nickserv", NULL, &conf_ni_table);
	authservice_loaded++;
}

void _moddeinit(void)
{
        if (nicksvs.me)
	{
		nicksvs.nick = NULL;
		nicksvs.user = NULL;
		nicksvs.host = NULL;
		nicksvs.real = NULL;
                service_delete(nicksvs.me);
		nicksvs.me = NULL;
	}
	authservice_loaded--;

        hook_del_config_ready(nickserv_config_ready);
        hook_del_nick_check(nickserv_handle_nickchange);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
