/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Copyright (c) 2005-2007 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains protocol support for charybdis-based ircd.
 *
 * $Id: charybdis.c 8223 2007-05-05 12:58:06Z jilles $
 */

#include "atheme.h"
#include "uplink.h"
#include "pmodule.h"
#include "protocol/charybdis.h"
#include "protocol/ircd-seven.h"

DECLARE_MODULE_V1("protocol/ircd-seven", true, _modinit, NULL, "$Id: charybdis.c 8223 2007-05-05 12:58:06Z jilles $", "Atheme Development Group <http://www.atheme.org>");

/* *INDENT-OFF* */

ircd_t Seven = {
        "ircd-seven",			/* IRCd name */
        "$$",                           /* TLD Prefix, used by Global. */
        true,                           /* Whether or not we use IRCNet/TS6 UID */
        false,                          /* Whether or not we use RCOMMAND */
        false,                          /* Whether or not we support channel owners. */
        false,                          /* Whether or not we support channel protection. */
        false,                          /* Whether or not we support halfops. */
	false,				/* Whether or not we use P10 */
	false,				/* Whether or not we use vHosts. */
	CMODE_EXLIMIT | CMODE_PERM | CMODE_IMMUNE, /* Oper-only cmodes */
        0,                              /* Integer flag for owner channel flag. */
        0,                              /* Integer flag for protect channel flag. */
        0,                              /* Integer flag for halfops. */
        "+",                            /* Mode we set for owner. */
        "+",                            /* Mode we set for protect. */
        "+",                            /* Mode we set for halfops. */
	PROTOCOL_CHARYBDIS,		/* Protocol type */
	CMODE_PERM,                     /* Permanent cmodes */
	CMODE_IMMUNE,                   /* Oper-immune cmode */
	"beIq",                         /* Ban-like cmodes */
	'e',                            /* Except mchar */
	'I',                            /* Invex mchar */
	IRCD_CIDR_BANS | IRCD_HOLDNICK  /* Flags */
};

struct cmode_ seven_mode_list[] = {
  { 'i', CMODE_INVITE },
  { 'm', CMODE_MOD    },
  { 'n', CMODE_NOEXT  },
  { 'p', CMODE_PRIV   },
  { 's', CMODE_SEC    },
  { 't', CMODE_TOPIC  },
  { 'c', CMODE_NOCOLOR},
  { 'r', CMODE_REGONLY},
  { 'z', CMODE_OPMOD  },
  { 'g', CMODE_FINVITE},
  { 'L', CMODE_EXLIMIT},
  { 'P', CMODE_PERM   },
  { 'F', CMODE_FTARGET},
  { 'Q', CMODE_DISFWD },
  { 'M', CMODE_IMMUNE },
  { '\0', 0 }
};

struct cmode_ seven_user_mode_list[] = {
  { 'p', UF_IMMUNE   },
  { 'a', UF_ADMIN    },
  { 'i', UF_INVIS    },
  { 'o', UF_IRCOP    },
  { '\0', 0 }
};

/* *INDENT-ON* */

static bool seven_is_valid_hostslash(const char *host)
{
        const char *p;
        bool dot = false;

        if (*host == '.' || *host == '/' || *host == ':')
                return false;

        for (p = host; *p != '\0'; p++)
        {
                if (*p == '.' || *p == ':' || *p == '/')
                        dot = true;
                else if (!((*p >= '0' && *p <= '9') || (*p >= 'A' && *p <= 'Z') ||
                                        (*p >= 'a' && *p <= 'z') || *p == '-'))
                        return false;
        }
        /* hyperion allows a trailing / but RichiH does not want it, whatever */
        if (dot && p[-1] == '/')
                return false;
        return dot;
}

/* The following m_functions are copied from generic_ts6, with additions to handle the
 * "identified" / "owns this nick" flag.
 */

