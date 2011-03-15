/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Copyright (c) 2005-2007 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains protocol support for charybdis-based ircd.
 *
 */

#include "atheme.h"
#include "uplink.h"
#include "pmodule.h"
#include "protocol/charybdis.h"

DECLARE_MODULE_V1("protocol/charybdis", true, _modinit, NULL, PACKAGE_STRING, "Atheme Development Group <http://www.atheme.org>");

/* *INDENT-OFF* */

ircd_t Charybdis = {
        "Charybdis",			/* IRCd name */
        "$$",                           /* TLD Prefix, used by Global. */
        true,                           /* Whether or not we use IRCNet/TS6 UID */
        false,                          /* Whether or not we use RCOMMAND */
        false,                          /* Whether or not we support channel owners. */
        false,                          /* Whether or not we support channel protection. */
        false,                          /* Whether or not we support halfops. */
	false,				/* Whether or not we use P10 */
	false,				/* Whether or not we use vHosts. */
	CMODE_EXLIMIT | CMODE_PERM,	/* Oper-only cmodes */
        0,                              /* Integer flag for owner channel flag. */
        0,                              /* Integer flag for protect channel flag. */
        0,                              /* Integer flag for halfops. */
        "+",                            /* Mode we set for owner. */
        "+",                            /* Mode we set for protect. */
        "+",                            /* Mode we set for halfops. */
	PROTOCOL_CHARYBDIS,		/* Protocol type */
	CMODE_PERM,                     /* Permanent cmodes */
	0,                              /* Oper-immune cmode */
	"beIq",                         /* Ban-like cmodes */
	'e',                            /* Except mchar */
	'I',                            /* Invex mchar */
	IRCD_CIDR_BANS | IRCD_HOLDNICK  /* Flags */
};

struct cmode_ charybdis_mode_list[] = {
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
  { 'C', CMODE_NOCTCP },

  /* following modes are added as extensions */
  { 'N', CMODE_NPC       },
  { 'S', CMODE_SSLONLY   },
  { 'O', CMODE_OPERONLY  },
  { 'A', CMODE_ADMINONLY },

  { '\0', 0 }
};

static bool check_forward(const char *, channel_t *, mychan_t *, user_t *, myuser_t *);
static bool check_jointhrottle(const char *, channel_t *, mychan_t *, user_t *, myuser_t *);

struct extmode charybdis_ignore_mode_list[] = {
  { 'f', check_forward },
  { 'j', check_jointhrottle },
  { '\0', 0 }
};

struct cmode_ charybdis_status_mode_list[] = {
  { 'o', CSTATUS_OP    },
  { 'v', CSTATUS_VOICE },
  { '\0', 0 }
};

struct cmode_ charybdis_prefix_mode_list[] = {
  { '@', CSTATUS_OP    },
  { '+', CSTATUS_VOICE },
  { '\0', 0 }
};

struct cmode_ charybdis_user_mode_list[] = {
  { 'p', UF_IMMUNE   },
  { 'a', UF_ADMIN    },
  { 'i', UF_INVIS    },
  { 'o', UF_IRCOP    },
  { 'D', UF_DEAF     },
  { '\0', 0 }
};

/* *INDENT-ON* */

/* ircd allows forwards to existing channels; the target channel must be
 * +F or the setter must have ops in it */
static bool check_forward(const char *value, channel_t *c, mychan_t *mc, user_t *u, myuser_t *mu)
{
	channel_t *target_c;
	mychan_t *target_mc;
	chanuser_t *target_cu;

	if (*value != '#' || strlen(value) > 50)
		return false;
	if (u == NULL && mu == NULL)
		return true;
	target_c = channel_find(value);
	target_mc = mychan_find(value);
	if (target_c == NULL && target_mc == NULL)
		return false;
	if (target_c != NULL && target_c->modes & CMODE_FTARGET)
		return true;
	if (target_mc != NULL && target_mc->mlock_on & CMODE_FTARGET)
		return true;
	if (u != NULL)
	{
		target_cu = chanuser_find(target_c, u);
		if (target_cu != NULL && target_cu->modes & CSTATUS_OP)
			return true;
		if (chanacs_user_flags(target_mc, u) & CA_SET)
			return true;
	}
	else if (mu != NULL)
		if (chanacs_find(target_mc, entity(mu), CA_SET))
			return true;
	return false;
}

static bool check_jointhrottle(const char *value, channel_t *c, mychan_t *mc, user_t *u, myuser_t *mu)
{
	const char *p, *arg2;

	p = value, arg2 = NULL;
	while (*p != '\0')
	{
		if (*p == ':')
		{
			if (arg2 != NULL)
				return false;
			arg2 = p + 1;
		}
		else if (!isdigit(*p))
			return false;
		p++;
	}
	if (arg2 == NULL)
		return false;
	if (p - arg2 > 10 || arg2 - value - 1 > 10 || !atoi(value) || !atoi(arg2))
		return false;
	return true;
}

