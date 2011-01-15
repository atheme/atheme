/*
 * Copyright (c) 2005 William Pitcock, et al.
 * The rights to this code are as documented in doc/LICENSE.
 *
 * This file contains data structures concerning modules.
 *
 */

#ifndef MODULE_H
#define MODULE_H

#include "privs.h"
#include "abirev.h"

typedef struct module_ module_t;
typedef struct v4_moduleheader_ v4_moduleheader_t;

struct module_ {
	char name[BUFSIZE];
	char modpath[BUFSIZE];
	v4_moduleheader_t *header;

	unsigned int mflags;

	void *address;
	mowgli_module_t *handle;

	mowgli_list_t dephost;
	mowgli_list_t deplist;

	mowgli_list_t symlist;		/* MAPIv2 symbol dependencies. */
};

#define MODTYPE_STANDARD	0
#define MODTYPE_CORE		1 /* Can't be unloaded. */
#define MODTYPE_FAIL		0x8000 /* modinit failed */

#define MAPI_ATHEME_MAGIC	0xdeadbeef
#define MAPI_ATHEME_V4		4

#define MAX_CMD_PARC		20

typedef enum {
	MODULE_UNLOAD_INTENT_PERM,
	MODULE_UNLOAD_INTENT_RELOAD,
} module_unload_intent_t;

typedef enum {
	MODULE_UNLOAD_CAPABILITY_OK,
	MODULE_UNLOAD_CAPABILITY_NEVER,
	MODULE_UNLOAD_CAPABILITY_RELOAD_ONLY,
} module_unload_capability_t;

struct v4_moduleheader_ {
	unsigned int atheme_mod;
	unsigned int abi_ver;
	unsigned int abi_rev;
	const char *serial;
	const char *name;
	module_unload_capability_t can_unload;
	void (*modinit)(module_t *m);
	void (*deinit)(module_unload_intent_t intent);
	const char *vendor;
	const char *version;
};

#define DECLARE_MODULE_V1(name, norestart, modinit, deinit, ver, ven) \
	v4_moduleheader_t _header = { \
		MAPI_ATHEME_MAGIC, MAPI_ATHEME_V4, \
		CURRENT_ABI_REVISION, "unknown", \
		name, norestart, modinit, deinit, ven, ver \
	}

E void _modinit(module_t *m);
E void _moddeinit(module_unload_intent_t intent);

E void modules_init(void);
E module_t *module_load(const char *filespec);
E void module_load_dir(const char *dirspec);
E void module_load_dir_match(const char *dirspec, const char *pattern);
E void *module_locate_symbol(const char *modname, const char *sym);
E void module_unload(module_t *m, module_unload_intent_t intent);
E module_t *module_find(const char *name);
E module_t *module_find_published(const char *name);
E bool module_request(const char *name);

#define MODULE_TRY_REQUEST_DEPENDENCY(self, modname) \
	if (module_request(modname) == false)				\
	{								\
		(self)->mflags = MODTYPE_FAIL;				\
		return;							\
	}

#define MODULE_TRY_REQUEST_SYMBOL(self, dest, modname, sym) \
	if ((dest = module_locate_symbol(modname, sym)) == NULL)		\
	{									\
		MODULE_TRY_REQUEST_DEPENDENCY(self, modname);			\
		if ((dest = module_locate_symbol(modname, sym)) == NULL)	\
		{								\
			(self)->mflags = MODTYPE_FAIL;				\
			return;							\
		}								\
	}

#endif

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
