/* main.c - rpgserv main() routine.
 * based on elly's rpgserv for atheme-6.x --nenolod
 */

#include "atheme.h"

static void
rs_cmd_enable(struct sourceinfo *si, int parc, char *parv[])
{
	char *chan = parv[0];
	struct mychan *mc;

	if (!chan)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "ENABLE");
		command_fail(si, fault_needmoreparams, _("Syntax: ENABLE <channel>"));
		return;
	}

	mc = mychan_find(chan);
	if (!mc)
	{
		command_fail(si, fault_nosuch_target, _("Channel \2%s\2 is not registered."), chan);
		return;
	}

	if (metadata_find(mc, "private:rpgserv:enabled"))
	{
		command_fail(si, fault_nochange, _("\2%s\2 already has RPGServ enabled."), chan);
		return;
	}

	metadata_add(mc, "private:rpgserv:enabled", si->su->nick);
	logcommand(si, CMDLOG_ADMIN, "RPGSERV:ENABLE: \2%s\2", chan);
	command_success_nodata(si, _("RPGServ enabled for \2%s\2."), chan);
}

static void
rs_cmd_disable(struct sourceinfo *si, int parc, char *parv[])
{
	char *chan = parv[0];
	struct mychan *mc;

	if (!chan)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "DISABLE");
		command_fail(si, fault_needmoreparams, _("Syntax: DISABLE <channel>"));
		return;
	}

	mc = mychan_find(chan);
	if (!mc)
	{
		command_fail(si, fault_nosuch_target, _("Channel \2%s\2 is not registered."), chan);
		return;
	}

	if (!metadata_find(mc, "private:rpgserv:enabled"))
	{
		command_fail(si, fault_nochange, _("\2%s\2 already has RPGServ disabled."), chan);
		return;
	}

	metadata_delete(mc, "private:rpgserv:enabled");
	logcommand(si, CMDLOG_ADMIN, "RPGSERV:DISABLE: \2%s\2", chan);
	command_success_nodata(si, _("RPGServ disabled for \2%s\2."), chan);
}

static struct command rs_enable = { "ENABLE", N_("Enable RPGServ for a channel."), AC_NONE, 1, rs_cmd_enable, { .path = "rpgserv/enable" } };
static struct command rs_disable = { "DISABLE", N_("Disable RPGServ for a channel."), AC_NONE, 1, rs_cmd_disable, { .path = "rpgserv/disable" } };

static void
mod_init(struct module ATHEME_VATTR_UNUSED *const restrict m)
{
	service_named_bind_command("rpgserv", &rs_enable);
	service_named_bind_command("rpgserv", &rs_disable);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("rpgserv", &rs_enable);
	service_named_unbind_command("rpgserv", &rs_disable);
}

SIMPLE_DECLARE_MODULE_V1("rpgserv/enable", MODULE_UNLOAD_CAPABILITY_OK)
