/*
 * Copyright (c) 2009 William Pitcock <nenolod@atheme.org>.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains functionality which implements the OService INJECT command.
 *
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/override", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void os_cmd_override(sourceinfo_t *si, int parc, char *parv[]);

command_t os_override = { "OVERRIDE", N_("Perform a transaction on another user's account"), PRIV_OVERRIDE, 4, os_cmd_override, { .path = "oservice/override" } };

void _modinit(module_t *m)
{
        service_named_bind_command("operserv", &os_override);
}

void _moddeinit(void)
{
	service_named_unbind_command("operserv", &os_override);
}

typedef struct {
	sourceinfo_t si;
	sourceinfo_t *parent_si;
} cooked_sourceinfo_t;

static void override_command_fail(sourceinfo_t *si, faultcode_t code, const char *message)
{
	cooked_sourceinfo_t *csi = (cooked_sourceinfo_t *) si;

	return_if_fail(csi != NULL);

	command_fail(csi->parent_si, code, "%s", message);
}

static void override_command_success_nodata(sourceinfo_t *si, const char *message)
{
	cooked_sourceinfo_t *csi = (cooked_sourceinfo_t *) si;

	return_if_fail(csi != NULL);

	command_success_nodata(csi->parent_si, "%s", message);
}

static void override_command_success_string(sourceinfo_t *si, const char *result, const char *message)
{
	cooked_sourceinfo_t *csi = (cooked_sourceinfo_t *) si;

	return_if_fail(csi != NULL);

	command_success_string(csi->parent_si, result, "%s", message);
}

static const char *override_get_source_name(sourceinfo_t *si)
{
	cooked_sourceinfo_t *csi = (cooked_sourceinfo_t *) si;

	return_val_if_fail(csi != NULL, NULL);

	return get_source_name(csi->parent_si);
}

static const char *override_get_source_mask(sourceinfo_t *si)
{
	cooked_sourceinfo_t *csi = (cooked_sourceinfo_t *) si;

	return_val_if_fail(csi != NULL, NULL);

	return get_source_mask(csi->parent_si);
}

static const char *override_get_oper_name(sourceinfo_t *si)
{
	cooked_sourceinfo_t *csi = (cooked_sourceinfo_t *) si;

	return_val_if_fail(csi != NULL, NULL);

	return get_oper_name(csi->parent_si);
}

static const char *override_get_storage_oper_name(sourceinfo_t *si)
{
	cooked_sourceinfo_t *csi = (cooked_sourceinfo_t *) si;

	return_val_if_fail(csi != NULL, NULL);

	return get_storage_oper_name(csi->parent_si);
}

struct sourceinfo_vtable override_vtable = {
	"override",
	override_command_fail,
	override_command_success_nodata,
	override_command_success_string,
	override_get_source_name,
	override_get_source_mask,
	override_get_oper_name,
	override_get_storage_oper_name,
};

static int text_to_parv(char *text, int maxparc, char **parv)
{
	int count = 0;
	char *p;

	if (maxparc == 0)
		return 0;

	if (!text)
		return 0;

	p = text;
	while (count < maxparc - 1 && (parv[count] = strtok(p, " ")) != NULL)
		count++, p = NULL;

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

static void os_cmd_override(sourceinfo_t *si, int parc, char *parv[])
{
	cooked_sourceinfo_t o_si;
	myuser_t *mu = NULL;
	service_t *svs;
	service_t *memosvs;
	command_t *cmd;
	int newparc, i;
	char *newparv[20];

	if (!parv[0] || !parv[1] || !parv[2])
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "OVERRIDE");
		command_fail(si, fault_needmoreparams, _("Syntax: OVERRIDE <account> <service> <command> [params]"));
		return;
	}

	if (*parv[0] == '#')
	{
		mychan_t *mc;
		mowgli_node_t *n;

		if (!(mc = mychan_find(parv[0])))
		{
			command_fail(si, fault_nosuch_target, _("\2%s\2 is not a registered channel."), parv[0]);
			return;
		}

		MOWGLI_ITER_FOREACH(n, mc->chanacs.head)
		{
			chanacs_t *ca = (chanacs_t *) n->data;

			if (ca->entity != NULL && isuser(ca->entity) && ca->level & CA_FOUNDER)
			{
				mu = user(ca->entity);
				break;
			}
		}

		/* this should never happen, but we'll check anyway. */
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
	if (!irccasecmp(parv[1], memosvs->nick))
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

	memset(&o_si, '\0', sizeof(cooked_sourceinfo_t));
	o_si.si.su = NULL;
	o_si.si.smu = mu;
	o_si.si.service = svs;
	o_si.si.v = &override_vtable;
	o_si.si.connection = NULL;
	o_si.parent_si = si;

	logcommand(si, CMDLOG_ADMIN, "OVERRIDE: (account: \2%s\2) (service: \2%s\2) (command: \2%s\2) [parameters: \2%s\2]", parv[0], parv[1], parv[2], parv[3] != NULL ? parv[3] : "");
	wallops("\2%s\2 is using OperServ OVERRIDE: account=%s service=%s command=%s params=%s", get_source_name(si), parv[0], parv[1], parv[2], parv[3] != NULL ? parv[3] : "");

	newparc = text_to_parv(parv[3] != NULL ? parv[3] : "", cmd->maxparc, newparv);
	for (i = newparc; i < (int)(sizeof(newparv) / sizeof(newparv[0])); i++)
		newparv[i] = NULL;
	command_exec(svs, (sourceinfo_t *) &o_si, cmd, newparc, newparv);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
