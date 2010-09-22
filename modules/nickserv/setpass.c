/*
 * Copyright (c) 2007 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the NickServ SETPASS function.
 *
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"nickserv/setpass", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void clear_setpass_key(user_t *u);
static void ns_cmd_setpass(sourceinfo_t *si, int parc, char *parv[]);

command_t ns_setpass = { "SETPASS", N_("Changes a password using an authcode."), AC_NONE, 3, ns_cmd_setpass, { .path = "nickserv/setpass" } };

void _modinit(module_t *m)
{
	hook_add_event("user_identify");
	hook_add_user_identify(clear_setpass_key);
	service_named_bind_command("nickserv", &ns_setpass);
}

void _moddeinit()
{
	hook_del_user_identify(clear_setpass_key);
	service_named_unbind_command("nickserv", &ns_setpass);
}

static void ns_cmd_setpass(sourceinfo_t *si, int parc, char *parv[])
{
	myuser_t *mu;
	metadata_t *md;
	char *nick = parv[0];
	char *key = parv[1];
	char *password = parv[2];

	if (!nick || !key || !password)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SETPASS");
		command_fail(si, fault_needmoreparams, _("Syntax: SETPASS <account> <key> <newpass>"));
		return;
	}

	if (strchr(password, ' '))
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "SETPASS");
		command_fail(si, fault_badparams, _("Syntax: SETPASS <account> <key> <newpass>"));
		return;
	}

	if (!(mu = myuser_find(nick)))
	{
		command_fail(si, fault_nosuch_target, _("\2%s\2 is not registered."), nick);
		return;
	}

	if (strlen(password) > 32)
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "SETPASS");
		return;
	}

	if (!strcasecmp(password, entity(mu)->name))
	{
		command_fail(si, fault_badparams, _("You cannot use your nickname as a password."));
		command_fail(si, fault_badparams, _("Syntax: SETPASS <account> <key> <newpass>"));
		return;
	}

	md = metadata_find(mu, "private:setpass:key");
	if (md != NULL && crypt_verify_password(key, md->value))
	{
		logcommand(si, CMDLOG_SET, "SETPASS: \2%s\2", entity(mu)->name);
		set_password(mu, password);
		metadata_delete(mu, "private:setpass:key");

		command_success_nodata(si, _("The password for \2%s\2 has been changed to \2%s\2."), entity(mu)->name, password);

		return;
	}

	if (md != NULL)
	{
		logcommand(si, CMDLOG_SET, "failed SETPASS (invalid key)");
	}
	command_fail(si, fault_badparams, _("Verification failed. Invalid key for \2%s\2."), 
		entity(mu)->name);

	return;
}

static void clear_setpass_key(user_t *u)
{
	myuser_t *mu = u->myuser;

	if (!metadata_find(mu, "private:setpass:key"))
		return;

	metadata_delete(mu, "private:setpass:key");
	notice(nicksvs.nick, u->nick, "Warning: SENDPASS had been used to mail you a password recovery "
		"key. Since you have identified, that key is no longer valid.");
}
