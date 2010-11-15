/*
 * Copyright (c) 2005 William Pitcock <nenolod -at- nenolod.net>
 * Copyright (c) 2007 Jilles Tjoelker
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Changes your e-mail address.
 *
 */

#include "atheme.h"
#include "uplink.h"

DECLARE_MODULE_V1
(
	"nickserv/set_email", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

mowgli_patricia_t **ns_set_cmdtree;

static void ns_cmd_set_email(sourceinfo_t *si, int parc, char *parv[]);

command_t ns_set_email = { "EMAIL", N_("Changes your e-mail address."), AC_NONE, 1, ns_cmd_set_email, { .path = "nickserv/set_email" } };

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(ns_set_cmdtree, "nickserv/set_core", "ns_set_cmdtree");

	command_add(&ns_set_email, *ns_set_cmdtree);
}

void _moddeinit(void)
{
	command_delete(&ns_set_email, *ns_set_cmdtree);
}

/* SET EMAIL <new address> */
static void ns_cmd_set_email(sourceinfo_t *si, int parc, char *parv[])
{
	char *email = parv[0];
	metadata_t *md;

	if (si->smu == NULL)
		return;

	if (email == NULL)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "EMAIL");
		command_fail(si, fault_needmoreparams, _("Syntax: SET EMAIL <new e-mail>"));
		return;
	}

	if (si->smu->flags & MU_WAITAUTH)
	{
		command_fail(si, fault_noprivs, _("Please verify your original registration before changing your e-mail address."));
		return;
	}

	if (!strcasecmp(si->smu->email, email))
	{
		md = metadata_find(si->smu, "private:verify:emailchg:newemail");
		if (md != NULL)
		{
			command_success_nodata(si, _("The email address change to \2%s\2 has been cancelled."), md->value);
			metadata_delete(si->smu, "private:verify:emailchg:key");
			metadata_delete(si->smu, "private:verify:emailchg:newemail");
			metadata_delete(si->smu, "private:verify:emailchg:timestamp");
		}
		else
			command_fail(si, fault_nochange, _("The email address for account \2%s\2 is already set to \2%s\2."), entity(si->smu)->name, si->smu->email);
		return;
	}

	if (!validemail(email))
	{
		command_fail(si, fault_badparams, _("\2%s\2 is not a valid email address."), email);
		return;
	}

	if (me.auth == AUTH_EMAIL)
	{
		unsigned long key = makekey();

		metadata_add(si->smu, "private:verify:emailchg:key", number_to_string(key));
		metadata_add(si->smu, "private:verify:emailchg:newemail", email);
		metadata_add(si->smu, "private:verify:emailchg:timestamp", number_to_string(CURRTIME));

		if (!sendemail(si->su != NULL ? si->su : si->service->me, EMAIL_SETEMAIL, si->smu, number_to_string(key)))
		{
			command_fail(si, fault_emailfail, _("Sending email failed, sorry! Your email address is unchanged."));
			metadata_delete(si->smu, "private:verify:emailchg:key");
			metadata_delete(si->smu, "private:verify:emailchg:newemail");
			metadata_delete(si->smu, "private:verify:emailchg:timestamp");
			return;
		}

		logcommand(si, CMDLOG_SET, "SET:EMAIL: \2%s\2 (\2%s\2 -> \2%s\2) (awaiting verification)", entity(si->smu)->name, si->smu->email, email);
		command_success_nodata(si, _("An email containing email changing instructions has been sent to \2%s\2."), email);
		command_success_nodata(si, _("Your email address will not be changed until you follow these instructions."));

		return;
	}

	myuser_set_email(si->smu, email);

	logcommand(si, CMDLOG_SET, "SET:EMAIL: \2%s\2 (\2%s\2 -> \2%s\2)", entity(si->smu)->name, si->smu->email, email);
	command_success_nodata(si, _("The email address for account \2%s\2 has been changed to \2%s\2."), entity(si->smu)->name, si->smu->email);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
