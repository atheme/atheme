/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2003-2004 E. Will, et al.
 * Copyright (C) 2005-2008 Atheme Project (http://atheme.org/)
 *
 * This file contains protocol support for solanum.
 */

#include <atheme.h>
#include <atheme/protocol/solanum.h>

#define UF_NOLOGOUT UF_CUSTOM1
#define UF_HELPER   UF_CUSTOM2

static struct ircd Solanum = {
	.ircdname = "solanum",
	.tldprefix = "$$",
	.uses_uid = true,
	.uses_rcommand = false,
	.uses_owner = false,
	.uses_protect = false,
	.uses_halfops = false,
	.uses_p10 = false,
	.uses_vhost = false,
	.oper_only_modes = CMODE_EXLIMIT | CMODE_PERM | CMODE_IMMUNE,
	.owner_mode = 0,
	.protect_mode = 0,
	.halfops_mode = 0,
	.owner_mchar = "+",
	.protect_mchar = "+",
	.halfops_mchar = "+",
	.type = PROTOCOL_CHARYBDIS,
	.perm_mode = CMODE_PERM,
	.oimmune_mode = CMODE_IMMUNE,
	.ban_like_modes = "beIq",
	.except_mchar = 'e',
	.invex_mchar = 'I',
	.flags = IRCD_CIDR_BANS | IRCD_HOLDNICK | IRCD_TOPIC_NOCOLOUR,
};

static const struct cmode solanum_mode_list[] = {
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
  { 'C', CMODE_NOCTCP },

  // following modes are added as extensions
  { 'N', CMODE_NPC	 },
  { 'S', CMODE_SSLONLY	 },
  { 'O', CMODE_OPERONLY  },
  { 'A', CMODE_ADMINONLY },
  { 'u', CMODE_NOFILTER  },
  { 'T', CMODE_NONOTICE  },
  { 'R', CMODE_MODREG    },

  { '\0', 0 }
};

static const struct cmode solanum_user_mode_list[] = {
  { 'p', UF_IMMUNE   },
  { 'a', UF_ADMIN    },
  { 'i', UF_INVIS    },
  { 'o', UF_IRCOP    },
  { 'O', UF_HELPER   },
  { 'D', UF_DEAF     },
  { 'S', UF_SERVICE  },
  { 'V', UF_NOLOGOUT },
  { '\0', 0 }
};

static struct sourceinfo *current_si = NULL;

static bool
solanum_is_valid_hostslash(const char *host)
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

	// hyperion allows a trailing / but RichiH does not want it, whatever
	if (dot && p[-1] == '/')
		return false;

	return dot;
}

static const char *
current_time(void)
{
	static char buf[32];
	struct tm *tm = NULL;
	unsigned int ms = 0;

#if defined(TIME_UTC)
	struct timespec ts;
	timespec_get(&ts, TIME_UTC);
	tm = gmtime(&ts.tv_sec);
	ms = ts.tv_nsec / 1000000;
#elif defined(CLOCK_REALTIME)
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	tm = gmtime(&ts.tv_sec);
	ms = ts.tv_nsec / 1000000;
#elif defined(HAVE_GETTIMEOFDAY)
	struct timeval tv;
	gettimeofday(&tv, NULL);
	tm = gmtime(&tv.tv_sec);
	ms = tv.tv_usec / 1000;
#else
	time_t t = time(NULL);
	tm = gmtime(&t);
#endif

	snprintf(buf, sizeof(buf), "%04d-%02d-%02dT%02d:%02d:%02d.%03uZ",
		tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
		tm->tm_hour, tm->tm_min, tm->tm_sec, ms);

	return buf;
}

static const char *
response_tag(void)
{
	static char buf[128];

	if (current_si == NULL || current_si->tags == NULL)
		return "";

	const char *response = mowgli_patricia_retrieve(current_si->tags, "solanum.chat/response");
	if (response == NULL)
		return "";

	snprintf(buf, sizeof(buf), ";solanum.chat/response=%s", response);
	return buf;
}

