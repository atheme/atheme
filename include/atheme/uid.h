/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2011 William Pitcock <nenolod@dereferenced.org>
 *
 * UID provider stuff.
 */

#ifndef ATHEME_INC_UID_H
#define ATHEME_INC_UID_H 1

#include <atheme/stdheaders.h>

struct uid_provider
{
	void          (*uid_init)(const char *sid);
	const char *  (*uid_get)(void);
};

extern const struct uid_provider *uid_provider_impl;

#endif /* !ATHEME_INC_UID_H */
