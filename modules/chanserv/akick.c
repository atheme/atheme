/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService AKICK functions.
 *
 */

#include "atheme.h"

static void cs_cmd_akick(sourceinfo_t *si, int parc, char *parv[]);
static void akick_timeout_check(void *arg);
static void akickdel_list_create(void *arg);

DECLARE_MODULE_V1
(
	"chanserv/akick", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

command_t cs_akick = { "AKICK", N_("Manipulates a channel's AKICK list."),
                        AC_NONE, 4, cs_cmd_akick, { .path = "cservice/akick" } };

typedef struct {
	time_t expiration;
	char host[HOSTLEN];
	char chan[CHANNELLEN];
	mowgli_node_t node;

} akick_timeout_t;

time_t akickdel_next;
mowgli_list_t akickdel_list;

static akick_timeout_t *akick_add_timeout(mychan_t *mc, char *host, time_t expireson);

mowgli_heap_t *akick_timeout_heap;

void _modinit(module_t *m)
{
        service_named_bind_command("chanserv", &cs_akick);

        akick_timeout_heap = mowgli_heap_create(sizeof(akick_timeout_t), 512, BH_NOW);

    	if (akick_timeout_heap == NULL)
    	{
    		m->mflags = MODTYPE_FAIL;
    		return;
    	}

    	event_add_once("akickdel_list_create", akickdel_list_create, NULL, 0);
}

void _moddeinit(module_unload_intent_t intent)
{
	service_named_unbind_command("chanserv", &cs_akick);
	mowgli_heap_destroy(akick_timeout_heap);
}

void cs_cmd_akick(sourceinfo_t *si, int parc, char *parv[])
{
	myentity_t *mt;
	mychan_t *mc;
	chanacs_t *ca, *ca2;
	metadata_t *md, *md2;
	mowgli_node_t *n, *tn;
	int operoverride = 0;
	char *chan;
	char *cmd;
	long duration;
	char expiry[512];
	char modestr[512];
	char *s;
	char *uname = parv[2];
	char *token = strtok(parv[3], " ");

	char *treason,reason[BUFSIZE];

	if (parc < 2)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "AKICK");
		command_fail(si, fault_needmoreparams, _("Syntax: AKICK <#channel> ADD|DEL|LIST [parameters]"));
		return;
	}

	if (parv[0][0] == '#')
		chan = parv[0], cmd = parv[1];
	else if (parv[1][0] == '#')
		cmd = parv[0], chan = parv[1];
	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "AKICK");
		command_fail(si, fault_badparams, _("Syntax: AKICK <#channel> ADD|DEL|LIST [parameters]"));
		return;
	}

	if ((!strcasecmp("DEL", cmd)) && (!uname))
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "AKICK");
		command_fail(si, fault_needmoreparams, _("Syntax: AKICK <#channel> DEL <nickname|hostmask>"));
		return;
	}
	else if ((!strcasecmp("ADD", cmd)) && (!uname))
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "AKICK");
		command_fail(si, fault_needmoreparams, _("Syntax: AKICK <#channel> ADD <nickname|hostmask> [!P|!T <minutes>] [reason]"));
		return;
	}

	/* make sure they're registered, logged in
	 * and the founder of the channel before
	 * we go any further.
	 */
	if (!si->smu)
	{
		/* if they're opers and just want to LIST, they don't have to log in */
		if (!(has_priv(si, PRIV_CHAN_AUSPEX) && !strcasecmp("LIST", cmd)))
		{
			command_fail(si, fault_noprivs, _("You are not logged in."));
			return;
		}
	}

	mc = mychan_find(chan);
	if (!mc)
	{
		command_fail(si, fault_nosuch_target, _("Channel \2%s\2 is not registered."), chan);
		return;
	}
	
	if (metadata_find(mc, "private:close:closer"))
	{
		command_fail(si, fault_noprivs, _("\2%s\2 is closed."), chan);
		return;
	}

	/* ADD */
	if (!strcasecmp("ADD", cmd))
	{
		/* A duration, reason or duration and reason. */
		if (token)
		{
			if (!strcasecmp(token, "!P")) /* A duration [permanent] */
			{
				duration = 0;
				treason = strtok(NULL, "");

				if (treason)
					strlcpy(reason, treason, BUFSIZE);
				else
					reason[0] = 0;
			}
			else if (!strcasecmp(token, "!T")) /* A duration [temporary] */
			{
				s = strtok(NULL, " ");
				treason = strtok(NULL, "");

				if (treason)
					strlcpy(reason, treason, BUFSIZE);
				else
					reason[0] = 0;

				if (s)
				{
					duration = (atol(s) * 60);
					while (isdigit(*s))
						s++;
					if (*s == 'h' || *s == 'H')
						duration *= 60;
					else if (*s == 'd' || *s == 'D')
						duration *= 1440;
					else if (*s == 'w' || *s == 'W')
						duration *= 10080;
					else if (*s == '\0')
						;
					else
						duration = 0;

					if (duration == 0)
					{
						command_fail(si, fault_badparams, _("Invalid duration given."));
						command_fail(si, fault_badparams, _("Syntax: AKICK <#channel> ADD <nick|hostmask> [!P|!T <minutes>] [reason]"));
						return;
					}
				}
				else
				{
					command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "AKICK ADD");
					command_fail(si, fault_needmoreparams, _("Syntax: AKILL <#channel> ADD <nick|hostmask> [!P|!T <minutes>] [reason]"));
					return;
				}
			}
			else
			{ /* A reason */
				duration = chansvs.akick_time;
				strlcpy(reason, token, BUFSIZE);
			}
		} 
		else
		{ /* No reason and no duration */
			duration = chansvs.akick_time;
			reason[0] = 0;
		}

		if ((chanacs_source_flags(mc, si) & (CA_FLAGS | CA_REMOVE)) != (CA_FLAGS | CA_REMOVE))
		{
			command_fail(si, fault_noprivs, _("You are not authorized to perform this operation."));
			return;
		}

		mt = myentity_find_ext(uname);
		if (!mt)
		{
			/* we might be adding a hostmask */
			if (!validhostmask(uname))
			{
				command_fail(si, fault_badparams, _("\2%s\2 is neither a nickname nor a hostmask."), uname);
				return;
			}

			uname = collapse(uname);

			ca = chanacs_find_host_literal(mc, uname, 0);
			if (ca != NULL)
			{
				if (ca->level & CA_AKICK)
					command_fail(si, fault_nochange, _("\2%s\2 is already on the AKICK list for \2%s\2"), uname, mc->name);
				else
					command_fail(si, fault_alreadyexists, _("\2%s\2 already has flags \2%s\2 on \2%s\2"), uname, bitmask_to_flags(ca->level), mc->name);
				return;
			}

			ca = chanacs_find_host(mc, uname, CA_AKICK);
			if (ca != NULL)
			{
				command_fail(si, fault_nochange, _("The more general mask \2%s\2 is already on the AKICK list for \2%s\2"), ca->host, mc->name);
				return;
			}

			/* new entry */
			ca2 = chanacs_open(mc, NULL, uname, true);
			if (chanacs_is_table_full(ca2))
			{
				command_fail(si, fault_toomany, _("Channel %s access list is full."), mc->name);
				chanacs_close(ca2);
				return;
			}
			chanacs_modify_simple(ca2, CA_AKICK, 0);

			if (reason[0])
				metadata_add(ca2, "reason", reason);

			if (duration > 0)
			{
				akick_timeout_t *timeout;
				time_t expireson = ca2->tmodified+duration;

				snprintf(expiry, sizeof expiry, "%ld", (duration > 0L ? expireson : 0L));
				metadata_add(ca2, "expires", expiry);

				verbose(mc, "\2%s\2 added \2%s\2 to the AKICK list, expires in %s.", get_source_name(si), uname,timediff(duration));
				logcommand(si, CMDLOG_SET, "AKICK:ADD: \2%s\2 on \2%s\2, expires in %s.", uname, mc->name,timediff(duration));
				command_success_nodata(si, _("AKICK on \2%s\2 was successfully added for \2%s\2 and will expire in %s."), uname, mc->name,timediff(duration) );

				timeout = akick_add_timeout(mc, uname, expireson);

				if (akickdel_next == 0 || akickdel_next > timeout->expiration)
				{
					if (akickdel_next != 0)
						event_delete(akick_timeout_check, NULL);

					akickdel_next = timeout->expiration;
					event_add_once("akick_timeout_check", akick_timeout_check, NULL, akickdel_next - CURRTIME);
				}
			}
			else
			{
				verbose(mc, "\2%s\2 added \2%s\2 to the AKICK list.", get_source_name(si), uname);
				logcommand(si, CMDLOG_SET, "AKICK:ADD: \2%s\2 on \2%s\2", uname, mc->name);

				command_success_nodata(si, _("AKICK on \2%s\2 was successfully added to the AKICK list for \2%s\2."), uname, mc->name);
			}

			hook_call_channel_acl_change(ca2);
			chanacs_close(ca2);

			return;
		}
		else
		{
			if ((ca = chanacs_find(mc, mt, 0x0)))
			{
				if (ca->level & CA_AKICK)
					command_fail(si, fault_nochange, _("\2%s\2 is already on the AKICK list for \2%s\2"), mt->name, mc->name);
				else
					command_fail(si, fault_alreadyexists, _("\2%s\2 already has flags \2%s\2 on \2%s\2"), mt->name, bitmask_to_flags(ca->level), mc->name);
				return;
			}

			/* new entry */
			ca2 = chanacs_open(mc, mt, NULL, true);
			if (chanacs_is_table_full(ca2))
			{
				command_fail(si, fault_toomany, _("Channel %s access list is full."), mc->name);
				chanacs_close(ca2);
				return;
			}
			chanacs_modify_simple(ca2, CA_AKICK, 0);

			if (reason[0])
				metadata_add(ca2, "reason", reason);

			if (duration > 0)
			{
				akick_timeout_t *timeout;
				time_t expireson = ca2->tmodified+duration;

				snprintf(expiry, sizeof expiry, "%ld", (duration > 0L ? ca2->tmodified+duration : 0L));
				metadata_add(ca2, "expires", expiry);

				command_success_nodata(si, _("AKICK on \2%s\2 was successfully added for \2%s\2 and will expire in %s."), mt->name, mc->name, timediff(duration));
				verbose(mc, "\2%s\2 added \2%s\2 to the AKICK list, expires in %s.", get_source_name(si), mt->name, timediff(duration));
				logcommand(si, CMDLOG_SET, "AKICK:ADD: \2%s\2 on \2%s\2, expires in %s", mt->name, mc->name, timediff(duration));

				timeout = akick_add_timeout(mc, mt->name, expireson);

				if (akickdel_next == 0 || akickdel_next > timeout->expiration)
				{
					if (akickdel_next != 0)
						event_delete(akick_timeout_check, NULL);
					akickdel_next = timeout->expiration;
					event_add_once("akick_timeout_check", akick_timeout_check, NULL, akickdel_next - CURRTIME);
				}
			}
			else
			{
				command_success_nodata(si, _("AKICK on \2%s\2 was successfully added to the AKICK list for \2%s\2."), mt->name, mc->name);

				verbose(mc, "\2%s\2 added \2%s\2 to the AKICK list.", get_source_name(si), mt->name);
				logcommand(si, CMDLOG_SET, "AKICK:ADD: \2%s\2 on \2%s\2", mt->name, mc->name);
			}

			hook_call_channel_acl_change(ca2);
			chanacs_close(ca2);
			return;
		}
	}
	else if (!strcasecmp("DEL", cmd))
	{
		akick_timeout_t *timeout;
		chanban_t *cb;
		char *vhost;

		if ((chanacs_source_flags(mc, si) & (CA_FLAGS | CA_REMOVE)) != (CA_FLAGS | CA_REMOVE))
		{
			command_fail(si, fault_noprivs, _("You are not authorized to perform this operation."));
			return;
		}

		mt = myentity_find_ext(uname);
		if (!mt)
		{
			/* we might be deleting a hostmask */
			ca = chanacs_find_host_literal(mc, uname, CA_AKICK);
			if (ca == NULL)
			{
				ca = chanacs_find_host(mc, uname, CA_AKICK);
				if (ca != NULL)
					command_fail(si, fault_nosuch_key, _("\2%s\2 is not on the AKICK list for \2%s\2, however \2%s\2 is."), uname, mc->name, ca->host);
				else
					command_fail(si, fault_nosuch_key, _("\2%s\2 is not on the AKICK list for \2%s\2."), uname, mc->name);
				return;
			}

			chanacs_modify_simple(ca, 0, CA_AKICK);
			hook_call_channel_acl_change(ca);
			chanacs_close(ca);

			verbose(mc, "\2%s\2 removed \2%s\2 from the AKICK list.", get_source_name(si), uname);
			logcommand(si, CMDLOG_SET, "AKICK:DEL: \2%s\2 on \2%s\2", uname, mc->name);
			command_success_nodata(si, _("\2%s\2 has been removed from the AKICK list for \2%s\2."), uname, mc->name);

			MOWGLI_ITER_FOREACH_SAFE(n, tn, akickdel_list.head)
			{
				timeout = n->data;
				if (!strcmp(timeout->host, uname) && !strcmp(timeout->chan, mc->name))
				{
					mowgli_node_delete(&timeout->node, &akickdel_list);
					mowgli_heap_free(akick_timeout_heap, timeout);
				}
			}

			if ((cb = chanban_find(mc->chan, uname, 'b')))
			{
				snprintf(modestr, sizeof modestr, "-b %s", uname);
				mode_sts(chansvs.nick, mc->chan, modestr);
				chanban_delete(cb);
			}
			return;
		}

		if (!(ca = chanacs_find(mc, mt, CA_AKICK)))
		{
			command_fail(si, fault_nosuch_key, _("\2%s\2 is not on the AKICK list for \2%s\2."), mt->name, mc->name);
			return;
		}

		if ((md = metadata_find(mt, "private:host:vhost")) && (vhost = strstr(md->value, "@")) && (++vhost))
		{
			snprintf(expiry, sizeof timeout->host, "*!*@%s", vhost);
			if ((cb = chanban_find(mc->chan, expiry, 'b')))
			{
				snprintf(modestr, sizeof modestr, "-b %s", timeout->host);
				mode_sts(chansvs.nick, mc->chan, modestr);
				chanban_delete(cb);
			}
		}

		MOWGLI_ITER_FOREACH_SAFE(n, tn, akickdel_list.head)
		{
			timeout = n->data;
			if (!strcmp(timeout->host, mt->name) && !strcmp(timeout->chan, mc->name))
			{
				mowgli_node_delete(&timeout->node, &akickdel_list);
				mowgli_heap_free(akick_timeout_heap, timeout);
			}
		}

		chanacs_modify_simple(ca, 0, CA_AKICK);
		hook_call_channel_acl_change(ca);
		chanacs_close(ca);

		command_success_nodata(si, _("\2%s\2 has been removed from the AKICK list for \2%s\2."), mt->name, mc->name);
		logcommand(si, CMDLOG_SET, "AKICK:DEL: \2%s\2 on \2%s\2", mt->name, mc->name);
		verbose(mc, "\2%s\2 removed \2%s\2 from the AKICK list.", get_source_name(si), mt->name);

		return;
	}
	else if (!strcasecmp("LIST", cmd))
	{
		int i = 0;

		if (!chanacs_source_has_flag(mc, si, CA_ACLVIEW))
		{
			if (has_priv(si, PRIV_CHAN_AUSPEX))
				operoverride = 1;
			else
			{
				command_fail(si, fault_noprivs, _("You are not authorized to perform this operation."));
				return;
			}
		}
		command_success_nodata(si, _("AKICK list for \2%s\2:"), mc->name);


		MOWGLI_ITER_FOREACH_SAFE(n, tn, mc->chanacs.head)
		{
			time_t expires_on = 0;
			char *ago;
			long time_left = 0;

			ca = (chanacs_t *)n->data;

			if (ca->level == CA_AKICK)
			{
				md = metadata_find(ca, "reason");

				/* check if it's a temporary akick */
				if ((md2 = metadata_find(ca, "expires")))
				{
					snprintf(expiry, sizeof expiry, "%s", md2->value);
					expires_on = (time_t)atol(expiry);
					time_left = difftime(expires_on, CURRTIME);
				}
				ago = ca->tmodified ? time_ago(ca->tmodified) : "?";

				if (ca->entity == NULL)
					command_success_nodata(si, _("%d: \2%s\2  %s [expires: %s, modified: %s ago]"),
							++i, ca->host,
							md ? md->value : "",
							expires_on > 0 ? timediff(time_left) : "never",
							ago);
				else if (isuser(ca->entity) && MOWGLI_LIST_LENGTH(&user(ca->entity)->logins) > 0)
					command_success_nodata(si, _("%d: \2%s\2 (logged in)  %s [expires: %s, modified: %s ago]"),
							++i, ca->entity->name,
							md ? md->value : "",
							expires_on > 0 ? timediff(time_left) : "never",
							ago);
				else
					command_success_nodata(si, _("%d: \2%s\2 (not logged in)  %s [expires: %s, modified: %s ago]"),
							++i, ca->entity->name,
							md ? md->value : "",
							expires_on > 0 ? timediff(time_left) : "never",
							ago);
			}

		}

		command_success_nodata(si, _("Total of \2%d\2 %s in \2%s\2's AKICK list."), i, (i == 1) ? "entry" : "entries", mc->name);
		if (operoverride)
			logcommand(si, CMDLOG_ADMIN, "AKICK:LIST: \2%s\2 (oper override)", mc->name);
		else
			logcommand(si, CMDLOG_GET, "AKICK:LIST: \2%s\2", mc->name);
	}
	else
		command_fail(si, fault_badparams, _("Invalid command. Use \2/%s%s help\2 for a command listing."), (ircd->uses_rcommand == false) ? "msg " : "", chansvs.me->disp);
}

