/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains the main() routine.
 *
 * $Id: main.c 2979 2005-10-17 23:58:46Z alambert $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/main", FALSE, _modinit, _moddeinit,
	"$Id: main.c 2979 2005-10-17 23:58:46Z alambert $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_join(chanuser_t *cu);
static void cs_part(chanuser_t *cu);

list_t cs_cmdtree;
list_t cs_fcmdtree;
list_t cs_helptree;

/* main services client routine */
static void chanserv(char *origin, uint8_t parc, char *parv[])
{
	char *cmd, *s;
	char orig[BUFSIZE];
	boolean_t is_fcommand = FALSE;
	hook_cmessage_data_t cdata;

        if (!origin)
        {
                slog(LG_DEBUG, "services(): recieved a request with no origin!");
                return;
        }

	/* this should never happen */
	if (parv[parc - 2][0] == '&')
	{
		slog(LG_ERROR, "services(): got parv with local channel: %s", parv[0]);
		return;
	}

	/* is this a fantasy command? */
	if (parv[parc - 2][0] == '#' && chansvs.fantasy == TRUE)
		is_fcommand = TRUE;
	else if (parv[parc - 2][0] == '#')
		return;

	/* make a copy of the original for debugging */
	strlcpy(orig, parv[parc - 1], BUFSIZE);

	/* lets go through this to get the command */
	cmd = strtok(parv[parc - 1], " ");

	if (!cmd)
		return;

	/* ctcp? case-sensitive as per rfc */
	if (!strcmp(cmd, "\001PING"))
	{
		if (!(s = strtok(NULL, " ")))
			s = " 0 ";

		strip(s);
		notice(chansvs.nick, origin, "\001PING %s\001", s);
		return;
	}
	else if (!strcmp(cmd, "\001VERSION\001"))
	{
		notice(chansvs.nick, origin,
		       "\001VERSION atheme-%s. %s %s %s%s%s%s%s%s%s%s%s TS5ow\001",
		       version, revision, me.name,
		       (match_mapping) ? "A" : "",
		       (me.loglevel & LG_DEBUG) ? "d" : "",
		       (me.auth) ? "e" : "",
		       (config_options.flood_msgs) ? "F" : "",
		       (config_options.leave_chans) ? "l" : "",
		       (config_options.join_chans) ? "j" : "",
		       (config_options.leave_chans) ? "l" : "", (config_options.join_chans) ? "j" : "", (!match_mapping) ? "R" : "", (config_options.raw) ? "r" : "", (runflags & RF_LIVE) ? "n" : "");

		return;
	}
	else if (!strcmp(cmd, "\001CLIENTINFO\001"))
	{
		/* easter egg :X */
		notice(chansvs.nick, origin, "\001CLIENTINFO 114 97 107 97 117 114\001");
		return;
	}

	/* ctcps we don't care about are ignored */
	else if (*cmd == '\001')
		return;

	/* take the command through the hash table */
	if (!is_fcommand)
		command_exec(chansvs.disp, origin, cmd, &cs_cmdtree);
	else
	{
		fcommand_exec(parv[parc - 2], origin, cmd, &cs_fcmdtree);

		cdata.c = channel_find(parv[parc - 2]);
		cdata.msg = parv[parc - 1];
		hook_call_event("channel_message", &cdata);
	}
}

static void chanserv_config_ready(void *unused)
{
	if (chansvs.me)
		del_service(chansvs.me);

	chansvs.me = add_service(chansvs.nick, chansvs.user,
				 chansvs.host, chansvs.real, chanserv);
	chansvs.disp = chansvs.me->disp;

	hook_del_hook("config_ready", chanserv_config_ready);
}

