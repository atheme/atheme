/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2007 Robin Burchell, et al.
 * Copyright (C) 2010 William Pitcock <nenolod@atheme.org>
 *
 * This file contains code for the NickServ LIST function.
 * Based on Alex Lambert's LISTEMAIL.
 */

#include <atheme.h>
#include "list_common.h"

// Imported by many other modules
extern void list_register(const char *, struct list_param *);
extern void list_unregister(const char *);

static mowgli_patricia_t *list_params;

static bool
email_match(const struct mynick *mn, const void *arg)
{
	struct myuser *mu = mn->owner;
	const char *cmpr = (const char*)arg;

	return !match(cmpr, mu->email);
}

static bool
lastlogin_match(const struct mynick *mn, const void *arg)
{
	struct myuser *mu = mn->owner;
	const time_t lastlogin = *((const time_t *) arg);

	return (CURRTIME - mu->lastlogin) > lastlogin;
}

static bool
pattern_match(const struct mynick *mn, const void *arg)
{
	const char *pattern = (const char*)arg;

	char pat[512], *nickpattern = NULL, *hostpattern = NULL, *p;
	struct metadata *md;

	bool hostmatch;

	struct myuser *mu = mn->owner;

	if (pattern != NULL)
	{
		mowgli_strlcpy(pat, pattern, sizeof pat);
		p = strrchr(pat, ' ');
		if (p == NULL)
			p = strrchr(pat, '!');
		if (p != NULL)
		{
			*p++ = '\0';
			nickpattern = pat;
			hostpattern = p;
		}
		else if (strchr(pat, '@'))
			hostpattern = pat;
		else
			nickpattern = pat;
		if (nickpattern && !strcmp(nickpattern, "*"))
			nickpattern = NULL;
	}

	if (nickpattern && match(nickpattern, mn->nick))
		return false;

	if (hostpattern)
	{
		hostmatch = false;
		md = metadata_find(mu, "private:host:actual");
		if (md != NULL && !match(hostpattern, md->value))
			hostmatch = true;
		md = metadata_find(mu, "private:host:vhost");
		if (md != NULL && !match(hostpattern, md->value))
			hostmatch = true;
		if (!hostmatch)
			return false;
	}

	return true;
}

static bool
registered_match(const struct mynick *mn, const void *arg)
{
	struct myuser *mu = mn->owner;
	const time_t age = *((const time_t *) arg);

	return (CURRTIME - mu->registered) > age;
}

static bool
primary_match(const struct mynick *mn, const void *arg)
{
	struct myuser *mu = mn->owner;
	(void) arg;

	return !irccasecmp(mn->nick, entity(mn->owner)->name);
}

static bool
has_waitauth(const struct mynick *mn, const void *arg)
{
	struct myuser *mu = mn->owner;

	return ( mu->flags & MU_WAITAUTH ) == MU_WAITAUTH;
}

void
list_register(const char *param_name, struct list_param *param)
{
	mowgli_patricia_add(list_params, param_name, param);
}

void
list_unregister(const char *param_name)
{
	mowgli_patricia_delete(list_params, param_name);
}

static time_t
parse_age(char *s)
{
	time_t duration;

	duration = (atol(s) * SECONDS_PER_MINUTE);
	while (isdigit((unsigned char)*s))
		s++;

	if (*s == 'h' || *s == 'H')
		duration *= MINUTES_PER_HOUR;
	else if (*s == 'd' || *s == 'D')
		duration *= MINUTES_PER_DAY;
	else if (*s == 'w' || *s == 'W')
		duration *= MINUTES_PER_WEEK;
	else if (*s == '\0')
		;
	else
		duration = 0;

	return duration;
}

static void
build_criteriastr(char *buf, int parc, char *parv[])
{
	int i;

	return_if_fail(buf != NULL);

	*buf = 0;
	for (i = 0; i < parc; i++)
	{
		mowgli_strlcat(buf, parv[i], BUFSIZE);
		mowgli_strlcat(buf, " ", BUFSIZE);
	}
}

static void
list_one(struct sourceinfo *si, struct myuser *mu, struct mynick *mn)
{
	char buf[BUFSIZE];

	if (mn != NULL)
		mu = mn->owner;

	*buf = '\0';
	if (metadata_find(mu, "private:freeze:freezer")) {
		if (*buf)
			mowgli_strlcat(buf, " ", BUFSIZE);

		mowgli_strlcat(buf, "\2[frozen]\2", BUFSIZE);
	}
	if (metadata_find(mu, "private:restrict:setter")) {
		if (*buf)
			mowgli_strlcat(buf, " ", BUFSIZE);

		mowgli_strlcat(buf, "\2[restricted]\2", BUFSIZE);
	}
	if (mu->flags & MU_HOLD) {
		if (*buf)
			mowgli_strlcat(buf, " ", BUFSIZE);

		mowgli_strlcat(buf, "\2[held]\2", BUFSIZE);
	}
	if (mu->flags & MU_WAITAUTH) {
		if (*buf)
			mowgli_strlcat(buf, " ", BUFSIZE);

		mowgli_strlcat(buf, "\2[unverified]\2", BUFSIZE);
	}

	if (mn == NULL || !irccasecmp(mn->nick, entity(mu)->name))
		command_success_nodata(si, "- %s (%s) %s", entity(mu)->name, mu->email, buf);
	else
		command_success_nodata(si, "- %s (%s) (%s) %s", mn->nick, mu->email, entity(mu)->name, buf);
}

