/*
 * Copyright (c) 2009 Celestin, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains a BService INFO which can show
 * botserv settings on channel or bot.
 *
 * $Id: info.c 7969 2009-04-08 19:19:38Z celestin $
 */

#include "atheme.h"
#include "botserv.h"

DECLARE_MODULE_V1
(
	"botserv/info", FALSE, _modinit, _moddeinit,
	"$Id: info.c 7969 2009-04-08 19:19:38Z celestin $",
	"Atheme Development Group <http://atheme.org/>"
);

static void bs_cmd_info(sourceinfo_t *si, int parc, char *parv[]);

command_t bs_info = { "INFO", N_("Allows you to see BotServ information about a channel or a bot."), AC_NONE, 1, bs_cmd_info };

list_t *bs_bots;
 

list_t *bs_cmdtree;
list_t *bs_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(bs_cmdtree, "botserv/main", "bs_cmdtree");
	MODULE_USE_SYMBOL(bs_helptree, "botserv/main", "bs_helptree");
	MODULE_USE_SYMBOL(bs_bots, "botserv/main", "bs_bots");

	command_add(&bs_info, bs_cmdtree);

	help_addentry(bs_helptree, "INFO", "help/botserv/info", NULL);
}

void _moddeinit()
{
	command_delete(&bs_info, bs_cmdtree);

	help_delentry(bs_helptree, "INFO");
}

/* ******************************************************************** */

botserv_bot_t* botserv_bot_find(char *name)
{
	node_t *n;

	if(name == NULL)
		return NULL;

	LIST_FOREACH(n, bs_bots->head)
	{
		botserv_bot_t *bot = (botserv_bot_t *) n->data;

		if (!irccasecmp(name, bot->nick))
			return bot;
	}

	return NULL;
}

/* ******************************************************************** */ 

