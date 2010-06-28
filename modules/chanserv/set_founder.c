/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Copyright (c) 2006-2010 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains routines to handle the CService SET FOUNDER command.
 *
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/set_founder", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_set_founder(sourceinfo_t *si, int parc, char *parv[]);

command_t cs_set_founder = { "FOUNDER", N_("Transfers foundership of a channel."), AC_NONE, 2, cs_cmd_set_founder };

list_t *cs_set_cmdtree;
list_t *cs_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(cs_set_cmdtree, "chanserv/set_core", "cs_set_cmdtree");
	MODULE_USE_SYMBOL(cs_helptree, "chanserv/main", "cs_helptree");

	command_add(&cs_set_founder, cs_set_cmdtree);

	help_addentry(cs_helptree, "SET FOUNDER", "help/cservice/set_founder", NULL);
}

void _moddeinit()
{
	command_delete(&cs_set_founder, cs_set_cmdtree);

	help_delentry(cs_helptree, "SET FOUNDER");
}

/*
 * This is how CS SET FOUNDER behaves in the absence of channel passwords:
 *
 * To transfer a channel, the original founder (OF) issues the command:
 *    /CS SET #chan FOUNDER NF
 * where NF is the new founder of the channel.
 *
 * Then, to complete the transfer, the NF must issue the command:
 *    /CS SET #chan FOUNDER NF
 *
 * To cancel the transfer before it completes, the OF can issue the command:
 *    /CS SET #chan FOUNDER OF
 *
 * The purpose of the confirmation step is to prevent users from giving away
 * undesirable channels (e.g. registering #kidsex and transferring to an
 * innocent user.) Originally, we used channel passwords for this purpose.
 */