static void
ns_cmd_list(struct sourceinfo *si, int parc, char *parv[])
{
	char criteriastr[BUFSIZE];

	mowgli_patricia_iteration_state_t state;
	struct myentity_iteration_state mestate;
	struct mynick *mn;

	unsigned int matches = 0;

	int i;
	bool error = false;
	bool found;

	MOWGLI_PATRICIA_FOREACH(mn, &state, nicklist)
	{
		found = true;

		for (i = 0; i < parc; i++)
		{
			struct list_param *param = mowgli_patricia_retrieve(list_params, parv[i]);

			if (param == NULL) {
				command_fail(si, fault_badparams, _("\2%s\2 is not a recognized LIST criterion"), parv[i]);
				error = true;
				break;
			}

			if (param->opttype == OPT_BOOL) {
				bool arg = true;

				if (!param->is_match(mn, &arg)) {
					found = false;
					break;
				}
			} else if (param->opttype == OPT_INT) {
				if (i + 1 < parc) {
					int arg = atoi(parv[++i]);

					if (!param->is_match (mn, &arg)) {
						found = false;
						break;
					}
				} else {
					command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, parv[i]);
					error = true;
					break;
				}
			} else if (param->opttype == OPT_STRING) {
				if (i + 1 < parc) {
					if (!param->is_match (mn, parv[++i])) {
						found = false;
						break;
					}
				} else {
					command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, parv[i]);
					error = true;
					break;
				}
			}
			else if (param->opttype == OPT_AGE) {
				if (i + 1 < parc)
				{
					time_t age = parse_age(parv[++i]);

					if (!param->is_match (mn, &age)) {
						found = false;
						break;
					}
				} else {
					command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, parv[i]);
					error = true;
					break;
				}
			}
		} //for (i = 0; i < parc; i++)

		if (error) {
			break;
		}

		if (found) {
			list_one(si, NULL, mn);
			matches++;
		}
	} //MOWGLI_PATRICIA_FOREACH(mn, &state, nicklist)

	build_criteriastr(criteriastr, parc, parv);

	logcommand(si, CMDLOG_ADMIN, "LIST: \2%s\2 (\2%u\2 matches)", criteriastr, matches);
	if (matches == 0)
		command_success_nodata(si, _("No nicknames matched criteria \2%s\2"), criteriastr);
	else
		command_success_nodata(si, ngettext(N_("\2%u\2 match for criteria \2%s\2."),
		                                    N_("\2%u\2 matches for criteria \2%s\2."), matches),
		                                    matches, criteriastr);
}

static struct command ns_list = {
	.name           = "LIST",
	.desc           = N_("Lists nicknames registered matching a given pattern."),
	.access         = PRIV_USER_AUSPEX,
	.maxparc        = 10,
	.cmd            = &ns_cmd_list,
	.help           = { .path = "nickserv/list" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "nickserv/main")

	list_params = mowgli_patricia_create(strcasecanon);
	service_named_bind_command("nickserv", &ns_list);

	// list email
	static struct list_param email;
	email.opttype = OPT_STRING;
	email.is_match = email_match;

	static struct list_param lastlogin;
	lastlogin.opttype = OPT_AGE;
	lastlogin.is_match = lastlogin_match;

	static struct list_param pattern;
	pattern.opttype = OPT_STRING;
	pattern.is_match = pattern_match;

	static struct list_param registered;
	registered.opttype = OPT_AGE;
	registered.is_match = registered_match;

	static struct list_param primary;
	primary.opttype = OPT_BOOL;
	primary.is_match = primary_match;

	list_register("email", &email);
	list_register("lastlogin", &lastlogin);
	list_register("mail", &email);

	list_register("pattern", &pattern);
	list_register("registered", &registered);
	list_register("primary", &primary);

	static struct list_param waitauth;
	waitauth.opttype = OPT_BOOL;
	waitauth.is_match = has_waitauth;

	list_register("waitauth", &waitauth);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("nickserv", &ns_list);

	list_unregister("email");
	list_unregister("lastlogin");
	list_unregister("mail");

	list_unregister("pattern");
	list_unregister("registered");

	list_unregister("waitauth");
}

SIMPLE_DECLARE_MODULE_V1("nickserv/list", MODULE_UNLOAD_CAPABILITY_OK)
