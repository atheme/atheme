/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2010 Atheme Project (http://atheme.org/)
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * atheme-services: A collection of minimalist IRC services
 * main.c: RFC1459 transport core.
 */

#include <atheme.h>
#include "rfc1459.h"

static void
mod_init(struct module ATHEME_VATTR_UNUSED *const restrict m)
{
	parse = &irc_parse;
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{

}

SIMPLE_DECLARE_MODULE_V1("transport/rfc1459", MODULE_UNLOAD_CAPABILITY_NEVER)
