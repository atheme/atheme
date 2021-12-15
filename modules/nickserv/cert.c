/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2006-2016 Atheme Project (http://atheme.org/)
 *
 * Changes and shows nickname CertFP authentication lists.
 */

#include <atheme.h>

static void
ns_cmd_cert(struct sourceinfo *si, int parc, char *parv[])
{
	struct myuser *mu;
	mowgli_node_t *n, *tn;
	char *mcfp;
	struct mycertfp *cert;

	if (parc < 1)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "CERT");
		command_fail(si, fault_needmoreparams, _("Syntax: CERT ADD|DEL|LIST|CLEAR [fingerprint]"));
		return;
	}

	if (!strcasecmp(parv[0], "CLEAR"))
	{
		if (parc < 2)
		{
			mu = si->smu;
			if (mu == NULL)
			{
				command_fail(si, fault_noprivs, STR_NOT_LOGGED_IN);
				return;
			}
		}
		else
		{
			if (!has_priv(si, PRIV_USER_ADMIN))
			{
				command_fail(si, fault_noprivs, _("You are not authorized to use the target argument."));
				return;
			}

			if (!(mu = myuser_find_ext(parv[1])))
			{
				command_fail(si, fault_badparams, STR_IS_NOT_REGISTERED, parv[1]);
				return;
			}
		}

		if (mu != si->smu)
			logcommand(si, CMDLOG_ADMIN, "CERT:CLEAR: \2%s\2", entity(mu)->name);
		else
			logcommand(si, CMDLOG_SET, "CERT:CLEAR");

		command_success_nodata(si, _("Clearing all fingerprints for \2%s\2:"), entity(mu)->name);

		MOWGLI_ITER_FOREACH_SAFE(n, tn, mu->cert_fingerprints.head)
		{
			mycertfp_delete((struct mycertfp *) n->data);
		}

		command_success_nodata(si, _("All fingerprints for \2%s\2 have been removed."), entity(mu)->name);
		return;
	}

	if (!strcasecmp(parv[0], "LIST"))
	{
		if (parc < 2)
		{
			mu = si->smu;
			if (mu == NULL)
			{
				command_fail(si, fault_noprivs, STR_NOT_LOGGED_IN);
				return;
			}
		}
		else
		{
			if (!has_priv(si, PRIV_USER_AUSPEX))
			{
				command_fail(si, fault_noprivs, _("You are not authorized to use the target argument."));
				return;
			}

			if (!(mu = myuser_find_ext(parv[1])))
			{
				command_fail(si, fault_badparams, STR_IS_NOT_REGISTERED, parv[1]);
				return;
			}
		}

		if (mu != si->smu)
			logcommand(si, CMDLOG_ADMIN, "CERT:LIST: \2%s\2", entity(mu)->name);
		else
			logcommand(si, CMDLOG_GET, "CERT:LIST");

		command_success_nodata(si, _("Fingerprint list for \2%s\2:"), entity(mu)->name);

		MOWGLI_ITER_FOREACH(n, mu->cert_fingerprints.head)
		{
			mcfp = ((struct mycertfp *) n->data)->certfp;
			command_success_nodata(si, "- %s", mcfp);
		}

		command_success_nodata(si, _("End of \2%s\2 fingerprint list."), entity(mu)->name);
	}
	else if (!strcasecmp(parv[0], "ADD"))
	{
		mu = si->smu;
		if (parc < 2)
		{
			mcfp = si->su != NULL ? si->su->certfp : NULL;

			if (mcfp == NULL)
			{
				command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "CERT ADD");
				command_fail(si, fault_needmoreparams, _("Syntax: CERT ADD <fingerprint>"));
				return;
			}
		}
		else
		{
			mcfp = parv[1];
		}

		if (mu == NULL)
		{
			command_fail(si, fault_noprivs, STR_NOT_LOGGED_IN);
			return;
		}

		cert = mycertfp_find(mcfp);
		if (cert == NULL)
			;
		else if (cert->mu == mu)
		{
			command_fail(si, fault_nochange, _("Fingerprint \2%s\2 is already on your fingerprint list."), mcfp);
			return;
		}
		else
		{
			command_fail(si, fault_nochange, _("Fingerprint \2%s\2 is already on another user's fingerprint list."), mcfp);
			return;
		}
		if (mycertfp_add(mu, mcfp, false))
		{
			command_success_nodata(si, _("Added fingerprint \2%s\2 to your fingerprint list."), mcfp);
			logcommand(si, CMDLOG_SET, "CERT:ADD: \2%s\2", mcfp);
		}
		else
			command_fail(si, fault_toomany, _("Your fingerprint list is full."));
	}
	else if (!strcasecmp(parv[0], "DEL"))
	{
		if (parc < 2)
		{
			command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "CERT DEL");
			command_fail(si, fault_needmoreparams, _("Syntax: CERT DEL <fingerprint>"));
			return;
		}
		mu = si->smu;
		if (mu == NULL)
		{
			command_fail(si, fault_noprivs, STR_NOT_LOGGED_IN);
			return;
		}
		cert = mycertfp_find(parv[1]);
		if (cert == NULL || cert->mu != mu)
		{
			command_fail(si, fault_nochange, _("Fingerprint \2%s\2 is not on your fingerprint list."), parv[1]);
			return;
		}
		command_success_nodata(si, _("Deleted fingerprint \2%s\2 from your fingerprint list."), parv[1]);
		logcommand(si, CMDLOG_SET, "CERT:DEL: \2%s\2", parv[1]);
		mycertfp_delete(cert);
	}
	else
	{
		command_fail(si, fault_needmoreparams, STR_INVALID_PARAMS, "CERT");
		command_fail(si, fault_needmoreparams, _("Syntax: CERT ADD|DEL|LIST|CLEAR [fingerprint]"));
		return;
	}
}

static struct command ns_cert = {
	.name           = "CERT",
	.desc           = N_("Changes and shows your nickname CertFP authentication list."),
	.access         = AC_NONE,
	.maxparc        = 2,
	.cmd            = &ns_cmd_cert,
	.help           = { .path = "nickserv/cert" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "nickserv/main")

	service_named_bind_command("nickserv", &ns_cert);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("nickserv", &ns_cert);
}

SIMPLE_DECLARE_MODULE_V1("nickserv/cert", MODULE_UNLOAD_CAPABILITY_OK)
