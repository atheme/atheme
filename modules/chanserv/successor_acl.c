/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2010 William Pitcock <nenolod@dereferenced.org>
 *
 * Implements a named-successor ACL flag.
 */

#include <atheme.h>

static unsigned int successor_flag = 0;

static void
channel_pick_successor_hook(struct hook_channel_succession_req *req)
{
	return_if_fail(req != NULL);
	return_if_fail(req->mc != NULL);

	// maybe some other module already chose a new successor.
	if (req->mu != NULL)
		return;

	req->mu = mychan_pick_candidate(req->mc, successor_flag);
	if (req->mu == NULL)
		return;
}

static void
channel_succession_hook(struct hook_channel_succession_req *req)
{
	return_if_fail(req != NULL);
	return_if_fail(req->mc != NULL);
	return_if_fail(req->mu != NULL);

	// Remove the successor flag from the new founder.
	chanacs_change_simple(req->mc, entity(req->mu), NULL, 0, successor_flag, NULL);
}

static void
mod_init(struct module ATHEME_VATTR_UNUSED *const restrict m)
{
	if ((successor_flag = flags_associate('S', 0, false, "successor")) == 0)
	{
		slog(LG_ERROR, "chanserv/successor_acl: Inserting +S into flagset failed.");
		exit(EXIT_FAILURE);
	}

	slog(LG_DEBUG, "chanserv/successor_acl: +S has been inserted with bitmask 0x%x", successor_flag);
	hook_add_first_channel_pick_successor(channel_pick_successor_hook);
	hook_add_channel_succession(channel_succession_hook);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{

}

SIMPLE_DECLARE_MODULE_V1("chanserv/successor_acl", MODULE_UNLOAD_CAPABILITY_NEVER)
