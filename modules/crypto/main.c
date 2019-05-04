/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2019 Atheme Development Group (https://atheme.github.io/)
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 */

#include <atheme.h>

// Imported by other modules/crypto/foo.so
extern mowgli_list_t *crypto_conf_table;
mowgli_list_t *crypto_conf_table = NULL;

static void
mod_init(struct module *const restrict m)
{
	if (! (crypto_conf_table = mowgli_list_create()))
	{
		(void) slog(LG_ERROR, "%s: mowgli_list_create() failed (BUG?)", m->name);

		m->mflags |= MODFLAG_FAIL;
		return;
	}

	(void) add_subblock_top_conf("crypto", crypto_conf_table);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	(void) del_top_conf("crypto");

	(void) mowgli_list_free(crypto_conf_table);
}

SIMPLE_DECLARE_MODULE_V1("crypto/main", MODULE_UNLOAD_CAPABILITY_OK)
