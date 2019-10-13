/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005 William Pitcock <nenolod -at- nenolod.net>
 * Copyright (C) 2007 Jilles Tjoelker
 *
 * Changes your e-mail address.
 */

#include <atheme.h>

static mowgli_patricia_t **ns_set_cmdtree = NULL;

// SET EMAIL <new address>
static void
ns_cmd_set_email(struct sourceinfo *si, int parc, char *parv[])
{
	char *email = parv[0];
	struct metadata *md;

	if (!email)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SET EMAIL");
		command_fail(si, fault_needmoreparams, _("Syntax: SET EMAIL <new e-mail>"));
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
		command_fail(si, fault_badparams, _("\2%s\2 is not a valid e-mail address."), email);
		return;
	}

	if (!email_within_limits(email))
	{
		command_fail(si, fault_toomany, _("\2%s\2 has too many accounts registered."), email);
		return;
	}

	if (me.auth == AUTH_EMAIL)
	{
		unsigned long key = makekey();

		metadata_add(si->smu, "private:verify:emailchg:key", number_to_string(key));
		metadata_add(si->smu, "private:verify:emailchg:newemail", email);
		metadata_add(si->smu, "private:verify:emailchg:timestamp", number_to_string(CURRTIME));

		if (!sendemail(si->su != NULL ? si->su : si->service->me, si->smu, EMAIL_SETEMAIL, email, number_to_string(key)))
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

	logcommand(si, CMDLOG_SET, "SET:EMAIL: \2%s\2 (\2%s\2 -> \2%s\2)", entity(si->smu)->name, si->smu->email, email);
	myuser_set_email(si->smu, email);
	command_success_nodata(si, _("The email address for account \2%s\2 has been changed to \2%s\2."), entity(si->smu)->name, si->smu->email);
}

static struct command ns_set_email = {
	.name           = "EMAIL",
	.desc           = N_("Changes your e-mail address."),
	.access         = AC_NONE,
	.maxparc        = 1,
	.cmd            = &ns_cmd_set_email,
	.help           = { .path = "nickserv/set_email" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_SYMBOL(m, ns_set_cmdtree, "nickserv/set_core", "ns_set_cmdtree")

	command_add(&ns_set_email, *ns_set_cmdtree);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	command_delete(&ns_set_email, *ns_set_cmdtree);
}

SIMPLE_DECLARE_MODULE_V1("nickserv/set_email", MODULE_UNLOAD_CAPABILITY_OK)
