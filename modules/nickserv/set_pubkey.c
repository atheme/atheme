/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005 William Pitcock <nenolod -at- nenolod.net>
 * Copyright (C) 2007 Jilles Tjoelker
 * Copyright (C) 2014 Zohlai Development Group
 *
 * Sets the public key (for SASL ECDSA-NIST256P-CHALLENGE) of a user.
 */

#include <atheme.h>

#ifdef HAVE_LIBCRYPTO_ECDSA

static mowgli_patricia_t **ns_set_cmdtree = NULL;

// SET PUBKEY <key>
static void
ns_cmd_set_pubkey(struct sourceinfo *si, int parc, char *parv[])
{
	char *newkey = parv[0];
	struct metadata *md;

	if (!newkey)
	{
		md = metadata_find(si->smu, "private:pubkey");

		if (!md)
		{
			command_fail(si, fault_nochange, _("Your public key was not set."));
			return;
		}

		metadata_delete(si->smu, "private:pubkey");
		logcommand(si, CMDLOG_SET, "SET:PUBKEY:REMOVE");
		command_success_nodata(si, _("Your public key entry has been deleted."));
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

static struct command ns_set_pubkey = {
	.name           = "PUBKEY",
	.desc           = N_("Changes your ECDSA-NIST256P-CHALLENGE public key."),
	.access         = AC_NONE,
	.maxparc        = 1,
	.cmd            = &ns_cmd_set_pubkey,
	.help           = { .path = "nickserv/set_pubkey" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_SYMBOL(m, ns_set_cmdtree, "nickserv/set_core", "ns_set_cmdtree")

	command_add(&ns_set_pubkey, *ns_set_cmdtree);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	command_delete(&ns_set_pubkey, *ns_set_cmdtree);
}

#else /* HAVE_LIBCRYPTO_ECDSA */

static void
mod_init(struct module *const restrict m)
{
	(void) slog(LG_ERROR, "Module %s requires OpenSSL ECDSA support, refusing to load.", m->name);

	m->mflags |= MODFLAG_FAIL;
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	// Nothing To Do
}

#endif /* !HAVE_LIBCRYPTO_ECDSA */

SIMPLE_DECLARE_MODULE_V1("nickserv/set_pubkey", MODULE_UNLOAD_CAPABILITY_OK)
