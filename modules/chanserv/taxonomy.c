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
	"chanserv/taxonomy", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_taxonomy(sourceinfo_t *si, int parc, char *parv[]);

command_t cs_taxonomy = { "TAXONOMY", N_("Displays a channel's metadata."),
                        AC_NONE, 1, cs_cmd_taxonomy, { .path = "cservice/taxonomy" } };
                                                                                   
void _modinit(module_t *m)
{
	service_named_bind_command("chanserv", &cs_taxonomy);
}

void _moddeinit()
{
	service_named_unbind_command("chanserv", &cs_taxonomy);
}

void cs_cmd_taxonomy(sourceinfo_t *si, int parc, char *parv[])
{
	char *target = parv[0];
	mychan_t *mc;
	mowgli_node_t *n;
	bool isoper;

	if (!target || *target != '#')
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "TAXONOMY");
		command_fail(si, fault_needmoreparams, _("Syntax: TAXONOMY <#channel>"));
		return;
	}

	if (!(mc = mychan_find(target)))
	{
		command_fail(si, fault_nosuch_target, _("\2%s\2 is not a registered channel."), target);
		return;
	}

	isoper = has_priv(si, PRIV_CHAN_AUSPEX);

	if (use_channel_private && mc->flags & MC_PRIVATE &&
			!chanacs_source_has_flag(mc, si, CA_ACLVIEW) && !isoper)
	{
		command_fail(si, fault_noprivs, _("Channel \2%s\2 is private."),
				mc->name);
		return;
	}

	if (isoper)
		logcommand(si, CMDLOG_ADMIN, "TAXONOMY: \2%s\2 (oper)", mc->name);
	else
		logcommand(si, CMDLOG_GET, "TAXONOMY: \2%s\2", mc->name);
	command_success_nodata(si, _("Taxonomy for \2%s\2:"), target);

	MOWGLI_ITER_FOREACH(n, object(mc)->metadata.head)
	{
		metadata_t *md = n->data;

                if (md->private && !isoper)
                        continue;

		command_success_nodata(si, "%-32s: %s", md->name, md->value);
	}

	command_success_nodata(si, _("End of \2%s\2 taxonomy."), target);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