void akick_timeout_check(void *arg)
{
	mowgli_node_t *n, *tn;
	akick_timeout_t *timeout;
	myentity_t *mt;
	chanacs_t *ca;
	metadata_t *md;
	char *vhost;

	char modestr[HOSTLEN+3];
	chanban_t *cb;
	akickdel_next = 0;

	MOWGLI_ITER_FOREACH_SAFE(n, tn, akickdel_list.head)
	{
		timeout = n->data;

		if (timeout->expiration > CURRTIME)
		{
			akickdel_next = timeout->expiration;
			event_add_once("akick_timeout_check", akick_timeout_check, NULL, akickdel_next - CURRTIME);
			break;
		}

		ca = NULL;

		mychan_t *mc = mychan_find(timeout->chan);
		if (strstr(timeout->host,"@"))
		{
			if ((ca = chanacs_find_host_literal(mc, timeout->host, CA_AKICK)) && (cb = chanban_find(mc->chan, ca->host, 'b')))
			{
				snprintf(modestr, sizeof modestr, "-b %s", ca->host);
				mode_sts(chansvs.nick, mc->chan, modestr);
				chanban_delete(cb);
			}
		}
		else
		{
			if ((mt = myentity_find(timeout->host)) && (ca = chanacs_find(mc, mt, CA_AKICK)) && (md = metadata_find(mt, "private:host:vhost")) && (vhost = strstr(md->value,"@")) && ++vhost)
			{
				snprintf(timeout->host, sizeof timeout->host, "*!*@%s", vhost);
				if ((cb = chanban_find(mc->chan, timeout->host, 'b')))
				{
					snprintf(modestr, sizeof modestr, "-b %s", timeout->host);
					mode_sts(chansvs.nick, mc->chan, modestr);
					chanban_delete(cb);
				}
			}
		}

		if (ca)
		{
			chanacs_modify_simple(ca, 0, CA_AKICK);
			chanacs_close(ca);
		}
		mowgli_node_delete(&timeout->node, &akickdel_list);
		mowgli_heap_free(akick_timeout_heap, timeout);

	}
}

