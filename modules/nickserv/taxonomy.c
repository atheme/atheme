/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Lists object properties via their metadata table.
 *
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"nickserv/taxonomy", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void ns_cmd_taxonomy(sourceinfo_t *si, int parc, char *parv[]);

command_t ns_taxonomy = { "TAXONOMY", N_("Displays a user's metadata."), AC_NONE, 1, ns_cmd_taxonomy, { .path = "nickserv/taxonomy" } };

void _modinit(module_t *m)
{
	service_named_bind_command("nickserv", &ns_taxonomy);
}

void _moddeinit()
{
	service_named_unbind_command("nickserv", &ns_taxonomy);
}

static void ns_cmd_taxonomy(sourceinfo_t *si, int parc, char *parv[])
{
	char *target = parv[0];
	myuser_t *mu;
	mowgli_node_t *n;
	bool isoper;

	if (!target && si->smu)
		target = entity(si->smu)->name;
	if (!target)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "TAXONOMY");
		command_fail(si, fault_needmoreparams, _("Syntax: TAXONOMY <nick>"));
		return;
	}

	if (!(mu = myuser_find_ext(target)))
	{
		command_fail(si, fault_badparams, _("\2%s\2 is not registered."), target);
		return;
	}

	isoper = has_priv(si, PRIV_USER_AUSPEX);
	if (isoper)
		logcommand(si, CMDLOG_ADMIN, "TAXONOMY: \2%s\2 (oper)", entity(mu)->name);
	else
		logcommand(si, CMDLOG_GET, "TAXONOMY: \2%s\2", entity(mu)->name);

	command_success_nodata(si, _("Taxonomy for \2%s\2:"), entity(mu)->name);

	MOWGLI_ITER_FOREACH(n, object(mu)->metadata.head)
	{
		metadata_t *md = n->data;

		if (md->private && !isoper)
			continue;

		command_success_nodata(si, "%-32s: %s", md->name, md->value);
	}

	command_success_nodata(si, _("End of \2%s\2 taxonomy."), entity(mu)->name);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