static void bs_cmd_info(sourceinfo_t *si, int parc, char *parv[])
{
	char *dest = parv[0];
	mychan_t *mc = NULL;
	botserv_bot_t* bot = NULL;
	metadata_t *md;
	int comma = 0, i;
	char buf[BUFSIZE], strfbuf[32], *end;
	time_t registered;
	struct tm tm; 
	node_t *n;
	chanuser_t *cu; 

	if (parc < 1 || parv[0] == NULL)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "INFO");
		command_fail(si, fault_needmoreparams, _("Syntax: INFO <#channel>"));
		command_fail(si, fault_needmoreparams, _("Syntax: INFO <botnick>"));
		return;
	}
	
	if (parv[0][0] == '#')
	{
		mc = mychan_find(dest);
	} 
	else
	{
		bot = botserv_bot_find(dest);
	}

	if (bot != NULL)
	{
		command_success_nodata(si, _("Information for bot \2%s\2:"), bot->nick); 
		command_success_nodata(si, _("     Mask : %s@%s"), bot->user, bot->host);
		command_success_nodata(si, _("Real name : %s"), bot->real);
		registered = bot->registered; 
		tm = *localtime(&registered);
		strftime(strfbuf, sizeof(strfbuf) - 1, "%b %d %H:%M:%S %Y", &tm);
		command_success_nodata(si, _("  Created : %s (%s ago)"), strfbuf, time_ago(registered));
		if (bot->private)
			command_success_nodata(si, _("  Options : Private"));
		else
			command_success_nodata(si, _("  Options : None"));
		command_success_nodata(si, _("  Used on : %d channel(s)"), LIST_LENGTH(&bot->me->me->channels));
		if(has_priv(si, PRIV_CHAN_AUSPEX))
		{
			i = 0;
			LIST_FOREACH(n, bot->me->me->channels.head)
			{
				cu = (chanuser_t *)n->data;
				command_success_nodata(si, _("%d: %s"), ++i, cu->chan->name);
			} 
		}
	}
	else if (mc != NULL)
	{
		if (!chanacs_source_has_flag(mc, si, CA_ACLVIEW) && !has_priv(si, PRIV_CHAN_AUSPEX))
		{
			command_fail(si, fault_noprivs, _("You are not authorized to perform this operation."));
			return;
		}

		command_success_nodata(si, _("Information for channel \2%s\2:"), mc->name); 
		if ((md = metadata_find(mc, "private:botserv:bot-assigned")) != NULL)
			command_success_nodata(si, _("         Bot nick : %s"), md->value); 
		else
			command_success_nodata(si, _("         Bot nick : not assigned yet")); 
		if ((md = metadata_find(mc, "private:botserv:kick-badwords")) != NULL)
			command_success_nodata(si, _(" Bad words kicker : enabled")); 
		else
			command_success_nodata(si, _(" Bad words kicker : disabled")); 
		if ((md = metadata_find(mc, "private:botserv:kick-bolds")) != NULL)
			command_success_nodata(si, _("     Bolds kicker : enabled")); 
		else
			command_success_nodata(si, _("     Bolds kicker : disabled")); 
		if ((md = metadata_find(mc, "private:botserv:kick-caps")) != NULL)
			command_success_nodata(si, _("      Caps kicker : enabled")); 
		else
			command_success_nodata(si, _("      Caps kicker : disabled")); 
		if ((md = metadata_find(mc, "private:botserv:kick-colors")) != NULL)
			command_success_nodata(si, _("    Colors kicker : enabled")); 
		else
			command_success_nodata(si, _("    Colors kicker : disabled")); 
		if ((md = metadata_find(mc, "private:botserv:kick-flood")) != NULL)
			command_success_nodata(si, _("     Flood kicker : enabled")); 
		else
			command_success_nodata(si, _("     Flood kicker : disabled")); 
		if ((md = metadata_find(mc, "private:botserv:kick-repeat")) != NULL)
			command_success_nodata(si, _("    Repeat kicker : enabled")); 
		else
			command_success_nodata(si, _("    Repeat kicker : disabled")); 
		if ((md = metadata_find(mc, "private:botserv:kick-reverses")) != NULL)
			command_success_nodata(si, _("  Reverses kicker : enabled")); 
		else
			command_success_nodata(si, _("  Reverses kicker : disabled")); 
		if ((md = metadata_find(mc, "private:botserv:kick-underlines")) != NULL)
			command_success_nodata(si, _("Underlines kicker : enabled")); 
		else
			command_success_nodata(si, _("Underlines kicker : disabled")); 
		end = buf;
		*end = 0;
		if ((md = metadata_find(mc, "private:botserv:bot-dontkick-ops")) != NULL)
		{
			end += snprintf(end, sizeof(buf) - (end - buf), "%s", "Dont kick ops"); 
			comma = 1;
		}
		if ((md = metadata_find(mc, "private:botserv:bot-dontkick-voices")) != NULL)
		{
			end += snprintf(end, sizeof(buf) - (end - buf), "%s%s", comma?", ":"", "Dont kick voices"); 
			comma = 1;
		}
		if ((md = metadata_find(mc, "private:botserv:bot-handle-fantasy")) != NULL)
		{
			end += snprintf(end, sizeof(buf) - (end - buf), "%s%s", comma?", ":"", "Fantasy"); 
			comma = 1;
		}
		if ((md = metadata_find(mc, "private:botserv:bot-handle-greet")) != NULL)
		{
			end += snprintf(end, sizeof(buf) - (end - buf), "%s%s", comma?", ":"", "Greet"); 
			comma = 1;
		}
		if ((md = metadata_find(mc, "private:botserv:no-bot")) != NULL)
		{
			end += snprintf(end, sizeof(buf) - (end - buf), "%s%s", (comma) ? ", ":"", "No bot"); 
			comma = 1;
		}
		command_success_nodata(si, _("          Options : %s"), (*buf) ? buf : "None"); 
	}
	else
	{
		command_fail(si, fault_nosuch_target, STR_INSUFFICIENT_PARAMS, "INFO");
		command_fail(si, fault_nosuch_target, _("Syntax: INFO <#channel>"));
		command_fail(si, fault_nosuch_target, _("Syntax: INFO <botnick>"));
	}
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */ 
 
 
