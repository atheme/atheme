/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2006 Robin Burchell <surreal.w00t@gmail.com>
 *
 * This file contains functionality implementing OperServ RNC.
 */

#include <atheme.h>

struct rnc
{
	const char *gecos;
	unsigned int count;
};

static void
os_cmd_rnc(struct sourceinfo *si, int parc, char *parv[])
{
	char *param = parv[0];
	unsigned int count = 20;

	if (param && ! string_to_uint(param, &count))
		count = 20;

	struct user *u;
	struct rnc *rnc, *biggest;
	mowgli_patricia_t *realnames;
	unsigned int i, found = 0;
	mowgli_patricia_iteration_state_t state;

	realnames = mowgli_patricia_create(noopcanon);

	MOWGLI_PATRICIA_FOREACH(u, &state, userlist)
	{
		rnc = mowgli_patricia_retrieve(realnames, u->gecos);
		if (rnc != NULL)
			rnc->count++;
		else
		{
			rnc = smalloc(sizeof *rnc);
			rnc->gecos = u->gecos;
			rnc->count = 1;
			mowgli_patricia_add(realnames, rnc->gecos, rnc);
		}
	}

	// this is ugly to the max :P
	for (i = 1; i <= count; i++)
	{
		found = 0;
		biggest = NULL;
		MOWGLI_PATRICIA_FOREACH(rnc, &state, realnames)
		{
			if (rnc->count > found)
			{
				found = rnc->count;
				biggest = rnc;
			}
		}

		if (biggest == NULL)
			break;

		command_success_nodata(si, ngettext(N_("\2%u\2: \2%u\2 match for realname \2%s\2"),
		                                    N_("\2%u\2: \2%u\2 matches for realname \2%s\2"),
		                                    biggest->count), i, biggest->count, biggest->gecos);

		mowgli_patricia_delete(realnames, biggest->gecos);
		sfree(biggest);
	}

	// cleanup
	MOWGLI_PATRICIA_FOREACH(rnc, &state, realnames)
	{
		mowgli_patricia_delete(realnames, rnc->gecos);
		sfree(rnc);
	}
	mowgli_patricia_destroy(realnames, NULL, NULL);

	logcommand(si, CMDLOG_ADMIN, "RNC: \2%u\2", count);
}

static struct command os_rnc = {
	.name           = "RNC",
	.desc           = N_("Shows the most frequent realnames on the network"),
	.access         = PRIV_USER_AUSPEX,
	.maxparc        = 1,
	.cmd            = &os_cmd_rnc,
	.help           = { .path = "oservice/rnc" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "operserv/main")

	service_named_bind_command("operserv", &os_rnc);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("operserv", &os_rnc);
}

SIMPLE_DECLARE_MODULE_V1("operserv/rnc", MODULE_UNLOAD_CAPABILITY_OK)
