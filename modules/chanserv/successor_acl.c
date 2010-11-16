/*
 * Copyright (c) 2010 William Pitcock <nenolod@atheme.org>.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Implements a named-successor ACL flag.
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/successor_acl", true, _modinit, NULL,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static unsigned int successor_flag = 0;

static void channel_pick_successor_hook(hook_channel_succession_req_t *req)
{
	return_if_fail(req != NULL);
	return_if_fail(req->mc != NULL);

	/* maybe some other module already chose a new successor. */
	if (req->mu != NULL)
		return;

	req->mu = mychan_pick_candidate(req->mc, successor_flag);
	if (req->mu == NULL)
		return;

	/* remove the successor flag from the ACL entry since we've picked a successor. */
	chanacs_change_simple(req->mc, entity(req->mu), NULL, 0, successor_flag);
}

void _modinit(module_t *m)
{
	m->mflags = MODTYPE_CORE;

	if ((successor_flag = flags_associate('S', 0, false, "successor")) == 0)
	{
		slog(LG_ERROR, "chanserv/successor_acl: Inserting +S into flagset failed.");
		exit(EXIT_FAILURE);
	}

	slog(LG_DEBUG, "chanserv/successor_acl: +S has been inserted with bitmask 0x%x", successor_flag);
	hook_add_first_channel_pick_successor(channel_pick_successor_hook);
}
