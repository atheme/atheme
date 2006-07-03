/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains routines to handle the CService SET command.
 *
 * $Id: set.c 5686 2006-07-03 16:25:03Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/set", FALSE, _modinit, _moddeinit,
	"$Id: set.c 5686 2006-07-03 16:25:03Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_set(char *origin);

command_t cs_set = { "SET", "Sets various control flags.",
                        AC_NONE, cs_cmd_set };

list_t *cs_cmdtree;
list_t *cs_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(cs_cmdtree, "chanserv/main", "cs_cmdtree");
	MODULE_USE_SYMBOL(cs_helptree, "chanserv/main", "cs_helptree");

        command_add(&cs_set, cs_cmdtree);

	help_addentry(cs_helptree, "SET FOUNDER", "help/cservice/set_founder", NULL);
	help_addentry(cs_helptree, "SET MLOCK", "help/cservice/set_mlock", NULL);
	help_addentry(cs_helptree, "SET SECURE", "help/cservice/set_secure", NULL);
	help_addentry(cs_helptree, "SET VERBOSE", "help/cservice/set_verbose", NULL);
	help_addentry(cs_helptree, "SET URL", "help/cservice/set_url", NULL);
	help_addentry(cs_helptree, "SET EMAIL", "help/cservice/set_email", NULL);
	help_addentry(cs_helptree, "SET ENTRYMSG", "help/cservice/set_entrymsg", NULL);
	help_addentry(cs_helptree, "SET PROPERTY", "help/cservice/set_property", NULL);
	help_addentry(cs_helptree, "SET STAFFONLY", "help/cservice/set_staffonly", NULL);
	help_addentry(cs_helptree, "SET KEEPTOPIC", "help/cservice/set_keeptopic", NULL);
	help_addentry(cs_helptree, "SET FANTASY", "help/cservice/set_fantasy", NULL);
}

void _moddeinit()
{
	command_delete(&cs_set, cs_cmdtree);

	help_delentry(cs_helptree, "SET FOUNDER");
	help_delentry(cs_helptree, "SET MLOCK");
	help_delentry(cs_helptree, "SET SECURE");
	help_delentry(cs_helptree, "SET VERBOSE");
	help_delentry(cs_helptree, "SET URL");
	help_delentry(cs_helptree, "SET EMAIL");
	help_delentry(cs_helptree, "SET ENTRYMSG");
	help_delentry(cs_helptree, "SET PROPERTY");
	help_delentry(cs_helptree, "SET STAFFONLY");
	help_delentry(cs_helptree, "SET KEEPTOPIC");
	help_delentry(cs_helptree, "SET FANTASY");
}

struct set_command_ *set_cmd_find(char *origin, char *command);

/* SET <#channel> <setting> <parameters> */
static void cs_cmd_set(char *origin)
{
	char *name = strtok(NULL, " ");
	char *setting = strtok(NULL, " ");
	char *params = strtok(NULL, "");
	struct set_command_ *c;

	if (!name || !setting || !params)
	{
		notice(chansvs.nick, origin, STR_INSUFFICIENT_PARAMS, "SET");
		notice(chansvs.nick, origin, "Syntax: SET <#channel> <setting> <parameters>");
		return;
	}

	/* take the command through the hash table */
	if ((c = set_cmd_find(origin, setting)))
	{
		if (c->func)
			c->func(origin, name, params);
		else
			notice(chansvs.nick, origin, "Invalid setting.  Please use \2HELP\2 for help.");
	}
}

static void cs_set_email(char *origin, char *name, char *params)
{
        user_t *u = user_find_named(origin);
        mychan_t *mc;
        char *mail = strtok(params, " ");

        if (*name != '#')
        {
                notice(chansvs.nick, origin, STR_INVALID_PARAMS, "EMAIL");
                return;
        }

        if (!(mc = mychan_find(name)))
        {
                notice(chansvs.nick, origin, "\2%s\2 is not registered.", name);
                return;
        }

        if (!chanacs_user_has_flag(mc, u, CA_SET))
        {
                notice(chansvs.nick, origin, "You are not authorized to execute this command.");
                return;
        }

        if (!mail || !strcasecmp(mail, "NONE") || !strcasecmp(mail, "OFF"))
        {
                if (metadata_find(mc, METADATA_CHANNEL, "email"))
                {
                        metadata_delete(mc, METADATA_CHANNEL, "email");
                        notice(chansvs.nick, origin, "The e-mail address for \2%s\2 was deleted.", mc->name);
			logcommand(chansvs.me, u, CMDLOG_SET, "%s SET EMAIL NONE", mc->name);
                        return;
                }

                notice(chansvs.nick, origin, "The e-mail address for \2%s\2 was not set.", mc->name);
                return;
        }

        if (strlen(mail) >= EMAILLEN)
        {
                notice(chansvs.nick, origin, STR_INVALID_PARAMS, "EMAIL");
                return;
        }

        if (!validemail(mail))
        {
                notice(chansvs.nick, origin, "\2%s\2 is not a valid e-mail address.", mail);
                return;
        }

        /* we'll overwrite any existing metadata */
        metadata_add(mc, METADATA_CHANNEL, "email", mail);

	logcommand(chansvs.me, u, CMDLOG_SET, "%s SET EMAIL %s", mc->name, mail);
        notice(chansvs.nick, origin, "The e-mail address for \2%s\2 has been set to \2%s\2.", name, mail);
}

