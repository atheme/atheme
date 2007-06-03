/*
 * Copyright (c) 2005 William Pitcock, et al.
 * The rights to this code are as documented in doc/LICENSE.
 *
 * This file contains data structures concerning modules.
 *
 * $Id: module.h 8375 2007-06-03 20:03:26Z pippijn $
 */

#ifndef MODULE_H
#define MODULE_H

#include "privs.h"

typedef struct module_ module_t;
typedef struct moduleheader_ moduleheader_t;

struct module_ {
	char name[BUFSIZE];
	char modpath[BUFSIZE];
	moduleheader_t *header;

	unsigned int mflags;

	void *address;
	void *handle;

	list_t dephost;
	list_t deplist;
};

#define MODTYPE_STANDARD	0
#define MODTYPE_CORE		1 /* Can't be unloaded. */
#define MODTYPE_FAIL		0x8000 /* modinit failed */

#define MAPI_ATHEME_MAGIC	0xdeadbeef
#define MAPI_ATHEME_V1		1

struct moduleheader_ {
	unsigned int atheme_mod;
	unsigned int abi_ver;
	char *name;
	boolean_t norestart;
	void (*modinit)(module_t *m);
	void (*deinit)(void);
	char *vendor;
	char *version;
};

#define DECLARE_MODULE_V1(name, norestart, modinit, deinit, ver, ven) \
	moduleheader_t _header = { \
		MAPI_ATHEME_MAGIC, MAPI_ATHEME_V1, \
		name, norestart, modinit, deinit, ven, ver \
	}

E void _modinit(module_t *m);
E void _moddeinit(void);

E void modules_init(void);
E module_t *module_load(char *filespec);
E void module_load_dir(char *dirspec);
E void module_load_dir_match(char *dirspec, char *pattern);
E void *module_locate_symbol(char *modname, char *sym);
E void module_unload(module_t *m);
E module_t *module_find(char *name);
E module_t *module_find_published(char *name);

/* Use this macro in your _modinit() function to use symbols from
 * other modules. It will abort the _modinit() and unload your module
 * without calling _moddeinit(). -- jilles */
/* XXX this assumes the parameter is called m */
#define MODULE_USE_SYMBOL(dest, modname, sym) \
	if ((dest = module_locate_symbol(modname, sym)) == NULL)	\
	{								\
		m->mflags = MODTYPE_FAIL;				\
		return;							\
	}

#endif

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