static void cs_cmd_set_founder(sourceinfo_t *si, int parc, char *parv[])
{
	char *newfounder = parv[1];
	myuser_t *tmu;
	mychan_t *mc;

	if (!si->smu)
	{
		command_fail(si, fault_noprivs, _("You are not logged in."));
		return;
	}

	if (!newfounder)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SET FOUNDER");
		return;
	}

	if (!(tmu = myuser_find_ext(newfounder)))
	{
		command_fail(si, fault_nosuch_target, _("\2%s\2 is not registered."), newfounder);
		return;
	}

	if (!(mc = mychan_find(parv[0])))
	{
		command_fail(si, fault_nosuch_target, _("Channel \2%s\2 is not registered."), parv[0]);
		return;
	}

	if (!is_founder(mc, si->smu))
	{
		/* User is not currently the founder.
		 * Maybe he is trying to complete a transfer?
		 */
		metadata_t *md;

		/* XXX is it portable to compare times like that? */
		if ((si->smu == tmu)
			&& (md = metadata_find(mc, "private:verify:founderchg:newfounder"))
			&& !irccasecmp(md->value, si->smu->name)
			&& (md = metadata_find(mc, "private:verify:founderchg:timestamp"))
			&& (atol(md->value) >= si->smu->registered))
		{
			node_t *n;
			chanacs_t *ca;
	
			/* Duplicates the check below. We check below for user
			 * convenience, but we need to check here as well to
			 * avoid a verify/use bug that can cause us to make the
			 * access list too big. */
			if (!chanacs_find(mc, tmu, 0))
			{
				ca = chanacs_open(mc, tmu, NULL, true);
				if (ca->level == 0 && chanacs_is_table_full(ca))
				{
					command_fail(si, fault_toomany, _("Channel %s access list is full."), mc->name);
					chanacs_close(ca);
					return;
				}
				chanacs_close(ca);
			}

			if ((myuser_num_channels(tmu) >= me.maxchans) && !has_priv_myuser(tmu, PRIV_REG_NOLIMIT))
			{
				command_fail(si, fault_toomany, _("\2%s\2 has too many channels registered."), tmu->name);
				return;
			}

			if (metadata_find(mc, "private:close:closer"))
			{
				command_fail(si, fault_noprivs, _("\2%s\2 is closed; it cannot be transferred."), mc->name);
				return;
			}

			logcommand(si, CMDLOG_REGISTER, "SET:FOUNDER: \2%s\2 to \2%s\2 (completing transfer from \2%s\2)", mc->name, tmu->name, mychan_founder_names(mc));
			verbose(mc, "Foundership transferred from \2%s\2 to \2%s\2.", mychan_founder_names(mc), tmu->name);

			/* add target as founder... */
			LIST_FOREACH(n, mc->chanacs.head)
			{
				ca = n->data;
				/* CA_FLAGS is always on if CA_FOUNDER is on, this just
				 * ensures we don't crash if not -- jilles
				 */
				if (ca->myuser != NULL && ca->level & CA_FOUNDER)
					chanacs_modify_simple(ca, CA_FLAGS, CA_FOUNDER);
			}
			chanacs_change_simple(mc, tmu, NULL, CA_FOUNDER_0, 0);

			/* delete transfer metadata */
			metadata_delete(mc, "private:verify:founderchg:newfounder");
			metadata_delete(mc, "private:verify:founderchg:timestamp");

			/* done! */
			command_success_nodata(si, _("Transfer complete: \2%s\2 has been set as founder for \2%s\2."), tmu->name, mc->name);

			return;
		}

		command_fail(si, fault_noprivs, _("You are not the founder of \2%s\2."), mc->name);
		return;
	}

	if (is_founder(mc, tmu))
	{
		/* User is currently the founder and
		 * trying to transfer back to himself.
		 * Maybe he is trying to cancel a transfer?
		 */

		if (metadata_find(mc, "private:verify:founderchg:newfounder"))
		{
			metadata_delete(mc, "private:verify:founderchg:newfounder");
			metadata_delete(mc, "private:verify:founderchg:timestamp");

			logcommand(si, CMDLOG_REGISTER, "SET:FOUNDER: \2%s\2 to \2%s\2 (cancelling transfer)", mc->name, tmu->name);
			command_success_nodata(si, _("The transfer of \2%s\2 has been cancelled."), mc->name);

			return;
		}

		command_fail(si, fault_nochange, _("\2%s\2 is already the founder of \2%s\2."), tmu->name, mc->name);
		return;
	}

	/* If the target user does not have access yet, this may overflow
	 * the access list. Check at this time because that is more convenient
	 * for users.
	 * -- jilles
	 */
	if (!chanacs_find(mc, tmu, 0))
	{
		chanacs_t *ca;

		ca = chanacs_open(mc, tmu, NULL, true);
		if (ca->level == 0 && chanacs_is_table_full(ca))
		{
			command_fail(si, fault_toomany, _("Channel %s access list is full."), mc->name);
			chanacs_close(ca);
			return;
		}
		chanacs_close(ca);
	}

	/* check for lazy cancellation of outstanding requests */
	if (metadata_find(mc, "private:verify:founderchg:newfounder"))
	{
		logcommand(si, CMDLOG_REGISTER, "SET:FOUNDER: \2%s\2 to \2%s\2 (cancelling old transfer and initializing transfer)", mc->name, tmu->name);
		command_success_nodata(si, _("The previous transfer request for \2%s\2 has been cancelled."), mc->name);
	}
	else
		logcommand(si, CMDLOG_REGISTER, "SET:FOUNDER: \2%s\2 to \2%s\2 (initializing transfer)", mc->name, tmu->name);

	metadata_add(mc, "private:verify:founderchg:newfounder", tmu->name);
	metadata_add(mc, "private:verify:founderchg:timestamp", itoa(time(NULL)));

	command_success_nodata(si, _("\2%s\2 can now take ownership of \2%s\2."), tmu->name, mc->name);
	command_success_nodata(si, _("In order to complete the transfer, \2%s\2 must perform the following command:"), tmu->name);
	command_success_nodata(si, "   \2/msg %s SET %s FOUNDER %s\2", chansvs.nick, mc->name, tmu->name);
	command_success_nodata(si, _("After that command is issued, the channel will be transferred."));
	command_success_nodata(si, _("To cancel the transfer, use \2/msg %s SET %s FOUNDER %s\2"), chansvs.nick, mc->name, si->smu->name);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