static void cs_set_url(char *origin, char *name, char *params)
{
	user_t *u = user_find_named(origin);
	mychan_t *mc;
	char *url = strtok(params, " ");

	if (*name != '#')
	{
		notice(chansvs.nick, origin, STR_INVALID_PARAMS, "URL");
		return;
	}

	if (!(mc = mychan_find(name)))
	{
		notice(chansvs.nick, origin, "\2%s\2 is not registered.", name);
		return;
	}

	if (!chanacs_user_has_flag(mc, u, CA_SET))
	{
		notice(chansvs.nick, origin, "You are not authorized to execute this command.");
		return;
	}

	/* XXX: I'd like to be able to use /CS SET #channel URL to clear but CS SET won't let me... */
	if (!url || !strcasecmp("OFF", url) || !strcasecmp("NONE", url))
	{
		/* not in a namespace to allow more natural use of SET PROPERTY.
		 * they may be able to introduce spaces, though. c'est la vie.
		 */
		if (metadata_find(mc, METADATA_CHANNEL, "url"))
		{
			metadata_delete(mc, METADATA_CHANNEL, "url");
			logcommand(chansvs.me, u, CMDLOG_SET, "%s SET URL NONE", mc->name);
			notice(chansvs.nick, origin, "The URL for \2%s\2 has been cleared.", name);
			return;
		}

		notice(chansvs.nick, origin, "The URL for \2%s\2 was not set.", name);
		return;
	}

	/* we'll overwrite any existing metadata */
	metadata_add(mc, METADATA_CHANNEL, "url", url);

	logcommand(chansvs.me, u, CMDLOG_SET, "%s SET URL %s", mc->name, url);
	notice(chansvs.nick, origin, "The URL of \2%s\2 has been set to \2%s\2.", name, url);
}

