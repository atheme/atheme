/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2013 Atheme Project (http://atheme.org/)
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * atheme-services: A collection of minimalist IRC services
 * uid.c: UID management.
 */

#include <atheme.h>
#include "internal.h"

const struct uid_provider *uid_provider_impl = NULL;

void
init_uid(void)
{
	if (uid_provider_impl != NULL)
		return uid_provider_impl->uid_init(me.numeric);
}

const char *
uid_get(void)
{
	if (uid_provider_impl != NULL)
		return uid_provider_impl->uid_get();

	return NULL;
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