void _modinit(module_t *m)
{
	hook_add_event("config_ready");
	hook_add_hook("config_ready", chanserv_config_ready);

	if (!cold_start)
	{
		chansvs.me = add_service(chansvs.nick, chansvs.user, chansvs.host, chansvs.real, chanserv);
		chansvs.disp = chansvs.me->disp;
	}

	hook_add_event("channel_join");
	hook_add_event("channel_part");
	hook_add_hook("channel_join", (void (*)(void *)) cs_join);
	hook_add_hook("channel_part", (void (*)(void *)) cs_part);
}

void _moddeinit(void)
{
	if (chansvs.me)
		del_service(chansvs.me);

	hook_del_hook("channel_join", (void (*)(void *)) cs_join);
	hook_del_hook("channel_part", (void (*)(void *)) cs_part);
}

static void cs_join(chanuser_t *cu)
{
	user_t *u = cu->user;
	channel_t *chan = cu->chan;
	mychan_t *mc;
	char hostbuf[BUFSIZE];

	/* This crap is all moved in from chanuser_add(). It's better having
	 * it here than in node.c. It still needs a massive cleanup. Use the
	 * utility function chanacs_user_has_flag().              --alambert
	 */

	if ((chan->nummembers == 1) && (irccasecmp(config_options.chan, chan->name)))
	{
		if ((mc = mychan_find(chan->name)) && (config_options.join_chans))
		{
			join(chan->name, chansvs.nick);
			mc->used = CURRTIME;	/* XXX this fires on non-op */
		}
	}

	/* auto stuff */
	if (((mc = mychan_find(chan->name))) && (u->myuser))
	{
		if ((mc->flags & MC_STAFFONLY) && !is_ircop(u) && !is_sra(u->myuser))
		{
			ban(chansvs.nick, chan->name, u);
			kick(chansvs.nick, chan->name, u->nick, "You are not authorized to be on this channel");
		}

		if (should_kick(mc, u->myuser))
		{
			ban(chansvs.nick, chan->name, u);
			kick(chansvs.nick, chan->name, u->nick, "User is banned from this channel");
		}

		if (should_owner(mc, u->myuser))
		{
			if (ircd->uses_owner && !(cu->modes & ircd->owner_mode))
			{
				cmode(chansvs.nick, chan->name, ircd->owner_mchar, CLIENT_NAME(u));
				cu->modes |= ircd->owner_mode;
			}
		}
		else if ((mc->flags & MC_SECURE) && (cu->modes & ircd->owner_mode) && ircd->uses_owner)
		{
			char *mbuf = sstrdup(ircd->owner_mchar);
			*mbuf = '-';

			cmode(chansvs.nick, chan->name, mbuf, CLIENT_NAME(u));
			cu->modes &= ~ircd->owner_mode;

			free(mbuf);
		}

		if (should_protect(mc, u->myuser))
		{
			if (ircd->uses_protect && !(cu->modes & ircd->protect_mode))
			{
				cmode(chansvs.nick, chan->name, ircd->protect_mchar, CLIENT_NAME(u));
				cu->modes |= ircd->protect_mode;
			}
		}
		else if ((mc->flags & MC_SECURE) && (cu->modes & ircd->protect_mode) && ircd->uses_protect)
		{
			char *mbuf = sstrdup(ircd->protect_mchar);
			*mbuf = '-';

			cmode(chansvs.nick, chan->name, mbuf, CLIENT_NAME(u));
			cu->modes &= ~ircd->protect_mode;

			free(mbuf);
		}

		if (should_op(mc, u->myuser))
		{
			if (!(cu->modes & CMODE_OP))
			{
				cmode(chansvs.nick, chan->name, "+o", CLIENT_NAME(u));
				cu->modes |= CMODE_OP;
			}
		}
		else if ((mc->flags & MC_SECURE) && (cu->modes & CMODE_OP))
		{
			cmode(chansvs.nick, chan->name, "-o", CLIENT_NAME(u));
			cu->modes &= ~CMODE_OP;
		}

		if (should_halfop(mc, u->myuser))
		{
			if (ircd->uses_halfops && !(cu->modes & ircd->halfops_mode))
			{
				cmode(chansvs.nick, chan->name, "+h", CLIENT_NAME(u));
				cu->modes |= ircd->halfops_mode;
			}
		}
		else if ((mc->flags & MC_SECURE) && (cu->modes & ircd->halfops_mode) && ircd->uses_halfops)
		{
			cmode(chansvs.nick, chan->name, "-h", CLIENT_NAME(u));
			cu->modes &= ~ircd->halfops_mode;
		}

		if (should_voice(mc, u->myuser) && !(cu->modes & CMODE_VOICE))
		{
			cmode(chansvs.nick, chan->name, "+v", CLIENT_NAME(u));
			cu->modes |= CMODE_VOICE;
		}
	}

	if (mc)
	{
		hostbuf[0] = '\0';

		strlcat(hostbuf, u->nick, BUFSIZE);
		strlcat(hostbuf, "!", BUFSIZE);
		strlcat(hostbuf, u->user, BUFSIZE);
		strlcat(hostbuf, "@", BUFSIZE);
		strlcat(hostbuf, u->host, BUFSIZE);

		/* the case u->myuser != NULL was treated above */
		if ((mc->flags & MC_STAFFONLY) && !is_ircop(u) && u->myuser == NULL)
		{
			ban(chansvs.nick, chan->name, u);
			kick(chansvs.nick, chan->name, u->nick, "You are not authorized to be on this channel");
		}


		if (should_kick_host(mc, hostbuf))
		{
			ban(chansvs.nick, chan->name, u);
			kick(chansvs.nick, chan->name, u->nick, "User is banned from this channel");
		}

		if (!should_owner(mc, u->myuser) && (cu->modes & ircd->owner_mode))
		{
			char *mbuf = sstrdup(ircd->owner_mchar);
			*mbuf = '-';

			cmode(chansvs.nick, chan->name, mbuf, CLIENT_NAME(u));
			cu->modes &= ~ircd->owner_mode;

			free(mbuf);
		}

		if (!should_protect(mc, u->myuser) && (cu->modes & ircd->protect_mode))
		{
			char *mbuf = sstrdup(ircd->protect_mchar);
			*mbuf = '-';

			cmode(chansvs.nick, chan->name, mbuf, CLIENT_NAME(u));
			cu->modes &= ~ircd->protect_mode;

			free(mbuf);
		}

		if (should_op_host(mc, hostbuf))
		{
			if (!(cu->modes & CMODE_OP))
			{
				cmode(chansvs.nick, chan->name, "+o", CLIENT_NAME(u));
				cu->modes |= CMODE_OP;
			}
		}
		else if ((mc->flags & MC_SECURE) && !should_op(mc, u->myuser) && (cu->modes & CMODE_OP))
		{
			cmode(chansvs.nick, chan->name, "-o", CLIENT_NAME(u));
			cu->modes &= ~CMODE_OP;
		}

		if (should_halfop_host(mc, hostbuf))
		{
			if (ircd->uses_halfops && !(cu->modes & ircd->halfops_mode))
			{
				cmode(chansvs.nick, chan->name, "+h", CLIENT_NAME(u));
				cu->modes |= ircd->halfops_mode;
			}
		}
		else if ((mc->flags & MC_SECURE) && ircd->uses_halfops && !should_halfop(mc, u->myuser) && (cu->modes & ircd->halfops_mode))
		{
			cmode(chansvs.nick, chan->name, "-h", CLIENT_NAME(u));
			cu->modes &= ~ircd->halfops_mode;
		}

		if (should_voice_host(mc, hostbuf))
		{
			if (!(cu->modes & CMODE_VOICE))
			{
				cmode(chansvs.nick, chan->name, "+v", CLIENT_NAME(u));
				cu->modes |= CMODE_VOICE;
			}
		}
	}

	if (mc)
		check_modes(mc);
}

static void cs_part(chanuser_t *cu)
{
	if (config_options.leave_chans && (cu->chan->nummembers == 1))
		part(cu->chan->name, chansvs.nick);
}