static void cs_set_entrymsg(char *origin, char *name, char *params)
{
	user_t *u = user_find_named(origin);
	mychan_t *mc;

	if (!(mc = mychan_find(name)))
	{
		notice(chansvs.nick, origin, "\2%s\2 is not registered.", name);
		return;
	}

	if (!chanacs_user_has_flag(mc, u, CA_SET))
	{
		notice(chansvs.nick, origin, "You are not authorized to execute this command.");
		return;
	}

	/* XXX: I'd like to be able to use /CS SET #channel ENTRYMSG to clear but CS SET won't let me... */
	if (!params || !strcasecmp("OFF", params) || !strcasecmp("NONE", params))
	{
		/* entrymsg is private because users won't see it if they're AKICKED,
		 * if the channel is +i, or if the channel is STAFFONLY
		 */
		if (metadata_find(mc, METADATA_CHANNEL, "private:entrymsg"))
		{
			metadata_delete(mc, METADATA_CHANNEL, "private:entrymsg");
			logcommand(chansvs.me, u, CMDLOG_SET, "%s SET ENTRYMSG NONE", mc->name, params);
			notice(chansvs.nick, origin, "The entry message for \2%s\2 has been cleared.", name);
			return;
		}

		notice(chansvs.nick, origin, "The entry message for \2%s\2 was not set.", name);
		return;
	}

	/* we'll overwrite any existing metadata */
	metadata_add(mc, METADATA_CHANNEL, "private:entrymsg", params);

	logcommand(chansvs.me, u, CMDLOG_SET, "%s SET ENTRYMSG %s", mc->name, params);
	notice(chansvs.nick, origin, "The entry message for \2%s\2 has been set to \2%s\2", name, params);
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
static void cs_set_founder(char *origin, char *name, char *params)
{
	user_t *u = user_find_named(origin);
	char *newfounder = strtok(params, " ");
	myuser_t *tmu;
	mychan_t *mc;

	if (!u->myuser)
	{
		notice(chansvs.nick, origin, "You are not logged in.");
		return;
	}

	if (!name || !newfounder)
	{
		notice(chansvs.nick, origin, STR_INSUFFICIENT_PARAMS, "FOUNDER");
		notice(chansvs.nick, origin, "Usage: SET <#channel> FOUNDER <new founder>");
	}

	if (*name != '#')
	{
		notice(chansvs.nick, origin, STR_INVALID_PARAMS, "FOUNDER");
		return;
	}

	if (!(tmu = myuser_find_ext(newfounder)))
	{
		notice(chansvs.nick, origin, "\2%s\2 is not registered.", newfounder);
		return;
	}

	if (!(mc = mychan_find(name)))
	{
		notice(chansvs.nick, origin, "\2%s\2 is not registered.", name);
		return;
	}

	if (!is_founder(mc, u->myuser))
	{
		/* User is not currently the founder.
		 * Maybe he is trying to complete a transfer?
		 */
		metadata_t *md;

		/* XXX is it portable to compare times like that? */
		if ((u->myuser == tmu)
			&& (md = metadata_find(mc, METADATA_CHANNEL, "private:verify:founderchg:newfounder"))
			&& !irccasecmp(md->value, u->myuser->name)
			&& (md = metadata_find(mc, METADATA_CHANNEL, "private:verify:founderchg:timestamp"))
			&& (atol(md->value) >= u->myuser->registered))
		{
			mychan_t *tmc;
			node_t *n;
			uint32_t i, tcnt;

			/* make sure they're within limits (from cs_cmd_register) */
			for (i = 0, tcnt = 0; i < HASHSIZE; i++)
			{
				LIST_FOREACH(n, mclist[i].head)
				{
					tmc = (mychan_t *)n->data;

					if (is_founder(tmc, tmu))
						tcnt++;
				}
			}

			if ((tcnt >= me.maxchans) && !has_priv_myuser(tmu, PRIV_REG_NOLIMIT))
			{
				notice(chansvs.nick, origin, "\2%s\2 has too many channels registered.", tmu->name);
				return;
			}

			if (metadata_find(mc, METADATA_CHANNEL, "private:close:closer"))
			{
				notice(chansvs.nick, origin, "\2%s\2 is closed; it cannot be transferred.", mc->name);
				return;
			}

			logcommand(chansvs.me, u, CMDLOG_SET, "%s SET FOUNDER %s (completing transfer from %s)", mc->name, tmu->name, mc->founder->name);

			/* add target as founder... */
			mc->founder = tmu;
			chanacs_change_simple(mc, tmu, NULL, CA_FOUNDER_0, 0, CA_ALL);

			/* delete transfer metadata */
			metadata_delete(mc, METADATA_CHANNEL, "private:verify:founderchg:newfounder");
			metadata_delete(mc, METADATA_CHANNEL, "private:verify:founderchg:timestamp");

			/* done! */
			snoop("SET:FOUNDER: \2%s\2 -> \2%s\2", mc->name, tmu->name);
			notice(chansvs.nick, origin, "Transfer complete: "
				"\2%s\2 has been set as founder for \2%s\2.", tmu->name, mc->name);

			return;
		}

		notice(chansvs.nick, origin, "You are not the founder of \2%s\2.", mc->name);
		return;
	}

	if (is_founder(mc, tmu))
	{
		/* User is currently the founder and
		 * trying to transfer back to himself.
		 * Maybe he is trying to cancel a transfer?
		 */

		if (metadata_find(mc, METADATA_CHANNEL, "private:verify:founderchg:newfounder"))
		{
			metadata_delete(mc, METADATA_CHANNEL, "private:verify:founderchg:newfounder");
			metadata_delete(mc, METADATA_CHANNEL, "private:verify:founderchg:timestamp");

			logcommand(chansvs.me, u, CMDLOG_SET, "%s SET FOUNDER %s (cancelling transfer)", mc->name, tmu->name);
			notice(chansvs.nick, origin, "The transfer of \2%s\2 has been cancelled.", mc->name);

			return;
		}

		notice(chansvs.nick, origin, "\2%s\2 is already the founder of \2%s\2.", tmu->name, mc->name);
		return;
	}

	/* check for lazy cancellation of outstanding requests */
	if (metadata_find(mc, METADATA_CHANNEL, "private:verify:founderchg:newfounder"))
	{
		logcommand(chansvs.me, u, CMDLOG_SET, "%s SET FOUNDER %s (cancelling old transfer and initializing transfer)", mc->name, tmu->name);
		notice(chansvs.nick, origin, "The previous transfer request for \2%s\2 has been cancelled.", mc->name);
	}
	else
		logcommand(chansvs.me, u, CMDLOG_SET, "%s SET FOUNDER %s (initializing transfer)", mc->name, tmu->name);

	metadata_add(mc, METADATA_CHANNEL, "private:verify:founderchg:newfounder", tmu->name);
	metadata_add(mc, METADATA_CHANNEL, "private:verify:founderchg:timestamp", itoa(time(NULL)));

	notice(chansvs.nick, origin, "\2%s\2 can now take ownership of \2%s\2.", tmu->name, mc->name);
	notice(chansvs.nick, origin, "In order to complete the transfer, \2%s\2 must perform the following command:", tmu->name);
	notice(chansvs.nick, origin, "   \2/msg %s SET %s FOUNDER %s\2", chansvs.nick, mc->name, tmu->name);
	notice(chansvs.nick, origin, "After that command is issued, the channel will be transferred.", mc->name);
	notice(chansvs.nick, origin, "To cancel the transfer, use \2/msg %s SET %s FOUNDER %s\2", chansvs.nick, mc->name, mc->founder->name);
}

static void cs_set_mlock(char *origin, char *name, char *params)
{
	user_t *u = user_find_named(origin);
	mychan_t *mc;
	char modebuf[32], *end, c;
	int add = -1;
	int32_t newlock_on = 0, newlock_off = 0, newlock_limit = 0, flag = 0;
	int32_t mask;
	char newlock_key[KEYLEN];
	char newlock_ext[MAXEXTMODES][512];
	boolean_t newlock_ext_off[MAXEXTMODES];
	char newext[512];
	char ext_plus[MAXEXTMODES + 1], ext_minus[MAXEXTMODES + 1];
	int i;
	char *letters = strtok(params, " ");
	char *arg;

	if (*name != '#' || !letters)
	{
		notice(chansvs.nick, origin, STR_INVALID_PARAMS, "MLOCK");
		return;
	}

	if (!(mc = mychan_find(name)))
	{
		notice(chansvs.nick, origin, "\2%s\2 is not registered.", name);
		return;
	}

	if (!chanacs_user_has_flag(mc, u, CA_SET))
	{
		notice(chansvs.nick, origin, "You are not authorized to perform this command.");
		return;
	}

	for (i = 0; i < MAXEXTMODES; i++)
	{
		newlock_ext[i][0] = '\0';
		newlock_ext_off[i] = FALSE;
	}
	ext_plus[0] = '\0';
	ext_minus[0] = '\0';
	newlock_key[0] = '\0';

	mask = has_priv(u, PRIV_CHAN_CMODES) ? 0 : ircd->oper_only_modes;

	while (*letters)
	{
		if (*letters != '+' && *letters != '-' && add < 0)
		{
			letters++;
			continue;
		}

		switch ((c = *letters++))
		{
		  case '+':
			  add = 1;
			  break;

		  case '-':
			  add = 0;
			  break;

		  case 'k':
			  if (add)
			  {
				  arg = strtok(NULL, " ");
				  if (!arg)
				  {
					  notice(chansvs.nick, origin, "You need to specify which key to MLOCK.");
					  return;
				  }
				  else if (strlen(arg) >= KEYLEN)
				  {
					  notice(chansvs.nick, origin, "MLOCK key is too long (%d > %d).", strlen(arg), KEYLEN - 1);
					  return;
				  }
				  else if (strchr(arg, ',') || arg[0] == ':')
				  {
					  notice(chansvs.nick, origin, "MLOCK key contains invalid characters.");
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
			  if (add)
			  {
				  arg = strtok(NULL, " ");
				  if(!arg)
				  {
					  notice(chansvs.nick, origin, "You need to specify what limit to MLOCK.");
					  return;
				  }

				  if (atol(arg) <= 0)
				  {
					  notice(chansvs.nick, origin, "You must specify a positive integer for limit.");
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
				  if (add)
					  newlock_on |= flag, newlock_off &= ~flag;
				  else
					  newlock_off |= flag, newlock_on &= ~flag;
				  break;
			  }

			  for (i = 0; i < MAXEXTMODES; i++)
			  {
				  if (c == ignore_mode_list[i].mode)
				  {
					  if (add)
					  {
						  arg = strtok(NULL, " ");
						  if(!arg)
						  {
							  notice(chansvs.nick, origin, "You need to specify a value for mode +%c.", c);
							  return;
						  }
						  if (strlen(arg) > 350)
						  {
							  notice(chansvs.nick, origin, "Invalid value \2%s\2 for mode +%c.", arg, c);
							  return;
						  }
						  if ((mc->chan->extmodes[i] == NULL || strcmp(mc->chan->extmodes[i], arg)) && !ignore_mode_list[i].check(arg, mc->chan, mc, u, u->myuser))
						  {
							  notice(chansvs.nick, origin, "Invalid value \2%s\2 for mode +%c.", arg, c);
							  return;
						  }
						  strlcpy(newlock_ext[i], arg, sizeof newlock_ext[i]);
						  newlock_ext_off[i] = FALSE;
					  }
					  else
					  {
						  newlock_ext[i][0] = '\0';
						  newlock_ext_off[i] = TRUE;
					  }
				  }
			  }
		}
	}

	if (strlen(newext) > 450)
	{
		notice(chansvs.nick, origin, "Mode lock is too long.");
		return;
	}

	/* save it to mychan */
	/* leave the modes in mask unchanged -- jilles */
	mc->mlock_on = (newlock_on & ~mask) | (mc->mlock_on & mask);
	mc->mlock_off = (newlock_off & ~mask) | (mc->mlock_off & mask);
	mc->mlock_limit = newlock_limit;

	if (mc->mlock_key)
		free(mc->mlock_key);

	mc->mlock_key = *newlock_key != '\0' ? sstrdup(newlock_key) : NULL;

	newext[0] = '\0';
	for (i = 0; i < MAXEXTMODES; i++)
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
					modebuf, MAXEXTMODES + 1);
			if (!newlock_ext_off[i])
				strlcat(newext, newlock_ext[i], sizeof newext);
		}
	}
	if (newext[0] != '\0')
		metadata_add(mc, METADATA_CHANNEL, "private:mlockext", newext);
	else
		metadata_delete(mc, METADATA_CHANNEL, "private:mlockext");

	end = modebuf;
	*end = 0;

	if (mc->mlock_on || mc->mlock_key || mc->mlock_limit || *ext_plus)
		end += snprintf(end, sizeof(modebuf) - (end - modebuf), "+%s%s%s%s", flags_to_string(mc->mlock_on), mc->mlock_key ? "k" : "", mc->mlock_limit ? "l" : "", ext_plus);

	if (mc->mlock_off || *ext_minus)
		end += snprintf(end, sizeof(modebuf) - (end - modebuf), "-%s%s%s%s", flags_to_string(mc->mlock_off), mc->mlock_off & CMODE_KEY ? "k" : "", mc->mlock_off & CMODE_LIMIT ? "l" : "", ext_minus);

	if (*modebuf)
	{
		notice(chansvs.nick, origin, "The MLOCK for \2%s\2 has been set to \2%s\2.", mc->name, modebuf);
		logcommand(chansvs.me, u, CMDLOG_SET, "%s SET MLOCK %s", mc->name, modebuf);
	}
	else
	{
		notice(chansvs.nick, origin, "The MLOCK for \2%s\2 has been removed.", mc->name);
		logcommand(chansvs.me, u, CMDLOG_SET, "%s SET MLOCK NONE", mc->name);
	}

	check_modes(mc, TRUE);

	return;
}

static void cs_set_keeptopic(char *origin, char *name, char *params)
{
	user_t *u = user_find_named(origin);
	mychan_t *mc;

	if (*name != '#')
	{
		notice(chansvs.nick, origin, STR_INVALID_PARAMS, "KEEPTOPIC");
		return;
	}

	if (!(mc = mychan_find(name)))
	{
		notice(chansvs.nick, origin, "\2%s\2 is not registered.", name);
		return;
	}

	if (!chanacs_user_has_flag(mc, u, CA_SET))
	{
		notice(chansvs.nick, origin, "You are not authorized to perform this command.");
		return;
	}

	if (!strcasecmp("ON", params))
	{
		if (MC_KEEPTOPIC & mc->flags)
                {
                        notice(chansvs.nick, origin, "The \2KEEPTOPIC\2 flag is already set for \2%s\2.", mc->name);
                        return;
                }

		logcommand(chansvs.me, u, CMDLOG_SET, "%s SET KEEPTOPIC ON", mc->name);

                mc->flags |= MC_KEEPTOPIC;

                notice(chansvs.nick, origin, "The \2KEEPTOPIC\2 flag has been set for \2%s\2.", mc->name);

                return;
        }

        else if (!strcasecmp("OFF", params))
        {
                if (!(MC_KEEPTOPIC & mc->flags))
                {
                        notice(chansvs.nick, origin, "The \2KEEPTOPIC\2 flag is not set for \2%s\2.", mc->name);
                        return;
                }

		logcommand(chansvs.me, u, CMDLOG_SET, "%s SET KEEPTOPIC OFF", mc->name);

                mc->flags &= ~MC_KEEPTOPIC;

                notice(chansvs.nick, origin, "The \2KEEPTOPIC\2 flag has been removed for \2%s\2.", mc->name);

                return;
        }

        else
        {
                notice(chansvs.nick, origin, STR_INVALID_PARAMS, "KEEPTOPIC");
                return;
        }
}

static void cs_set_secure(char *origin, char *name, char *params)
{
	user_t *u = user_find_named(origin);
	mychan_t *mc;

	if (*name != '#')
	{
		notice(chansvs.nick, origin, STR_INVALID_PARAMS, "SECURE");
		return;
	}

	if (!(mc = mychan_find(name)))
	{
		notice(chansvs.nick, origin, "\2%s\2 is not registered.", name);
		return;
	}

	if (!chanacs_user_has_flag(mc, u, CA_SET))
	{
		notice(chansvs.nick, origin, "You are not authorized to perform this command.");
		return;
	}

	if (!strcasecmp("ON", params))
	{
		if (MC_SECURE & mc->flags)
		{
			notice(chansvs.nick, origin, "The \2SECURE\2 flag is already set for \2%s\2.", mc->name);
			return;
		}

		logcommand(chansvs.me, u, CMDLOG_SET, "%s SET SECURE ON", mc->name);

		mc->flags |= MC_SECURE;

		notice(chansvs.nick, origin, "The \2SECURE\2 flag has been set for \2%s\2.", mc->name);

		return;
	}

	else if (!strcasecmp("OFF", params))
	{
		if (!(MC_SECURE & mc->flags))
		{
			notice(chansvs.nick, origin, "The \2SECURE\2 flag is not set for \2%s\2.", mc->name);
			return;
		}

		logcommand(chansvs.me, u, CMDLOG_SET, "%s SET SECURE OFF", mc->name);

		mc->flags &= ~MC_SECURE;

		notice(chansvs.nick, origin, "The \2SECURE\2 flag has been removed for \2%s\2.", mc->name);

		return;
	}

	else
	{
		notice(chansvs.nick, origin, STR_INVALID_PARAMS, "SECURE");
		return;
	}
}

static void cs_set_verbose(char *origin, char *name, char *params)
{
	user_t *u = user_find_named(origin);
	mychan_t *mc;

	if (*name != '#')
	{
		notice(chansvs.nick, origin, STR_INVALID_PARAMS, "VERBOSE");
		return;
	}

	if (!(mc = mychan_find(name)))
	{
		notice(chansvs.nick, origin, "\2%s\2 is not registered.", name);
		return;
	}

	if (!chanacs_user_has_flag(mc, u, CA_SET))
	{
		notice(chansvs.nick, origin, "You are not authorized to perform this command.");
		return;
	}

	if (!strcasecmp("ON", params) || !strcasecmp("ALL", params))
	{
		if (MC_VERBOSE & mc->flags)
		{
			notice(chansvs.nick, origin, "The \2VERBOSE\2 flag is already set for \2%s\2.", mc->name);
			return;
		}

		logcommand(chansvs.me, u, CMDLOG_SET, "%s SET VERBOSE ON", mc->name);

 		mc->flags &= ~MC_VERBOSE_OPS;
 		mc->flags |= MC_VERBOSE;

		verbose(mc, "\2%s\2 enabled the VERBOSE flag", u->nick);
		notice(chansvs.nick, origin, "The \2VERBOSE\2 flag has been set for \2%s\2.", mc->name);

		return;
	}

	else if (!strcasecmp("OPS", params))
	{
		if (MC_VERBOSE_OPS & mc->flags)
		{
			notice(chansvs.nick, origin, "The \2VERBOSE_OPS\2 flag is already set for \2%s\2.", mc->name);
			return;
		}

		logcommand(chansvs.me, u, CMDLOG_SET, "%s SET VERBOSE OPS", mc->name);

		if (mc->flags & MC_VERBOSE)
		{
			verbose(mc, "\2%s\2 restricted VERBOSE to chanops", u->nick);
 			mc->flags &= ~MC_VERBOSE;
 			mc->flags |= MC_VERBOSE_OPS;
		}
		else
		{
 			mc->flags |= MC_VERBOSE_OPS;
			verbose(mc, "\2%s\2 enabled the VERBOSE_OPS flag", u->nick);
		}

		notice(chansvs.nick, origin, "The \2VERBOSE_OPS\2 flag has been set for \2%s\2.", mc->name);

		return;
	}
	else if (!strcasecmp("OFF", params))
	{
		if (!((MC_VERBOSE | MC_VERBOSE_OPS) & mc->flags))
		{
			notice(chansvs.nick, origin, "The \2VERBOSE\2 flag is not set for \2%s\2.", mc->name);
			return;
		}

		logcommand(chansvs.me, u, CMDLOG_SET, "%s SET VERBOSE OFF", mc->name);

		if (mc->flags & MC_VERBOSE)
			verbose(mc, "\2%s\2 disabled the VERBOSE flag", u->nick);
		else
			verbose(mc, "\2%s\2 disabled the VERBOSE_OPS flag", u->nick);
		mc->flags &= ~(MC_VERBOSE | MC_VERBOSE_OPS);

		notice(chansvs.nick, origin, "The \2VERBOSE\2 flag has been removed for \2%s\2.", mc->name);

		return;
	}

	else
	{
		notice(chansvs.nick, origin, STR_INVALID_PARAMS, "VERBOSE");
		return;
	}
}

static void cs_set_fantasy(char *origin, char *name, char *params)
{
	user_t *u = user_find_named(origin);
	mychan_t *mc;

	if (*name != '#')
	{
		notice(chansvs.nick, origin, STR_INVALID_PARAMS, "FANTASY");
		return;
	}

	if (!(mc = mychan_find(name)))
	{
		notice(chansvs.nick, origin, "\2%s\2 is not registered.", name);
		return;
	}

	if (!chanacs_user_has_flag(mc, u, CA_SET))
	{
		notice(chansvs.nick, origin, "You are not authorized to perform this command.");
		return;
	}

	if (!strcasecmp("ON", params))
	{
		metadata_t *md = metadata_find(mc, METADATA_CHANNEL, "disable_fantasy");

		if (!md)
		{
			notice(chansvs.nick, origin, "Fantasy is already enabled on \2%s\2.", mc->name);
			return;
		}

		metadata_delete(mc, METADATA_CHANNEL, "disable_fantasy");

		logcommand(chansvs.me, u, CMDLOG_SET, "%s SET FANTASY ON", mc->name);
		notice(chansvs.nick, origin, "The \2FANTASY\2 flag has been set for \2%s\2.", mc->name);
		return;
	}
	else if (!strcasecmp("OFF", params))
	{
		metadata_t *md = metadata_find(mc, METADATA_CHANNEL, "disable_fantasy");

		if (md)
		{
			notice(chansvs.nick, origin, "Fantasy is already disabled on \2%s\2.", mc->name);
			return;
		}

		metadata_add(mc, METADATA_CHANNEL, "disable_fantasy", "on");

		logcommand(chansvs.me, u, CMDLOG_SET, "%s SET FANTASY OFF", mc->name);
		notice(chansvs.nick, origin, "The \2FANTASY\2 flag has been removed for \2%s\2.", mc->name);
		return;
	}

	else
	{
		notice(chansvs.nick, origin, STR_INVALID_PARAMS, "FANTASY");
		return;
	}
}

static void cs_set_staffonly(char *origin, char *name, char *params)
{
	mychan_t *mc;

	if (*name != '#')
	{
		notice(chansvs.nick, origin, STR_INVALID_PARAMS, "STAFFONLY");
		return;
	}

	if (!(mc = mychan_find(name)))
	{
		notice(chansvs.nick, origin, "\2%s\2 is not registered.", name);
		return;
	}

	if (!strcasecmp("ON", params))
	{
		if (MC_STAFFONLY & mc->flags)
		{
			notice(chansvs.nick, origin, "The \2STAFFONLY\2 flag is already set for \2%s\2.", mc->name);
			return;
		}

		snoop("SET:STAFFONLY:ON: for \2%s\2 by \2%s\2", mc->name, origin);
		logcommand(chansvs.me, user_find_named(origin), CMDLOG_SET, "%s SET STAFFONLY ON", mc->name);

		mc->flags |= MC_STAFFONLY;

		notice(chansvs.nick, origin, "The \2STAFFONLY\2 flag has been set for \2%s\2.", mc->name);

		return;
	}

	else if (!strcasecmp("OFF", params))
	{
		if (!(MC_STAFFONLY & mc->flags))
		{
			notice(chansvs.nick, origin, "The \2STAFFONLY\2 flag is not set for \2%s\2.", mc->name);
			return;
		}

		snoop("SET:STAFFONLY:OFF: for \2%s\2 by \2%s\2", mc->name, origin);
		logcommand(chansvs.me, user_find_named(origin), CMDLOG_SET, "%s SET STAFFONLY OFF", mc->name);

		mc->flags &= ~MC_STAFFONLY;

		notice(chansvs.nick, origin, "The \2STAFFONLY\2 flag has been removed for \2%s\2.", mc->name);

		return;
	}

	else
	{
		notice(chansvs.nick, origin, STR_INVALID_PARAMS, "STAFFONLY");
		return;
	}
}

static void cs_set_property(char *origin, char *name, char *params)
{
	user_t *u = user_find_named(origin);
	mychan_t *mc;
        char *property = strtok(params, " ");
        char *value = strtok(NULL, "");

	if (*name != '#')
	{
		notice(chansvs.nick, origin, STR_INVALID_PARAMS, "PROPERTY");
		return;
	}

        if (!property)
        {
                notice(chansvs.nick, origin, "Syntax: SET <#channel> PROPERTY <property> [value]");
                return;
        }

	/* do we really need to allow this? -- jilles */
        if (strchr(property, ':') && !has_priv(u, PRIV_METADATA))
        {
                notice(chansvs.nick, origin, "Invalid property name.");
                return;
        }

	if (!(mc = mychan_find(name)))
	{
		notice(chansvs.nick, origin, "\2%s\2 is not registered.", name);
		return;
	}

	if (!is_founder(mc, u->myuser))
	{
		notice(chansvs.nick, origin, "You are not authorized to perform this command.");
		return;
	}

	if (mc->metadata.count >= me.mdlimit)
	{
		notice(chansvs.nick, origin, "Cannot add \2%s\2 to \2%s\2 metadata table, it is full.",
						property, name);
		return;
	}

        if (strchr(property, ':'))
        	snoop("SET:PROPERTY: \2%s\2: \2%s\2/\2%s\2", mc->name, property, value);

	if (!value)
	{
		metadata_t *md = metadata_find(mc, METADATA_CHANNEL, property);

		if (!md)
		{
			notice(chansvs.nick, origin, "Metadata entry \2%s\2 was not set.", property);
			return;
		}

		metadata_delete(mc, METADATA_CHANNEL, property);
		logcommand(chansvs.me, u, CMDLOG_SET, "%s SET PROPERTY %s (deleted)", mc->name, property);
		notice(chansvs.nick, origin, "Metadata entry \2%s\2 has been deleted.", property);
		return;
	}

	if (strlen(property) > 32 || strlen(value) > 300)
	{
		notice(chansvs.nick, origin, "Parameters are too long. Aborting.");
		return;
	}

	metadata_add(mc, METADATA_CHANNEL, property, value);
	logcommand(chansvs.me, u, CMDLOG_SET, "%s SET PROPERTY %s to %s", mc->name, property, value);
	notice(chansvs.nick, origin, "Metadata entry \2%s\2 added.", property);
}

/* *INDENT-OFF* */

/* commands we understand */
struct set_command_ set_commands[] = {
  { "FOUNDER",    AC_NONE,  cs_set_founder    },
  { "MLOCK",      AC_NONE,  cs_set_mlock      },
  { "SECURE",     AC_NONE,  cs_set_secure     },
  { "VERBOSE",    AC_NONE,  cs_set_verbose    },
  { "URL",	  AC_NONE,  cs_set_url        },
  { "ENTRYMSG",	  AC_NONE,  cs_set_entrymsg   },
  { "PROPERTY",   AC_NONE,  cs_set_property   },
  { "EMAIL",      AC_NONE,  cs_set_email      },
  { "KEEPTOPIC",  AC_NONE,  cs_set_keeptopic  },
  { "FANTASY",    AC_NONE,  cs_set_fantasy    },
  { "STAFFONLY",  PRIV_CHAN_ADMIN, cs_set_staffonly  },
  { NULL, 0, NULL }
};

/* *INDENT-ON* */

struct set_command_ *set_cmd_find(char *origin, char *command)
{
	user_t *u = user_find_named(origin);
	struct set_command_ *c;

	for (c = set_commands; c->name; c++)
	{
		if (!strcasecmp(command, c->name))
		{
			if (has_priv(u, c->access))
				return c;

			/* otherwise... */
			else
			{
				if (has_any_privs(u))
					notice(chansvs.nick, origin, "You do not have %s privilege.", c->access);
				else
					notice(chansvs.nick, origin, "You are not authorized to perform this operation.");
				return NULL;
			}
		}
	}

	/* it's a command we don't understand */
	notice(chansvs.nick, origin, "Invalid command. Please use \2HELP\2 for help.");
	return NULL;
}
