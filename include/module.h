/*
 * Copyright (c) 2005 William Pitcock, et al.
 * The rights to this code are as documented in doc/LICENSE.
 *
 * This file contains data structures concerning modules.
 *
 * $Id: module.h 4217 2005-12-27 03:36:36Z nenolod $
 */

#ifndef MODULE_H
#define MODULE_H

typedef struct module_ module_t;
typedef struct moduleheader_ moduleheader_t;

struct module_ {
	char name[BUFSIZE];
	char modpath[BUFSIZE];
	moduleheader_t *header;

	uint16_t mflags;

	void *address;
	void *handle;

	list_t dephost;
	list_t deplist;
};

#define MODTYPE_STANDARD	0
#define MODTYPE_CORE		1 /* Can't be unloaded. */

#define MAPI_ATHEME_MAGIC	0xdeadbeef
#define MAPI_ATHEME_V1		1

struct moduleheader_ {
	uint32_t atheme_mod;
	uint32_t abi_ver;
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

#endif
