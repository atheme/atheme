/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2003-2004 E. Will, et al.
 * Copyright (C) 2005-2007 Atheme Project (http://atheme.org/)
 *
 * This file contains protocol support for charybdis-based ircd.
 */

#include <atheme.h>
#include <atheme/protocol/charybdis.h>

static struct ircd Charybdis = {
	.ircdname = "Charybdis",
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
	.flags = IRCD_CIDR_BANS | IRCD_HOLDNICK,
};

static const struct cmode charybdis_mode_list[] = {
  { 'i', CMODE_INVITE },
  { 'm', CMODE_MOD    },
  { 'n', CMODE_NOEXT  },
  { 'p', CMODE_PRIV   },
  { 's', CMODE_SEC    },
  { 't', CMODE_TOPIC  },
  { 'r', CMODE_REGONLY},
  { 'z', CMODE_OPMOD  },
  { 'g', CMODE_FINVITE},
  { 'L', CMODE_EXLIMIT},
  { 'P', CMODE_PERM   },
  { 'F', CMODE_FTARGET},
  { 'Q', CMODE_DISFWD },

  // following modes are added as extensions
  { 'N', CMODE_NPC	 },
  { 'S', CMODE_SSLONLY	 },
  { 'O', CMODE_OPERONLY  },
  { 'A', CMODE_ADMINONLY },
  { 'c', CMODE_NOCOLOR	 },
  { 'C', CMODE_NOCTCP	 },
  { 'T', CMODE_NONOTICE  },
  { 'M', CMODE_IMMUNE	 },
  { 'u', CMODE_NOFILTER	 },

  { '\0', 0 }
};

static const struct cmode charybdis_status_mode_list[] = {
  { 'o', CSTATUS_OP    },
  { 'v', CSTATUS_VOICE },
  { '\0', 0 }
};

static const struct cmode charybdis_prefix_mode_list[] = {
  { '@', CSTATUS_OP    },
  { '+', CSTATUS_VOICE },
  { '\0', 0 }
};

static const struct cmode charybdis_user_mode_list[] = {
  { 'p', UF_IMMUNE   },
  { 'a', UF_ADMIN    },
  { 'i', UF_INVIS    },
  { 'o', UF_IRCOP    },
  { 'D', UF_DEAF     },
  { 'S', UF_SERVICE  },
  { '\0', 0 }
};

/* ircd allows forwards to existing channels; the target channel must be
 * +F or the setter must have ops in it */
static bool
check_forward(const char *value, struct channel *c, struct mychan *mc, struct user *u, struct myuser *mu)
{
	struct channel *target_c;
	struct mychan *target_mc;
	struct chanuser *target_cu;

	if (!VALID_GLOBAL_CHANNEL_PFX(value) || strlen(value) > 50)
		return false;
	if (u == NULL && mu == NULL)
		return true;
	target_c = channel_find(value);
	target_mc = mychan_from(target_c);
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
		if (chanacs_entity_has_flag(target_mc, entity(mu), CA_SET))
			return true;
	return false;
}

static bool
check_jointhrottle(const char *value, struct channel *c, struct mychan *mc, struct user *u, struct myuser *mu)
{
	const char *p, *arg2;

	p = value;
	arg2 = NULL;

	while (*p != '\0')
	{
		if (*p == ':')
		{
			if (arg2 != NULL)
				return false;
			arg2 = p + 1;
		}
		else if (!isdigit((unsigned char)*p))
			return false;
		p++;
	}
	if (arg2 == NULL)
		return false;
	if (p - arg2 > 10 || arg2 - value - 1 > 10 || !atoi(value) || !atoi(arg2))
		return false;
	return true;
}

static struct extmode charybdis_ignore_mode_list[] = {
  { 'f', check_forward },
  { 'j', check_jointhrottle },
  { '\0', 0 }
};

/* this may be slow, but it is not used much
 * returns true if it matches, false if not
 * note that the host part matches differently from a regular ban */
static bool
extgecos_match(const char *mask, struct user *u)
{
	char hostgbuf[NICKLEN + 1 + USERLEN + 1 + HOSTLEN + 1 + GECOSLEN + 1];
	char realgbuf[NICKLEN + 1 + USERLEN + 1 + HOSTLEN + 1 + GECOSLEN + 1];

	snprintf(hostgbuf, sizeof hostgbuf, "%s!%s@%s#%s", u->nick, u->user, u->vhost, u->gecos);
	snprintf(realgbuf, sizeof realgbuf, "%s!%s@%s#%s", u->nick, u->user, u->host, u->gecos);
	return !match(mask, hostgbuf) || !match(mask, realgbuf);
}

static mowgli_node_t *
charybdis_next_matching_ban(struct channel *c, struct user *u, int type, mowgli_node_t *first)
{
	struct chanban *cb;
	mowgli_node_t *n;
	char hostbuf[NICKLEN + 1 + USERLEN + 1 + HOSTLEN + 1];
	char realbuf[NICKLEN + 1 + USERLEN + 1 + HOSTLEN + 1];
	char ipbuf[NICKLEN + 1 + USERLEN + 1 + HOSTLEN + 1];
	char strippedmask[NICKLEN + 1 + USERLEN + 1 + HOSTLEN + 1 + CHANNELLEN + 3];
	char *p;
	bool negate, matched;
	int exttype;
	struct channel *target_c;

	snprintf(hostbuf, sizeof hostbuf, "%s!%s@%s", u->nick, u->user, u->vhost);
	snprintf(realbuf, sizeof realbuf, "%s!%s@%s", u->nick, u->user, u->host);

	// will be nick!user@ if ip unknown, doesn't matter
	snprintf(ipbuf, sizeof ipbuf, "%s!%s@%s", u->nick, u->user, u->ip);

	MOWGLI_ITER_FOREACH(n, first)
	{
		cb = n->data;

		if (cb->type != type)
			continue;

		/*
		 * strip any banforwards from the mask. (SRV-73)
		 * charybdis itself doesn't support banforward but i don't feel like copying
		 * this stuff into ircd-seven and it is possible that charybdis may support them
		 * one day.
		 *   --nenolod
		 */
		mowgli_strlcpy(strippedmask, cb->mask, sizeof strippedmask);
		p = strrchr(strippedmask, '$');
		if (p != NULL && p != strippedmask)
			*p = 0;

		if ((!match(strippedmask, hostbuf) || !match(strippedmask, realbuf) || !match(strippedmask, ipbuf) || !match_cidr(strippedmask, ipbuf)))
			return n;
		if (strippedmask[0] == '$')
		{
			p = strippedmask + 1;
			negate = *p == '~';
			if (negate)
				p++;
			exttype = *p++;
			if (exttype == '\0')
				continue;

			// check parameter
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

static bool
charybdis_is_valid_host(const char *host)
{
	const char *p;

	for (p = host; *p != '\0'; p++)
		if (!((*p >= '0' && *p <= '9') || (*p >= 'A' && *p <= 'Z') ||
					(*p >= 'a' && *p <= 'z') || *p == '.'
					|| *p == ':' || *p == '-' || *p == '/'))
			return false;
	return true;
}

static void
charybdis_notice_channel_sts(struct user *from, struct channel *target, const char *text)
{
	sts(":%s NOTICE %s :%s", from ? CLIENT_NAME(from) : ME, target->name, text);
}

static bool
charybdis_is_extban(const char *mask)
{
	const char without_param[] = "oza";
	const char with_param[] = "ajcxr";
	const size_t mask_len = strlen(mask);
	unsigned char offset = 0;

	if ((mask_len < 2 || mask[0] != '$'))
		return NULL;

	if (strchr(mask, ' '))
		return false;

	if (mask_len > 2 && mask[1] == '~')
		offset = 1;

	// e.g. $a and $~a
	if ((mask_len == 2 + offset) && strchr(without_param, mask[1 + offset]))
		return true;

	// e.g. $~a:Shutter and $~a:Shutter
	else if ((mask_len >= 3 + offset) && mask[2 + offset] == ':' && strchr(with_param, mask[1 + offset]))
		return true;

	return false;
}

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "protocol/ts6-generic")

	notice_channel_sts = &charybdis_notice_channel_sts;

	next_matching_ban = &charybdis_next_matching_ban;
	is_valid_host = &charybdis_is_valid_host;
	is_extban = &charybdis_is_extban;

	mode_list = charybdis_mode_list;
	ignore_mode_list = charybdis_ignore_mode_list;
	status_mode_list = charybdis_status_mode_list;
	prefix_mode_list = charybdis_prefix_mode_list;
	user_mode_list = charybdis_user_mode_list;
	ignore_mode_list_size = ARRAY_SIZE(charybdis_ignore_mode_list);

	ircd = &Charybdis;
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{

}

SIMPLE_DECLARE_MODULE_V1("protocol/charybdis", MODULE_UNLOAD_CAPABILITY_NEVER)
