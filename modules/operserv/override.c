/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2009 William Pitcock <nenolod@atheme.org>
 *
 * This file contains functionality which implements the OService INJECT command.
 */

#include "atheme.h"

struct override_sourceinfo
{
	struct sourceinfo si;
	struct sourceinfo *parent_si;
};

static void
override_command_fail(struct sourceinfo *si, enum cmd_faultcode code, const char *message)
{
	struct override_sourceinfo *csi = (struct override_sourceinfo *) si;

	return_if_fail(csi != NULL);

	command_fail(csi->parent_si, code, "%s", message);
}

static void
override_command_success_nodata(struct sourceinfo *si, const char *message)
{
	struct override_sourceinfo *csi = (struct override_sourceinfo *) si;

	return_if_fail(csi != NULL);

	command_success_nodata(csi->parent_si, "%s", message);
}

static void
override_command_success_string(struct sourceinfo *si, const char *result, const char *message)
{
	struct override_sourceinfo *csi = (struct override_sourceinfo *) si;

	return_if_fail(csi != NULL);

	command_success_string(csi->parent_si, result, "%s", message);
}

static const char *
override_get_source_name(struct sourceinfo *si)
{
	struct override_sourceinfo *csi = (struct override_sourceinfo *) si;

	return_val_if_fail(csi != NULL, NULL);

	return get_source_name(csi->parent_si);
}

static const char *
override_get_source_mask(struct sourceinfo *si)
{
	struct override_sourceinfo *csi = (struct override_sourceinfo *) si;

	return_val_if_fail(csi != NULL, NULL);

	return get_source_mask(csi->parent_si);
}

static const char *
override_get_oper_name(struct sourceinfo *si)
{
	struct override_sourceinfo *csi = (struct override_sourceinfo *) si;

	return_val_if_fail(csi != NULL, NULL);

	return get_oper_name(csi->parent_si);
}

static const char *
override_get_storage_oper_name(struct sourceinfo *si)
{
	struct override_sourceinfo *csi = (struct override_sourceinfo *) si;

	return_val_if_fail(csi != NULL, NULL);

	return get_storage_oper_name(csi->parent_si);
}

static void
override_sourceinfo_dispose(struct override_sourceinfo *o_si)
{
	atheme_object_unref(o_si->parent_si);
	sfree(o_si);
}

static int
text_to_parv(char *text, int maxparc, char **parv)
{
	int count = 0;
	char *p;

	if (maxparc == 0)
		return 0;

	if (!text)
		return 0;

	p = text;
	while (count < maxparc - 1 && (parv[count] = strtok(p, " ")) != NULL)
	{
		count++;
		p = NULL;
	}

	if ((parv[count] = strtok(p, "")) != NULL)
	{
		p = parv[count];

		while (*p == ' ')
			p++;
		parv[count] = p;

		if (*p != '\0')
		{
			p += strlen(p) - 1;

			while (*p == ' ' && p > parv[count])
				p--;

			p[1] = '\0';
			count++;
		}
	}

	return count;
}

static void
os_cmd_override(struct sourceinfo *si, int parc, char *parv[])
{
	static struct sourceinfo_vtable override_vtable = {
		.description = "override",
		.cmd_fail = override_command_fail,
		.cmd_success_nodata = override_command_success_nodata,
		.cmd_success_string = override_command_success_string,
		.get_source_name = override_get_source_name,
		.get_source_mask = override_get_source_mask,
		.get_oper_name = override_get_oper_name,
		.get_storage_oper_name = override_get_storage_oper_name,
	};

	struct myuser *mu = NULL;
	struct service *svs;
	struct service *memosvs;
	struct command *cmd;
	int newparc;
	size_t i;
	char *newparv[20];

	if (!parv[0] || !parv[1] || !parv[2])
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "OVERRIDE");
		command_fail(si, fault_needmoreparams, _("Syntax: OVERRIDE <account> <service> <command> [params]"));
		return;
	}

	if (*parv[0] == '#')
	{
		struct mychan *mc;
		mowgli_node_t *n;

		if (!(mc = mychan_find(parv[0])))
		{
			command_fail(si, fault_nosuch_target, _("\2%s\2 is not a registered channel."), parv[0]);
			return;
		}

		MOWGLI_ITER_FOREACH(n, mc->chanacs.head)
		{
			struct chanacs *ca = (struct chanacs *) n->data;

			if (ca->entity != NULL && isuser(ca->entity) && ca->level & CA_FOUNDER)
			{
				mu = user(ca->entity);
				break;
			}
		}

		// this should never happen, but we'll check anyway.
		if (mu == NULL)
		{
			slog(LG_DEBUG, "override: couldn't find a founder for %s!", parv[0]);
			command_fail(si, fault_nosuch_target, _("\2%s\2 doesn't have any founders."), parv[0]);
			return;
		}
	}
	else
	{
		if (!(mu = myuser_find(parv[0])))
		{
			command_fail(si, fault_nosuch_target, _("\2%s\2 is not a registered account."), parv[0]);
			return;
		}
	}

	svs = service_find_nick(parv[1]);
	if (svs == NULL || svs->commands == NULL)
	{
		command_fail(si, fault_nosuch_target, _("\2%s\2 is not a valid service."), parv[1]);
		return;
	}

	memosvs = service_find("memoserv");
	if (memosvs != NULL && !irccasecmp(parv[1], memosvs->nick))
	{
		command_fail(si, fault_noprivs, _("\2%s\2 cannot be used as an override service."), parv[1]);
		return;
	}

	cmd = command_find(svs->commands, parv[2]);
	if (cmd == NULL)
	{
		command_fail(si, fault_nosuch_target, _("\2%s\2 is not a valid command."), parv[2]);
		return;
	}

	struct override_sourceinfo *const o_si = smalloc(sizeof *o_si);
	o_si->si.smu = mu;
	o_si->si.service = svs;
	o_si->si.v = &override_vtable;
	o_si->parent_si = atheme_object_ref(si);

	atheme_object_init(atheme_object(o_si), NULL, (atheme_object_destructor_fn) override_sourceinfo_dispose);

	logcommand(si, CMDLOG_ADMIN, "OVERRIDE: (account: \2%s\2) (service: \2%s\2) (command: \2%s\2) [parameters: \2%s\2]", parv[0], parv[1], parv[2], parv[3] != NULL ? parv[3] : "");
	wallops("\2%s\2 is using OperServ OVERRIDE: account=%s service=%s command=%s params=%s", get_source_name(si), parv[0], parv[1], parv[2], parv[3] != NULL ? parv[3] : "");

	newparc = text_to_parv(parv[3] != NULL ? parv[3] : "", cmd->maxparc, newparv);
	for (i = newparc; i < ARRAY_SIZE(newparv); i++)
		newparv[i] = NULL;
	command_exec(svs, &o_si->si, cmd, newparc, newparv);

	atheme_object_unref(o_si);
}

static struct command os_override = {
	.name           = "OVERRIDE",
	.desc           = N_("Perform a transaction on another user's account"),
	.access         = PRIV_OVERRIDE,
	.maxparc        = 4,
	.cmd            = &os_cmd_override,
	.help           = { .path = "oservice/override" },
};

static void
mod_init(struct module ATHEME_VATTR_UNUSED *const restrict m)
{
        service_named_bind_command("operserv", &os_override);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("operserv", &os_override);
}

SIMPLE_DECLARE_MODULE_V1("operserv/override", MODULE_UNLOAD_CAPABILITY_OK)
