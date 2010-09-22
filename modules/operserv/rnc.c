/*
 * Copyright (c) 2006 Robin Burchell <surreal.w00t@gmail.com>
 * Rights to this code are documented in doc/LICENCE.
 *
 * This file contains functionality implementing OperServ RNC.
 *
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/rnc", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Robin Burchell <surreal.w00t@gmail.com>"
);

static void os_cmd_rnc(sourceinfo_t *si, int parc, char *parv[]);

command_t os_rnc = { "RNC", N_("Shows the most frequent realnames on the network"), PRIV_USER_AUSPEX, 1, os_cmd_rnc, { .path = "oservice/rnc" } };

typedef struct rnc_t_ rnc_t;
struct rnc_t_
{
	const char *gecos;
	int count;
};

void _modinit(module_t *m)
{
	service_named_bind_command("operserv", &os_rnc);
}

void _moddeinit(void)
{
	service_named_unbind_command("operserv", &os_rnc);
}

static void os_cmd_rnc(sourceinfo_t *si, int parc, char *parv[])
{
	char *param = parv[0];
	int count = param ? atoi(param) : 20;
	user_t *u;
	rnc_t *rnc, *biggest;
	mowgli_patricia_t *realnames;
	int i, found = 0;
	mowgli_patricia_iteration_state_t state;

	realnames = mowgli_patricia_create(noopcanon);

	MOWGLI_PATRICIA_FOREACH(u, &state, userlist)
	{
		rnc = mowgli_patricia_retrieve(realnames, u->gecos);
		if (rnc != NULL)
			rnc->count++;
		else
		{
			rnc = malloc(sizeof(rnc_t));
			rnc->gecos = u->gecos;
			rnc->count = 1;
			mowgli_patricia_add(realnames, rnc->gecos, rnc);
		}
	}

	/* this is ugly to the max :P */
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

		command_success_nodata(si, _("\2%d\2: \2%d\2 matches for realname \2%s\2"), i, biggest->count, biggest->gecos);
		mowgli_patricia_delete(realnames, biggest->gecos);
		free(biggest);
	}

	/* cleanup */
	MOWGLI_PATRICIA_FOREACH(rnc, &state, realnames)
	{
		mowgli_patricia_delete(realnames, rnc->gecos);
		free(rnc);
	}
	mowgli_patricia_destroy(realnames, NULL, NULL);

	logcommand(si, CMDLOG_ADMIN, "RNC: \2%d\2", count);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
