/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2012 William Pitcock <nenolod@dereferenced.org>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 */

#include <atheme.h>

static bool (*parent_command_authorize)(struct service *, struct sourceinfo *, struct command *, const char *) = NULL;

static bool
cmdperm_command_authorize(struct service *svs, struct sourceinfo *si, struct command *c, const char *userlevel)
{
	char permbuf[BUFSIZE], *cp;

	snprintf(permbuf, sizeof permbuf, "command:%s:%s", svs->internal_name, c->name);
	for (cp = permbuf; *cp != '\0'; cp++)
		*cp = ToLower(*cp);

	if (!has_priv(si, permbuf))
	{
		logaudit_denycmd(si, c, permbuf);
		return false;
	}

	return parent_command_authorize(svs, si, c, userlevel);
}

static void
mod_init(struct module ATHEME_VATTR_UNUSED *const restrict m)
{
	parent_command_authorize = command_authorize;
	command_authorize = cmdperm_command_authorize;
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	command_authorize = parent_command_authorize;
}

SIMPLE_DECLARE_MODULE_V1("security/cmdperm", MODULE_UNLOAD_CAPABILITY_OK)
