/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2003-2004 E. Will, et al.
 * Copyright (C) 2005-2007 Atheme Project (http://atheme.org/)
 * Copyright (C) 2014-2017 ChatLounge IRC Network Development Team
 *
 * This file contains protocol support for ChatIRCd, a charybdis fork.
 * Adapted from the Charybdis protocol module.
 */

#include <atheme.h>
#include <atheme/protocol/chatircd.h>

static struct ircd ChatIRCd = {
	.ircdname = "ChatIRCd",                     // IRCd name
	.tldprefix = "$$",                          // TLD Prefix, used by Global.
	.uses_uid = true,                           // Whether or not we use IRCNet/TS6 UID
	.uses_rcommand = false,                     // Whether or not we use RCOMMAND
	.uses_owner = true,                         // Whether or not we support channel owners.
	.uses_protect = true,                       // Whether or not we support channel protection.
	.uses_halfops = true,                       // Whether or not we support halfops.
	.uses_p10 = false,                          // Whether or not we use P10
	.uses_vhost = false,                        // Whether or not we use vHosts.
	.oper_only_modes = CMODE_EXLIMIT | CMODE_PERM | CMODE_NETADMINONLY | CMODE_ADMINONLY | CMODE_OPERONLY,	// Oper-only cmodes
	.owner_mode = CSTATUS_OWNER,                // Integer flag for owner channel flag.
	.protect_mode = CSTATUS_PROTECT,            // Integer flag for protect channel flag.
	.halfops_mode = CSTATUS_HALFOP,             // Integer flag for halfops.
	.owner_mchar = "+y",                        // Mode we set for owner.
	.protect_mchar = "+a",                      // Mode we set for protect.
	.halfops_mchar = "+h",                      // Mode we set for halfops.
	.type = PROTOCOL_CHARYBDIS,                 // Protocol type
	.perm_mode = CMODE_PERM,                    // Permanent cmodes
	.oimmune_mode = 0,                          // Oper-immune cmode
	.ban_like_modes = "beIq",                   // Ban-like cmodes
	.except_mchar = 'e',                        // Except mchar
	.invex_mchar = 'I',                         // Invex mchar
	.flags = IRCD_CIDR_BANS | IRCD_HOLDNICK,    // Flags
};

static const struct cmode chatircd_mode_list[] = {
  { 'i', CMODE_INVITE	},
  { 'm', CMODE_MOD	},
  { 'n', CMODE_NOEXT	},
  { 'p', CMODE_PRIV	},
  { 's', CMODE_SEC	},
  { 't', CMODE_TOPIC	},
  { 'r', CMODE_REGONLY	},
  { 'z', CMODE_OPMOD	},
  { 'g', CMODE_FINVITE	},
  { 'L', CMODE_EXLIMIT	},
  { 'P', CMODE_PERM	},
  { 'F', CMODE_FTARGET	},
  { 'Q', CMODE_DISFWD	},
  { 'T', CMODE_NONOTICE	},

  // following modes are added as extensions
  { 'N', CMODE_NETADMINONLY	},
  { 'S', CMODE_SSLONLY	},
  { 'O', CMODE_OPERONLY	},
  { 'A', CMODE_ADMINONLY},
  { 'c', CMODE_NOCOLOR	},
  { 'C', CMODE_NOCTCP	},

  { '\0', 0 }
};

static const struct cmode chatircd_status_mode_list[] = {
  { 'y', CSTATUS_OWNER },
  { 'a', CSTATUS_PROTECT },
  { 'o', CSTATUS_OP    },
  { 'h', CSTATUS_HALFOP },
  { 'v', CSTATUS_VOICE },
  { '\0', 0 }
};

static const struct cmode chatircd_prefix_mode_list[] = {
  { '~', CSTATUS_OWNER },
  { '&', CSTATUS_PROTECT },
  { '@', CSTATUS_OP    },
  { '%', CSTATUS_HALFOP },
  { '+', CSTATUS_VOICE },
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

	bool check_realhost = (config_options.masks_through_vhost || u->host == u->vhost);

	snprintf(hostgbuf, sizeof hostgbuf, "%s!%s@%s#%s", u->nick, u->user, u->vhost, u->gecos);
	snprintf(realgbuf, sizeof realgbuf, "%s!%s@%s#%s", u->nick, u->user, u->host, u->gecos);
	return !match(mask, hostgbuf) || (check_realhost && !match(mask, realgbuf));
}

/* Check if the user is both *NOT* identified to services, and
 * matches the given hostmask.  Syntax: $u:n!u@h
 * e.g. +b $u:*!webchat@* would ban all webchat users who are not
 * identified to services.
 */
static bool
unidentified_match(const char *mask, struct user *u)
{
	char hostbuf[NICKLEN + 1 + USERLEN + 1 + HOSTLEN + 1];
	char realbuf[NICKLEN + 1 + USERLEN + 1 + HOSTLEN + 1];

	// Is identified, so just bail.
	if (u->myuser != NULL)
		return false;

	snprintf(hostbuf, sizeof hostbuf, "%s!%s@%s", u->nick, u->user, u->vhost);
	snprintf(realbuf, sizeof realbuf, "%s!%s@%s", u->nick, u->user, u->host);

	// If here, not identified to services so just check if the given hostmask matches.
	if (!match(mask, hostbuf) || !match(mask, realbuf))
		return true;

	return false;
}

static mowgli_node_t *
chatircd_next_matching_ban(struct channel *c, struct user *u, int type, mowgli_node_t *first)
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

	bool check_realhost = (config_options.masks_through_vhost || u->host == u->vhost);

	MOWGLI_ITER_FOREACH(n, first)
	{
		cb = n->data;

		if (cb->type != type)
			continue;

		/* strip any banforwards from the mask. (SRV-73)
		 * charybdis itself doesn't support banforward but i don't feel like copying
		 * this stuff into ircd-seven and it is possible that charybdis may support them
		 * one day.
		 *   --nenolod
		 */
		mowgli_strlcpy(strippedmask, cb->mask, sizeof strippedmask);
		p = strrchr(strippedmask, '$');
		if (p != NULL && p != strippedmask)
			*p = 0;

		if (!match(strippedmask, hostbuf))
			return n;
		if (check_realhost && (!match(strippedmask, realbuf) || !match(strippedmask, ipbuf) || !match_cidr(strippedmask, ipbuf)))
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
				case 'u':
					if (p == NULL)
						continue;
					matched = unidentified_match(p, u);
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
chatircd_is_extban(const char *mask)
{
	const char without_param[] = "oza";
	const char with_param[] = "ajcxru";
	const size_t mask_len = strlen(mask);
	unsigned int offset = 0;

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
	MODULE_TRY_REQUEST_DEPENDENCY(m, "protocol/charybdis")

	next_matching_ban = &chatircd_next_matching_ban;
	is_extban = &chatircd_is_extban;

	mode_list = chatircd_mode_list;
	status_mode_list = chatircd_status_mode_list;
	prefix_mode_list = chatircd_prefix_mode_list;

	ircd = &ChatIRCd;
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{

}

SIMPLE_DECLARE_MODULE_V1("protocol/chatircd", MODULE_UNLOAD_CAPABILITY_NEVER)
