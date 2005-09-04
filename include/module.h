/*
 * Copyright (c) 2005 William Pitcock, et al.
 * The rights to this code are as documented in doc/LICENSE.
 *
 * This file contains data structures concerning modules.
 *
 * $Id: module.h 2123 2005-09-04 23:25:05Z nenolod $
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
};

#define MODTYPE_STANDARD	0
#define MODTYPE_CORE		1 /* Can't be unloaded. */

#define MAPI_ATHEME_MAGIC	0xdeadbeef
#define MAPI_ATHEME_V1		1

struct moduleheader_ {
	int atheme_mod;
	int abi_ver;
	char *name;
	boolean_t norestart;
	void (*modinit)(module_t *m);
	void (*deinit)(void);
	char *vendor;
	char *version;
};

#define DECLARE_MODULE_V1(name, norestart, modinit, deinit, ven, ver) \
	moduleheader_t _header = { \
		MAPI_ATHEME_MAGIC, MAPI_ATHEME_V1, \
		name, norestart, modinit, deinit, ven, ver \
	}

extern void _modinit(module_t *m);
extern void _moddeinit(void);

extern void modules_init(void);
extern module_t *module_load(char *filespec);
extern void module_load_dir(char *dirspec);
extern void module_load_dir_match(char *dirspec, char *pattern);
extern void *module_locate_symbol(char *modname, char *sym);
extern void module_unload(module_t *m);
extern module_t *module_find(char *name);
extern module_t *module_find_published(char *name);

#endif
