/*
 * Copyright (c) 2005 William Pitcock <nenolod -at- nenolod.net>
 * Copyright (c) 2007 Jilles Tjoelker
 * Copyright (c) 2014 Zohlai Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Sets the public key (for SASL ECDSA-NIST256p-CHALLENGE) of a user.
 *
 */

#include "atheme.h"
#include "uplink.h"

DECLARE_MODULE_V1
(
	"nickserv/set_pubkey", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	VENDOR_STRING
);

mowgli_patricia_t **ns_set_cmdtree;

static void ns_cmd_set_pubkey(sourceinfo_t *si, int parc, char *parv[]);

command_t ns_set_pubkey = { "PUBKEY", N_("Changes your ECDSA-NIST256p-CHALLENGE public key."), AC_NONE, 1, ns_cmd_set_pubkey, { .path = "nickserv/set_pubkey" } };

void _modinit(module_t *m)
{
	MODULE_TRY_REQUEST_SYMBOL(m, ns_set_cmdtree, "nickserv/set_core", "ns_set_cmdtree");

	command_add(&ns_set_pubkey, *ns_set_cmdtree);
}

void _moddeinit(module_unload_intent_t intent)
{
	command_delete(&ns_set_pubkey, *ns_set_cmdtree);
}

/* SET PUBKEY <key> */
static void ns_cmd_set_pubkey(sourceinfo_t *si, int parc, char *parv[])
{
	char *newkey = parv[0];
	metadata_t *md;

	if (!newkey)
	{
		md = metadata_find(si->smu, "private:pubkey");

		if (!md)
		{
			command_fail(si, fault_nosuch_target, _("Public key was not set"));
			return;
		}

		metadata_delete(si->smu, "private:pubkey");
		logcommand(si, CMDLOG_SET, "SET:PUBKEY:REMOVE");
		command_success_nodata(si, _("Public key entry has been deleted."));
		return;
	}

	md = metadata_find(si->smu, "private:pubkey");
	if (md != NULL && !strcmp(md->value, newkey))
	{
		command_fail(si, fault_nochange, _("Your public key is already set to \2%s\2."), newkey);
		return;
	}

	metadata_add(si->smu, "private:pubkey", newkey);
	logcommand(si, CMDLOG_SET, "SET:PUBKEY: \2%s\2", newkey);
	command_success_nodata(si, _("Your public key is now set to \2%s\2."), newkey);
	return;
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