static void m_nick(sourceinfo_t *si, int parc, char *parv[])
{
	server_t *s;
	user_t *u;

	/* got the right number of args for an introduction? */
	if (parc == 8)
	{
		s = server_find(parv[6]);
		if (!s)
		{
			slog(LG_DEBUG, "m_nick(): new user on nonexistant server: %s", parv[6]);
			return;
		}

		slog(LG_DEBUG, "m_nick(): new user on `%s': %s", s->name, parv[0]);

		u = user_add(parv[0], parv[4], parv[5], NULL, NULL, NULL, parv[7], s, atoi(parv[2]));
		if (u == NULL)
			return;

		user_mode(u, parv[3]);

		/* If server is not yet EOB we will do this later.
		 * This avoids useless "please identify" -- jilles */
		if (s->flags & SF_EOB)
			handle_nickchange(user_find(parv[0]));
	}

	/* if it's only 2 then it's a nickname change */
	else if (parc == 2)
	{
		bool realchange;

                if (!si->su)
                {       
                        slog(LG_DEBUG, "m_nick(): server trying to change nick: %s", si->s != NULL ? si->s->name : "<none>");
                        return;
                }

		slog(LG_DEBUG, "m_nick(): nickname change from `%s': %s", si->su->nick, parv[0]);

		realchange = irccasecmp(si->su->nick, parv[0]);

		if (user_changenick(si->su, parv[0], atoi(parv[1])))
			return;

		/* fix up +e if necessary -- jilles */
		if (realchange && should_reg_umode(si->su))
			/* changed nick to registered one, reset +e */
			sts(":%s ENCAP * IDENTIFIED %s %s", ME, CLIENT_NAME(si->su), si->su->nick);

		/* It could happen that our PING arrived late and the
		 * server didn't acknowledge EOB yet even though it is
		 * EOB; don't send double notices in that case -- jilles */
		if (si->su->server->flags & SF_EOB)
			handle_nickchange(si->su);
	}
	else
	{
		int i;
		slog(LG_DEBUG, "m_nick(): got NICK with wrong number of params");

		for (i = 0; i < parc; i++)
			slog(LG_DEBUG, "m_nick():   parv[%d] = %s", i, parv[i]);
	}
}

/* protocol-specific stuff to do on login */
static void seven_on_login(user_t *u, myuser_t *account, const char *wantedhost)
{
	if (!me.connected || u == NULL)
		return;

	sts(":%s ENCAP * SU %s %s", ME, CLIENT_NAME(u), account->name);

	if (should_reg_umode(u))
		sts(":%s ENCAP * IDENTIFIED %s %s", ME, CLIENT_NAME(u), u->nick);
}

static bool seven_on_logout(user_t *u, const char *account)
{
	if (!me.connected || u == NULL)
		return false;

	sts(":%s ENCAP * SU %s", ME, CLIENT_NAME(u));
	sts(":%s ENCAP * IDENTIFIED %s %s OFF", ME, CLIENT_NAME(u), u->nick);
	return false;
}

static void nick_group(hook_user_req_t *hdata)
{
	user_t *u;

	u = hdata->si->su != NULL && !irccasecmp(hdata->si->su->nick, hdata->mn->nick) ? hdata->si->su : user_find_named(hdata->mn->nick);
	if (u != NULL && should_reg_umode(u))
		sts(":%s ENCAP * IDENTIFIED %s %s", ME, CLIENT_NAME(u), u->nick);
}

static void nick_ungroup(hook_user_req_t *hdata)
{
	user_t *u;

	u = hdata->si->su != NULL && !irccasecmp(hdata->si->su->nick, hdata->mn->nick) ? hdata->si->su : user_find_named(hdata->mn->nick);
	if (u != NULL && !nicksvs.no_nick_ownership)
		sts(":%s ENCAP * IDENTIFIED %s %s OFF", ME, CLIENT_NAME(u), u->nick);
}

void _modinit(module_t * m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "protocol/charybdis");

	mode_list = seven_mode_list;
	user_mode_list = seven_user_mode_list;

	ircd_on_login = &seven_on_login;
	ircd_on_logout = &seven_on_logout;
	is_valid_host = &seven_is_valid_hostslash;

	pcommand_delete("NICK");
	pcommand_add("NICK", m_nick, 2, MSRC_USER | MSRC_SERVER);

	ircd = &Seven;

	hook_add_event("nick_group");
	hook_add_hook("nick_group", (void (*)(void *))nick_group);
	hook_add_event("nick_ungroup");
	hook_add_hook("nick_ungroup", (void (*)(void *))nick_ungroup);

	m->mflags = MODTYPE_CORE;

	pmodule_loaded = true;
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
