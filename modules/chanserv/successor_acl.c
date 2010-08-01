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

void _modinit(module_t *m)
{
	unsigned int flag;
	m->mflags = MODTYPE_CORE;

	if ((flag = flags_associate('S', 0, false)) == 0)
	{
		slog(LG_ERROR, "chanserv/successor_acl: Inserting +S into flagset failed.");
		exit(EXIT_FAILURE);
	}

	slog(LG_INFO, "chanserv/successor_acl: +S has been inserted with bitmask 0x%x", flag);
}
