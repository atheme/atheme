/*
 * Copyright (c) 2005 William Pitcock <nenolod@nenolod.net>
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Dynamic linker.
 *
 * $Id: linker.c 3951 2005-11-20 05:10:01Z terminal $
 */

#include <org.atheme.claro.base>

#ifndef _WIN32
#include <dlfcn.h>
#endif

#ifdef __OpenBSD__
# define RTLD_NOW RTLD_LAZY
#endif

#if !defined(_WIN32) && !defined(__OS2__)
# ifndef __HPUX__
#  define PLATFORM_SUFFIX ".so"
# else
#  define PLATFORM_SUFFIX ".sl"
# endif
#else
# define PLATFORM_SUFFIX ".dll"
#endif

/* Simple hack to make dl*() work on Windows */
#ifdef _WIN32
#define dlopen(f,n) LoadLibrary(f)
#define dlsym GetProcAddress
#define dlclose FreeLibrary
#endif

/*
 * linker_open()
 *
 * Inputs:
 *       path to file to open
 *
 * Outputs:
 *       linker handle.
 *
 * Side Effects:
 *       a shared module is loaded into the application's memory space
 */
void *linker_open(char *path)
{
	return dlopen(path, RTLD_NOW);
}

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
void *linker_open_ext(char *path)
{
	char *buf = smalloc(strlen(path) + 20);
	
	strlcpy(buf, path, strlen(path) + 20);

	if (!strstr(buf, PLATFORM_SUFFIX))
		strlcat(buf, PLATFORM_SUFFIX, strlen(path) + 20);

	return linker_open(buf);
}

/*
 * linker_getsym()
 *
 * Inputs:
 *       linker handle, symbol to retrieve
 *
 * Outputs:
 *       pointer to symbol, or NULL if no symbol.
 *
 * Side Effects:
 *       none
 */
void *linker_getsym(void *vptr, char *sym)
{
	return dlsym(vptr, sym);
}

/*
 * linker_close()
 *
 * Inputs:
 *       linker handle
 *
 * Outputs:
 *       none
 *
 * Side Effects:
 *       the module is unloaded from the linker
 */
void linker_close(void *vptr)
{
	dlclose(vptr);
}