static int ATHEME_FATTR_PRINTF(1, 2)
solanum_sts(const char *fmt, ...)
{
	va_list ap;
	char buf[BUFSIZE+1];
	char buf2[BUFSIZE+1];
	size_t off = 0;

	return_val_if_fail(fmt != NULL, 0);

	va_start(ap, fmt);
	vsnprintf(buf2, sizeof(buf2), fmt, ap);
	va_end(ap);

	if (ircd->flags & IRCD_MESSAGE_TAGS)
	{
		off = snprintf(buf, sizeof(buf), "@time=%s%s", current_time(), response_tag());
		// merge any tags passed into sts()
		if (*buf2 == '@')
			*buf2 = ';';
		else
			buf[off++] = ' ';
	}

	mowgli_strlcpy(buf + off, buf2, sizeof(buf) - off);
	return send_line(buf);
}

void
solanum_handle_command(struct proto_cmd *pcmd, struct sourceinfo *si, int parc, char *parv[])
{
	current_si = si;

	if (pcmd->handler)
		pcmd->handler(si, parc, parv);

	current_si = NULL;

	if (si->tags != NULL && ircd->flags & IRCD_MESSAGE_TAGS)
	{
		const char *response = mowgli_patricia_retrieve(si->tags, "solanum.chat/response");
		const char *mask;
		if (response == NULL)
			return;

		char buf[128];
		char *p;
		mowgli_strlcpy(buf, response, sizeof(buf));

		// skip past the first two tokens; we only care about the third (the mask)
		if (strtok_r(buf, ",", &p) == NULL)
			return;
		if (strtok_r(NULL, ",", &p) == NULL)
			return;
		if ((mask = strtok_r(NULL, ",", &p)) == NULL)
			return;

		// no ACK if the mask doesn't match us
		if (match(mask, me.name))
			return;

		if (si->s != NULL)
			sts("@solanum.chat/response=%s :%s ENCAP %s ACK", response, me.name, si->s->name);
		else if (si->su != NULL)
			sts("@solanum.chat/response=%s :%s ENCAP %s ACK", response, me.name, si->su->server->name);
	}
}

static void
solanum_wallops_sts(const char *reason)
{
	sts(":%s ENCAP * SNOTE s :%s", ME, reason);
}

/* The following m_functions are copied from generic_ts6, with additions to handle the
 * "identified" / "owns this nick" flag.
 */
static void
m_euid(struct sourceinfo *si, int parc, char *parv[])
{
	struct server *s;
	struct user *u;

	// got the right number of args for an introduction?
	if (parc >= 11)
	{
		s = si->s;
		slog(LG_DEBUG, "m_euid(): new user on `%s': %s", s->name, parv[0]);

		u = user_add(parv[0],				// nick
			parv[4],				// user
			*parv[8] != '*' ? parv[8] : parv[5],	// hostname
			parv[5],				// hostname (visible)
			parv[6],				// ip
			parv[7],				// uid
			parv[parc - 1],				// gecos
			s,					// object parent (server)
			atoll(parv[2]));			// hopcount
		if (u == NULL)
			return;

		user_mode(u, parv[3]);
		if (*parv[9] != '*')
		{
			handle_burstlogin(u, parv[9], 0);

			/* If an account is given in burst, then either they logged in with sasl,
			 * or they logged in before a split and are now returning. Either way we need
			 * to check for identified-to-nick status and update the ircd state accordingly.
			 * For sasl they should be marked identified, and when returning from a split
			 * their nick may have been ungrouped, they may have changed nicks, or their account
			 * may have been dropped.
			 */
			if (authservice_loaded)
				sts(":%s ENCAP * IDENTIFIED %s %s %s", ME, CLIENT_NAME(u), u->nick,
						should_reg_umode(u) ? "" : "OFF");
		}

		/* server_eob() cannot know if a user was introduced
		 * with NICK/UID or EUID and handle_nickchange() must
		 * be called exactly once for each new user -- jilles */
		if (s->flags & SF_EOB)
			handle_nickchange(u);
	}
	else
	{
		int i;
		slog(LG_DEBUG, "m_euid(): got EUID with wrong number of params");

		for (i = 0; i < parc; i++)
			slog(LG_DEBUG, "m_euid():   parv[%d] = %s", i, parv[i]);
	}
}

