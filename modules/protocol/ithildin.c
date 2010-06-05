/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Copyright (c) 2005-2006 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains protocol support for bahamut-based ircd.
 *
 */

#include "atheme.h"
#include "uplink.h"
#include "pmodule.h"
#include "protocol/bahamut.h"

DECLARE_MODULE_V1("protocol/ithildin", true, _modinit, NULL, PACKAGE_STRING, "Atheme Development Group <http://www.atheme.org>");

/* *INDENT-OFF* */

ircd_t ithildin = {
        "Ithildin 1.1",                 /* IRCd name */
        "$",                            /* TLD Prefix, used by Global. */
        false,                          /* Whether or not we use IRCNet/TS6 UID */
        false,                          /* Whether or not we use RCOMMAND */
        false,                          /* Whether or not we support channel owners. */
        false,                          /* Whether or not we support channel protection. */
        false,                          /* Whether or not we support halfops. */
	false,				/* Whether or not we use P10 */
	false,				/* Whether or not we use vHosts. */
	CMODE_OPERONLY,			/* Oper-only cmodes */
        0,                              /* Integer flag for owner channel flag. */
        0,                              /* Integer flag for protect channel flag. */
        0,                              /* Integer flag for halfops. */
        "+",                            /* Mode we set for owner. */
        "+",                            /* Mode we set for protect. */
        "+",                            /* Mode we set for halfops. */
	PROTOCOL_BAHAMUT,		/* Protocol type */
	0,                              /* Permanent cmodes */
	0,                              /* Oper-immune cmode */
	"beI",                          /* Ban-like cmodes */
	'e',                            /* Except mchar */
	'I',                            /* Invex mchar */
	IRCD_HOLDNICK                   /* Flags */
};

struct extmode ithildin_ignore_mode_list[] = {
  { '\0', 0 }
};

/* *INDENT-ON* */

/* introduce a client */
static void ithildin_introduce_nick(user_t *u)
{
	const char *umode = user_get_umodestr(u);

	sts("NICK %s 1 %lu %s %s %s :%s", u->nick, (unsigned long)u->ts, u->user, u->host, me.name, u->gecos);
	sts(":%s MODE %s %s", u->nick, u->nick, umode);
}

static void m_nick(sourceinfo_t *si, int parc, char *parv[])
{
	server_t *s;
	user_t *u;
	bool realchange;

	/* -> NICK jilles 1 1136143909 ~jilles 192.168.1.5 jaguar.test :Jilles Tjoelker */
	if (parc == 7)
	{
		s = server_find(parv[5]);
		if (!s)
		{
			slog(LG_DEBUG, "m_nick(): new user on nonexistant server: %s", parv[5]);
			return;
		}

		slog(LG_DEBUG, "m_nick(): new user on `%s': %s", s->name, parv[0]);

		u = user_add(parv[0], parv[3], parv[4], NULL, NULL, NULL, parv[6], s, atoi(parv[2]));
		if (u == NULL)
			return;

		/* Ok, we have the user ready to go.
		 * Here's the deal -- if the user's SVID is before
		 * the start time, and not 0, then check to see
		 * if it's a registered account or not.
		 *
		 * If it IS registered, deal with that accordingly,
		 * via handle_burstlogin(). --nenolod
		 */
		handle_nickchange(u);
	}
	/* if it's only 2 then it's a nickname change */
	else if (parc == 2)
	{
		if (!si->su)
		{
			slog(LG_DEBUG, "m_nick(): server trying to change nick: %s", si->s != NULL ? si->s->name : "<none>");
			return;
		}

		slog(LG_DEBUG, "m_nick(): nickname change from `%s': %s", si->su->nick, parv[0]);

		realchange = irccasecmp(si->su->nick, parv[0]);

		if (user_changenick(si->su, parv[0], atoi(parv[1])))
			return;

		/* fix up +r if necessary -- jilles */
		if (realchange && should_reg_umode(si->su))
			/* changed nick to registered one, reset +r */
			sts(":%s SVSMODE %s +rd %lu", nicksvs.nick, parv[0], (unsigned long)CURRTIME);

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

static unsigned int ith_server_login(void) {
	int ret;
	ret = sts("PASS %s :TS", curr_uplink->pass);
	if (ret == 1) { return 1; }

	me.bursting = true;

	sts("SERVER %s 1 :%s", me.name, me.desc);
	sts("SVINFO 5 3 0 :%lu", (unsigned long)CURRTIME);

	services_init();
	return 0;
}

void _modinit(module_t * m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "protocol/bahamut");

	server_login = &ith_server_login;
	introduce_nick = &ithildin_introduce_nick;

	ignore_mode_list = ithildin_ignore_mode_list;
	ignore_mode_list_size = ARRAY_SIZE(ithildin_ignore_mode_list);

	ircd = &ithildin;

	pcommand_delete("NICK");
	pcommand_add("NICK", m_nick, 2, MSRC_USER | MSRC_SERVER);

	m->mflags = MODTYPE_CORE;

	pmodule_loaded = true;
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