/* this may be slow, but it is not used much */
/* returns true if it matches, false if not */
/* note that the host part matches differently from a regular ban */
static bool extgecos_match(const char *mask, user_t *u)
{
	char hostgbuf[NICKLEN+USERLEN+HOSTLEN+GECOSLEN];
	char realgbuf[NICKLEN+USERLEN+HOSTLEN+GECOSLEN];

	snprintf(hostgbuf, sizeof hostgbuf, "%s!%s@%s#%s", u->nick, u->user, u->vhost, u->gecos);
	snprintf(realgbuf, sizeof realgbuf, "%s!%s@%s#%s", u->nick, u->user, u->host, u->gecos);
	return !match(mask, hostgbuf) || !match(mask, realgbuf);
}

static mowgli_node_t *charybdis_next_matching_ban(channel_t *c, user_t *u, int type, mowgli_node_t *first)
{
	chanban_t *cb;
	mowgli_node_t *n;
	char hostbuf[NICKLEN+USERLEN+HOSTLEN];
	char realbuf[NICKLEN+USERLEN+HOSTLEN];
	char ipbuf[NICKLEN+USERLEN+HOSTLEN];
	char strippedmask[NICKLEN+USERLEN+HOSTLEN+CHANNELLEN+2];
	char *p;
	bool negate, matched;
	int exttype;
	channel_t *target_c;

	snprintf(hostbuf, sizeof hostbuf, "%s!%s@%s", u->nick, u->user, u->vhost);
	snprintf(realbuf, sizeof realbuf, "%s!%s@%s", u->nick, u->user, u->host);
	/* will be nick!user@ if ip unknown, doesn't matter */
	snprintf(ipbuf, sizeof ipbuf, "%s!%s@%s", u->nick, u->user, u->ip);


	MOWGLI_ITER_FOREACH(n, first)
	{
		cb = n->data;
		
		/*
		 * strip any banforwards from the mask. (SRV-73)
		 * charybdis itself doesn't support banforward but i don't feel like copying
		 * this stuff into ircd-seven and it is possible that charybdis may support them
		 * one day.
		 *   --nenolod
		 */
		strlcpy(strippedmask, cb->mask, sizeof strippedmask);
		p = strrchr(strippedmask, '$');
		if (p != NULL && p != strippedmask)
			*p = 0;

		if (cb->type == type &&
				(!match(strippedmask, hostbuf) || !match(strippedmask, realbuf) || !match(strippedmask, ipbuf) || !match_cidr(strippedmask, ipbuf)))
			return n;
		if (cb->type == type && strippedmask[0] == '$')
		{
			p = strippedmask + 1;
			negate = *p == '~';
			if (negate)
				p++;
			exttype = *p++;
			if (exttype == '\0')
				continue;
			/* check parameter */
			if (*p++ != ':')
				p = NULL;
			switch (exttype)
			{
				case 'a':
					matched = u->myuser != NULL && !(u->myuser->flags & MU_WAITAUTH) && (p == NULL || !match(p, entity(u->myuser)->name));
					break;
				case 'c':
					if (p == NULL)
						continue;
					target_c = channel_find(p);
					if (target_c == NULL || (target_c->modes & (CMODE_PRIV | CMODE_SEC)))
						continue;
					matched = chanuser_find(target_c, u) != NULL;
					break;
				case 'o':
					matched = is_ircop(u);
					break;
				case 'r':
					if (p == NULL)
						continue;
					matched = !match(p, u->gecos);
					break;
				case 's':
					if (p == NULL)
						continue;
					matched = !match(p, u->server->name);
					break;
				case 'x':
					if (p == NULL)
						continue;
					matched = extgecos_match(p, u);
					break;
				default:
					continue;
			}
			if (negate ^ matched)
				return n;
		}
	}
	return NULL;
}

static bool charybdis_is_valid_host(const char *host)
{
	const char *p;

	for (p = host; *p != '\0'; p++)
		if (!((*p >= '0' && *p <= '9') || (*p >= 'A' && *p <= 'Z') ||
					(*p >= 'a' && *p <= 'z') || *p == '.'
					|| *p == ':' || *p == '-' || *p == '/'))
			return false;
	return true;
}

void _modinit(module_t * m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "protocol/ts6-generic");

	next_matching_ban = &charybdis_next_matching_ban;
	is_valid_host = &charybdis_is_valid_host;

	mode_list = charybdis_mode_list;
	ignore_mode_list = charybdis_ignore_mode_list;
	status_mode_list = charybdis_status_mode_list;
	prefix_mode_list = charybdis_prefix_mode_list;
	user_mode_list = charybdis_user_mode_list;
	ignore_mode_list_size = ARRAY_SIZE(charybdis_ignore_mode_list);

	ircd = &Charybdis;

	m->mflags = MODTYPE_CORE;

	pmodule_loaded = true;
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