static void
m_nick(struct sourceinfo *si, int parc, char *parv[])
{
	struct server *s;
	struct user *u;

	// got the right number of args for an introduction?
	if (parc == 8)
	{
		s = server_find(parv[6]);
		if (!s)
		{
			slog(LG_DEBUG, "m_nick(): new user on nonexistent server: %s", parv[6]);
			return;
		}

		slog(LG_DEBUG, "m_nick(): new user on `%s': %s", s->name, parv[0]);

		u = user_add(parv[0], parv[4], parv[5], NULL, NULL, NULL, parv[7], s, atoll(parv[2]));
		if (u == NULL)
			return;

		user_mode(u, parv[3]);

		/* If server is not yet EOB we will do this later.
		 * This avoids useless "please identify" -- jilles */
		if (s->flags & SF_EOB)
			handle_nickchange(user_find(parv[0]));
	}

	// if it's only 2 then it's a nickname change
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

		if (user_changenick(si->su, parv[0], atoll(parv[1])))
			return;

		// fix up +e if necessary -- jilles
		if (realchange && should_reg_umode(si->su))
			// changed nick to registered one, reset +e
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

static void
solanum_on_login(struct user *u, struct myuser *mu, const char *wantedhost)
{
	return_if_fail(u != NULL);

	sts(":%s ENCAP * SU %s %s", ME, CLIENT_NAME(u), entity(mu)->name);

	if (should_reg_umode(u))
		sts(":%s ENCAP * IDENTIFIED %s %s", ME, CLIENT_NAME(u), u->nick);
}

static bool
solanum_on_logout(struct user *u, const char *account)
{
	return_val_if_fail(u != NULL, false);

	sts(":%s ENCAP * SU %s", ME, CLIENT_NAME(u));
	sts(":%s ENCAP * IDENTIFIED %s %s OFF", ME, CLIENT_NAME(u), u->nick);
	return false;
}

static bool
solanum_is_ircop(struct user *u)
{
	if ((UF_IRCOP | UF_HELPER) & u->flags)
		return true;

	return false;
}

static void
nick_group(struct hook_user_req *hdata)
{
	struct user *u;

	u = hdata->si->su != NULL && !irccasecmp(hdata->si->su->nick, hdata->mn->nick) ? hdata->si->su : user_find_named(hdata->mn->nick);
	if (u != NULL && should_reg_umode(u))
		sts(":%s ENCAP * IDENTIFIED %s %s", ME, CLIENT_NAME(u), u->nick);
}

static void
nick_ungroup(struct hook_user_req *hdata)
{
	struct user *u;

	u = hdata->si->su != NULL && !irccasecmp(hdata->si->su->nick, hdata->mn->nick) ? hdata->si->su : user_find_named(hdata->mn->nick);
	if (u != NULL && !nicksvs.no_nick_ownership)
		sts(":%s ENCAP * IDENTIFIED %s %s OFF", ME, CLIENT_NAME(u), u->nick);
}

static void
user_can_logout(struct hook_user_logout_check *hdata)
{
	if (hdata->u && (hdata->u->flags & UF_NOLOGOUT) && !hdata->relogin)
		hdata->allowed = false;
}

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "protocol/charybdis")

	mode_list = solanum_mode_list;
	user_mode_list = solanum_user_mode_list;

	sts = &solanum_sts;
	handle_command = &solanum_handle_command;
	wallops_sts = &solanum_wallops_sts;
	ircd_on_login = &solanum_on_login;
	ircd_on_logout = &solanum_on_logout;
	is_valid_host = &solanum_is_valid_hostslash;
	is_ircop = &solanum_is_ircop;

	pcommand_delete("NICK");
	pcommand_add("NICK", m_nick, 2, MSRC_USER | MSRC_SERVER);
	pcommand_delete("EUID");
	pcommand_add("EUID", m_euid, 11, MSRC_SERVER);

	ircd = &Solanum;

	hook_add_nick_group(nick_group);
	hook_add_nick_ungroup(nick_ungroup);
	hook_add_user_can_logout(user_can_logout);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{

}

SIMPLE_DECLARE_MODULE_V1("protocol/solanum", MODULE_UNLOAD_CAPABILITY_NEVER)
