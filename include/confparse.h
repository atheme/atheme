/*
 * Copyright (C) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Data structures for flags to bitmask processing routines.
 *
 * $Id: confparse.h 2825 2005-10-10 21:53:24Z nenolod $
 */

#ifndef CONFPARSE_H
#define CONFPARSE_H

typedef struct _configfile CONFIGFILE;
typedef struct _configentry CONFIGENTRY;

struct _configfile
{
	char *cf_filename;
	CONFIGENTRY *cf_entries;
	CONFIGFILE *cf_next;
};

struct _configentry
{
	CONFIGFILE    *ce_fileptr;

	int ce_varlinenum;
	char *ce_varname;
	char *ce_vardata;
	int ce_vardatanum;
	int ce_fileposstart;
	int ce_fileposend;

	int ce_sectlinenum;
	CONFIGENTRY *ce_entries;

	CONFIGENTRY *ce_prevlevel;

	CONFIGENTRY *ce_next;
};

struct Token
{
	const char *text;
	int value;
};

struct ConfTable
{
        char *name;
        int rehashable;
        int (*handler) (CONFIGENTRY *);
	char *str_val;
	int *int_val;
};

extern void init_newconf(void);
extern struct ConfTable *find_top_conf(char *name);
extern struct ConfTable *find_conf_item(char *name, list_t *conflist);
extern void add_top_conf(char *name, int (*handler)(CONFIGENTRY *ce));
extern void add_conf_item(char *name, list_t *conflist, int (*handler)(CONFIGENTRY *ce));
extern void del_top_conf(char *name);
extern void del_conf_item(char *name, list_t *conflist);
extern int subblock_handler(CONFIGENTRY *ce, list_t *entries);

#endif
