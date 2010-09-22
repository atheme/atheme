/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Copyright (c) 2006-2010 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains routines to handle the CService SET MLOCK command.
 *
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/set_mlock", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_set_mlock(sourceinfo_t *si, int parc, char *parv[]);

command_t cs_set_mlock = { "MLOCK", N_("Sets channel mode lock."), AC_NONE, 2, cs_cmd_set_mlock, { .path = "cservice/set_mlock" } };

mowgli_patricia_t **cs_set_cmdtree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(cs_set_cmdtree, "chanserv/set_core", "cs_set_cmdtree");

	command_add(&cs_set_mlock, *cs_set_cmdtree);
}

void _moddeinit()
{
	command_delete(&cs_set_mlock, *cs_set_cmdtree);
}

static void cs_cmd_set_mlock(sourceinfo_t *si, int parc, char *parv[])
{
	mychan_t *mc;
	char modebuf[32], *end, c;
	int dir = MTYPE_NUL;
	int newlock_on = 0, newlock_off = 0, newlock_limit = 0, flag = 0;
	unsigned int mask, changed;
	bool mask_ext;
	char newlock_key[KEYLEN];
	char newlock_ext[ignore_mode_list_size][512];
	bool newlock_ext_off[ignore_mode_list_size];
	char newext[512];
	char ext_plus[ignore_mode_list_size + 1];
	char ext_minus[ignore_mode_list_size + 1];
	size_t i;
	char *letters = strtok(parv[1], " ");
	char *arg;
	metadata_t *md;

	if (!(mc = mychan_find(parv[0])))
	{
		command_fail(si, fault_nosuch_target, _("Channel \2%s\2 is not registered."), parv[0]);
		return;
	}

	if (!chanacs_source_has_flag(mc, si, CA_SET))
	{
		if (ircd->oper_only_modes == 0 ||
				!has_priv(si, PRIV_CHAN_CMODES) ||
				!has_priv(si, PRIV_CHAN_ADMIN))
		{
			command_fail(si, fault_noprivs, _("You are not authorized to perform this command."));
			return;
		}
		mask = ~ircd->oper_only_modes;
		mask_ext = true;
	}
	else
	{
		mask = has_priv(si, PRIV_CHAN_CMODES) ? 0 : ircd->oper_only_modes;
		mask_ext = false;

	}

	for (i = 0; i < ignore_mode_list_size; i++)
	{
		newlock_ext[i][0] = '\0';
		newlock_ext_off[i] = false;
	}
	newlock_key[0] = '\0';

	while (letters && *letters)
	{
		if (*letters != '+' && *letters != '-' && dir == MTYPE_NUL)
		{
			letters++;
			continue;
		}

		switch ((c = *letters++))
		{
		  case '+':
			  dir = MTYPE_ADD;
			  break;

		  case '-':
			  dir = MTYPE_DEL;
			  break;

		  case 'k':
			  if (dir == MTYPE_ADD)
			  {
				  arg = strtok(NULL, " ");
				  if (!arg)
				  {
					  command_fail(si, fault_badparams, _("You need to specify a value for mode +%c."), 'k');
					  return;
				  }
				  else if (strlen(arg) >= KEYLEN)
				  {
					  command_fail(si, fault_badparams, _("MLOCK key is too long (%d > %d)."), (int)strlen(arg), KEYLEN - 1);
					  return;
				  }
				  else if (strchr(arg, ',') || arg[0] == ':')
				  {
					  command_fail(si, fault_badparams, _("MLOCK key contains invalid characters."));
					  return;
				  }

				  strlcpy(newlock_key, arg, sizeof newlock_key);
				  newlock_off &= ~CMODE_KEY;
			  }
			  else
			  {
				  newlock_key[0] = '\0';
				  newlock_off |= CMODE_KEY;
			  }

			  break;

		  case 'l':
			  if (dir == MTYPE_ADD)
			  {
				  arg = strtok(NULL, " ");
				  if(!arg)
				  {
					  command_fail(si, fault_badparams, _("You need to specify a value for mode +%c."), 'l');
					  return;
				  }

				  if (atol(arg) <= 0)
				  {
					  command_fail(si, fault_badparams, _("You must specify a positive integer for limit."));
					  return;
				  }

				  newlock_limit = atol(arg);
				  newlock_off &= ~CMODE_LIMIT;
			  }
			  else
			  {
				  newlock_limit = 0;
				  newlock_off |= CMODE_LIMIT;
			  }

			  break;

		  default:
			  flag = mode_to_flag(c);

			  if (flag)
			  {
				  if (dir == MTYPE_ADD)
					  newlock_on |= flag, newlock_off &= ~flag;
				  else
					  newlock_off |= flag, newlock_on &= ~flag;
				  break;
			  }

			  for (i = 0; ignore_mode_list[i].mode != '\0'; i++)
			  {
				  if (c == ignore_mode_list[i].mode)
				  {
					  if (dir == MTYPE_ADD)
					  {
						  arg = strtok(NULL, " ");
						  if(!arg)
						  {
							  command_fail(si, fault_badparams, _("You need to specify a value for mode +%c."), c);
							  return;
						  }
						  if (strlen(arg) > 350)
						  {
							  command_fail(si, fault_badparams, _("Invalid value \2%s\2 for mode +%c."), arg, c);
							  return;
						  }
						  if ((mc->chan == NULL || mc->chan->extmodes[i] == NULL || strcmp(mc->chan->extmodes[i], arg)) && !ignore_mode_list[i].check(arg, mc->chan, mc, si->su, si->smu))
						  {
							  command_fail(si, fault_badparams, _("Invalid value \2%s\2 for mode +%c."), arg, c);
							  return;
						  }
						  strlcpy(newlock_ext[i], arg, sizeof newlock_ext[i]);
						  newlock_ext_off[i] = false;
					  }
					  else
					  {
						  newlock_ext[i][0] = '\0';
						  newlock_ext_off[i] = true;
					  }
				  }
			  }
		}
	}

	/* note: the following does not treat +lk and extmodes correctly */
	changed = ((newlock_on ^ mc->mlock_on) | (newlock_off ^ mc->mlock_off));
	changed &= ~mask;
	/* if they're only allowed to alter oper only modes, require
	 * them to actually change such modes -- jilles */
	if (!changed && mask_ext)
	{
		command_fail(si, fault_noprivs, _("You may only alter \2+%s\2 modes."), flags_to_string(~mask));
		return;
	}

	/* save it to mychan */
	/* leave the modes in mask unchanged -- jilles */
	mc->mlock_on = (newlock_on & ~mask) | (mc->mlock_on & mask);
	mc->mlock_off = (newlock_off & ~mask) | (mc->mlock_off & mask);
	if (!(mask & CMODE_LIMIT))
		mc->mlock_limit = newlock_limit;
	if (!(mask & CMODE_KEY))
	{
		free(mc->mlock_key);
		mc->mlock_key = *newlock_key != '\0' ? sstrdup(newlock_key) : NULL;
	}

	ext_plus[0] = '\0';
	ext_minus[0] = '\0';
	if (mask_ext)
	{
		md = metadata_find(mc, "private:mlockext");
		if (md != NULL)
		{
			arg = md->value;
			while (*arg != '\0')
			{
				modebuf[0] = *arg;
				modebuf[1] = '\0';
				strlcat(arg[1] == ' ' || arg[1] == '\0' ? ext_minus : ext_plus, modebuf, ignore_mode_list_size + 1);
				arg++;
				while (*arg != ' ' && *arg != '\0')
					arg++;
				while (*arg == ' ')
					arg++;
			}
		}
	}
	else
	{
		newext[0] = '\0';
		for (i = 0; i < ignore_mode_list_size; i++)
		{
			if (newlock_ext[i][0] != '\0' || newlock_ext_off[i])
			{
				if (*newext != '\0')
				{
					modebuf[0] = ' ';
					modebuf[1] = '\0';
					strlcat(newext, modebuf, sizeof newext);
				}
				modebuf[0] = ignore_mode_list[i].mode;
				modebuf[1] = '\0';
				strlcat(newext, modebuf, sizeof newext);
				strlcat(newlock_ext_off[i] ? ext_minus : ext_plus,
						modebuf, ignore_mode_list_size + 1);
				if (!newlock_ext_off[i])
					strlcat(newext, newlock_ext[i], sizeof newext);
			}
		}
		if (newext[0] != '\0')
			metadata_add(mc, "private:mlockext", newext);
		else
			metadata_delete(mc, "private:mlockext");
	}

	end = modebuf;
	*end = 0;

	if (mc->mlock_on || mc->mlock_key || mc->mlock_limit || *ext_plus)
		end += snprintf(end, sizeof(modebuf) - (end - modebuf), "+%s%s%s%s", flags_to_string(mc->mlock_on), mc->mlock_key ? "k" : "", mc->mlock_limit ? "l" : "", ext_plus);

	if (mc->mlock_off || *ext_minus)
		end += snprintf(end, sizeof(modebuf) - (end - modebuf), "-%s%s%s%s", flags_to_string(mc->mlock_off), mc->mlock_off & CMODE_KEY ? "k" : "", mc->mlock_off & CMODE_LIMIT ? "l" : "", ext_minus);

	if (*modebuf)
	{
		command_success_nodata(si, _("The MLOCK for \2%s\2 has been set to \2%s\2."), mc->name, modebuf);
		logcommand(si, CMDLOG_SET, "SET:MLOCK: \2%s\2 to \2%s\2", mc->name, modebuf);
	}
	else
	{
		command_success_nodata(si, _("The MLOCK for \2%s\2 has been removed."), mc->name);
		logcommand(si, CMDLOG_SET, "SET:MLOCK:NONE: \2%s\2", mc->name);
	}
	if (changed & ircd->oper_only_modes)
		logcommand(si, CMDLOG_SET, _("SET:MLOCK: \2%s\2 to \2%s\2 by \2%s\2"), mc->name, *modebuf != '\0' ? modebuf : "+", get_oper_name(si));

	check_modes(mc, true);
	if (mc->chan != NULL)
		mlock_sts(mc->chan);

	return;
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
