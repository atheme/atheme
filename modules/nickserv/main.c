/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains the main() routine.
 *
 */

#include "atheme.h"
#include "conf.h" /* XXX conf_ni_table */
#include <limits.h>

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

		if (!(u->flags & UF_SEENINFO) && chansvs.me != NULL)
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

static int c_ni_emailexempts(mowgli_config_file_entry_t *ce)
{
	mowgli_config_file_entry_t *subce;
	mowgli_node_t *n, *tn;

	if (!ce->entries)
		return 0;

	MOWGLI_ITER_FOREACH_SAFE(n, tn, nicksvs.emailexempts.head)
	{
		free(n->data);
		mowgli_node_delete(n, &nicksvs.emailexempts);
		mowgli_node_free(n);
	}

	MOWGLI_ITER_FOREACH(subce, ce->entries)
	{
		if (subce->entries != NULL)
		{
			conf_report_warning(ce, "Invalid email exempt entry");
			continue;
		}

		mowgli_node_add(sstrdup(subce->varname), mowgli_node_create(), &nicksvs.emailexempts);
	}

	return 0;
}

void _modinit(module_t *m)
{
        hook_add_event("config_ready");
        hook_add_config_ready(nickserv_config_ready);

        hook_add_event("nick_check");
        hook_add_nick_check(nickserv_handle_nickchange);

	nicksvs.me = service_add("nickserv", NULL);
	authservice_loaded++;
	
	add_bool_conf_item("SPAM", &nicksvs.me->conf_table, 0, &nicksvs.spam, false);
	add_bool_conf_item("NO_NICK_OWNERSHIP", &nicksvs.me->conf_table, 0, &nicksvs.no_nick_ownership, false);
	add_duration_conf_item("EXPIRE", &nicksvs.me->conf_table, 0, &nicksvs.expiry, "d", 0);
	add_duration_conf_item("ENFORCE_EXPIRE", &nicksvs.me->conf_table, 0, &nicksvs.enforce_expiry, "d", 0);
	add_duration_conf_item("ENFORCE_DELAY", &nicksvs.me->conf_table, 0, &nicksvs.enforce_delay, "s", 30);
	add_dupstr_conf_item("ENFORCE_PREFIX", &nicksvs.me->conf_table, 0, &nicksvs.enforce_prefix, "Guest");
	add_dupstr_conf_item("CRACKLIB_DICT", &nicksvs.me->conf_table, 0, &nicksvs.cracklib_dict, NULL);
	add_conf_item("EMAILEXEMPTS", &nicksvs.me->conf_table, c_ni_emailexempts);
	add_uint_conf_item("MAXNICKS", &nicksvs.me->conf_table, 9, &nicksvs.maxnicks, 1, INT_MAX, 5);
}

void _moddeinit(module_unload_intent_t intent)
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
