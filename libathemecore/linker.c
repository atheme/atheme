/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2014 Atheme Project (http://atheme.org/)
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * atheme-services: A collection of minimalist IRC services
 * linker.c: Abstraction of the dynamic linking system.
 */

#include <atheme.h>
#include "internal.h"

#if defined(MOWGLI_OS_HPUX)
# define PLATFORM_SUFFIX ".sl"
#elif defined(MOWGLI_OS_WIN)
# define PLATFORM_SUFFIX ".dll"
#else
# define PLATFORM_SUFFIX ".so"
#endif

/*
 * linker_open_ext()
 *
 * Inputs:
 *       path to file to open
 *
 * Outputs:
 *       linker handle
 *
 * Side Effects:
 *       the extension is appended if it's not already there.
 *       a shared module is loaded into the application's memory space
 */
mowgli_module_t *
linker_open_ext(const char *path, char *errbuf, size_t errlen)
{
	size_t len = strlen(path) + 20;
	char *buf = smalloc(len);
	void *ret;

	mowgli_strlcpy(buf, path, len);

	if (!strstr(buf, PLATFORM_SUFFIX))
		mowgli_strlcat(buf, PLATFORM_SUFFIX, len);

	/* Don't try to open a file that doesn't exist. */
	struct stat s;
	if (0 != stat(buf, &s))
	{
		mowgli_strlcpy(errbuf, strerror(errno), errlen);
		sfree(buf);
		return NULL;
	}

	ret = mowgli_module_open(buf);
	sfree(buf);

	if (!ret)
		mowgli_strlcpy(errbuf, "mowgli_module_open() failed", errlen);

	return ret;
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
