/*
 * Copyright (c) 2005 Alex Lambert
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Implements nickserv RETURN.
 *
 */

#include "atheme.h"
#include "authcookie.h"

DECLARE_MODULE_V1
(
	"nickserv/return", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void ns_cmd_return(sourceinfo_t *si, int parc, char *parv[]);

command_t ns_return = { "RETURN", N_("Returns an account to its owner."), PRIV_USER_ADMIN, 2, ns_cmd_return };

list_t *ns_cmdtree, *ns_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(ns_cmdtree, "nickserv/main", "ns_cmdtree");
	MODULE_USE_SYMBOL(ns_helptree, "nickserv/main", "ns_helptree");

	command_add(&ns_return, ns_cmdtree);
	help_addentry(ns_helptree, "RETURN", "help/nickserv/return", NULL);
}

void _moddeinit()
{
	command_delete(&ns_return, ns_cmdtree);
	help_delentry(ns_helptree, "RETURN");
}

static void ns_cmd_return(sourceinfo_t *si, int parc, char *parv[])
{
	char *target = parv[0];
	char *newmail = parv[1];
	char *newpass;
	char oldmail[EMAILLEN];
	myuser_t *mu;
	user_t *u;
	node_t *n, *tn;

	if (!target || !newmail)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "RETURN");
		command_fail(si, fault_needmoreparams, _("Usage: RETURN <account> <e-mail address>"));
		return;
	}

	if (!(mu = myuser_find(target)))
	{
		command_fail(si, fault_nosuch_target, _("\2%s\2 is not registered."), target);
		return;
	}

	if (is_soper(mu))
	{
		logcommand(si, CMDLOG_ADMIN, "failed RETURN \2%s\2 to \2%s\2 (is SOPER)", target, newmail);
		command_fail(si, fault_badparams, _("\2%s\2 belongs to a services operator; it cannot be returned."), target);
		return;
	}

	if (!validemail(newmail))
	{
		command_fail(si, fault_badparams, _("\2%s\2 is not a valid e-mail address."), newmail);
		return;
	}

	newpass = gen_pw(12);
	strlcpy(oldmail, mu->email, EMAILLEN);
	myuser_set_email(mu, newmail);

	if (!sendemail(si->su != NULL ? si->su : si->service->me, EMAIL_SENDPASS, mu, newpass))
	{
		myuser_set_email(mu, oldmail);
		command_fail(si, fault_emailfail, _("Sending email failed, account \2%s\2 remains with \2%s\2."),
				entity(mu)->name, mu->email);
		return;
	}

	set_password(mu, newpass);

	free(newpass);

	/* prevents users from "stealing it back" in the event of a takeover */
	metadata_delete(mu, "private:verify:emailchg:key");
	metadata_delete(mu, "private:verify:emailchg:newemail");
	metadata_delete(mu, "private:verify:emailchg:timestamp");
	metadata_delete(mu, "private:setpass:key");
	/* log them out */
	LIST_FOREACH_SAFE(n, tn, mu->logins.head)
	{
		u = (user_t *)n->data;
		if (!ircd_on_logout(u, entity(mu)->name))
		{
			u->myuser = NULL;
			node_del(n, &mu->logins);
			node_free(n);
		}
	}
	mu->flags |= MU_NOBURSTLOGIN;
	authcookie_destroy_all(mu);

	wallops("%s returned the account \2%s\2 to \2%s\2", get_oper_name(si), target, newmail);
	logcommand(si, CMDLOG_ADMIN | LG_REGISTER, "RETURN: \2%s\2 to \2%s\2", target, newmail);
	command_success_nodata(si, _("The e-mail address for \2%s\2 has been set to \2%s\2"),
						target, newmail);
	command_success_nodata(si, _("A random password has been set; it has been sent to \2%s\2."),
						newmail);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