static akick_timeout_t *akick_add_timeout(mychan_t *mc, char *host, time_t expireson)
{
	mowgli_node_t *n;
	akick_timeout_t *timeout, *timeout2;

	timeout = mowgli_heap_alloc(akick_timeout_heap);
	strlcpy(timeout->chan, mc->name, sizeof timeout->chan);
	strlcpy(timeout->host, host, sizeof timeout->host);
	timeout->expiration = expireson;

	MOWGLI_ITER_FOREACH_PREV(n, akickdel_list.tail)
	{
		timeout2 = n->data;
		if (timeout2->expiration <= timeout->expiration)
			break;
	}
	if (n == NULL)
		mowgli_node_add_head(timeout, &timeout->node, &akickdel_list);
	else if (n->next == NULL)
		mowgli_node_add(timeout, &timeout->node, &akickdel_list);
	else
		mowgli_node_add_before(timeout, &timeout->node, &akickdel_list, n->next);

	return timeout;
}

void akickdel_list_create(void *arg)
{
	mychan_t *mc;
	mowgli_node_t *n, *tn;
	chanacs_t *ca;
	metadata_t *md;
	time_t expireson;

	mowgli_patricia_iteration_state_t state;

	MOWGLI_PATRICIA_FOREACH(mc, &state, mclist)
	{
		MOWGLI_ITER_FOREACH_SAFE(n, tn, mc->chanacs.head)
		{
			ca = (chanacs_t *)n->data;
			
			if (!(ca->level & CA_AKICK))
				continue;

			md = metadata_find(ca, "expires");
			
			if (!md)
				continue;

			expireson = atol(md->value);
			
			if (CURRTIME > expireson)
			{
				chanacs_modify_simple(ca, 0, CA_AKICK);
				chanacs_close(ca);
			}
			else
				akick_add_timeout(mc, ca->host, expireson);
		}
	}
}
/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
