/*
 * Copyright (C) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Data structures for flags to bitmask processing routines.
 *
 * $Id: confparse.h 8375 2007-06-03 20:03:26Z pippijn $
 */

#ifndef CONFPARSE_H
#define CONFPARSE_H

struct _configfile
{
	char *cf_filename;
	config_entry_t *cf_entries;
	config_file_t *cf_next;
};

struct _configentry
{
	config_file_t *ce_fileptr;

	int ce_varlinenum;
	char *ce_varname;
	char *ce_vardata;
	int ce_vardatanum;
	int ce_fileposstart;
	int ce_fileposend;

	int ce_sectlinenum;
	config_entry_t *ce_entries;

	config_entry_t *ce_prevlevel;

	config_entry_t *ce_next;
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
	int (*handler) (config_entry_t *);
	char *str_val;
	int *int_val;
};

extern void init_newconf(void);
extern struct ConfTable *find_top_conf(const char *name);
extern struct ConfTable *find_conf_item(const char *name, list_t *conflist);
extern void add_top_conf(const char *name, int (*handler)(config_entry_t *ce));
extern void add_conf_item(const char *name, list_t *conflist, int (*handler)(config_entry_t *ce));
extern void del_top_conf(const char *name);
extern void del_conf_item(const char *name, list_t *conflist);
extern int subblock_handler(config_entry_t *ce, list_t *entries);

extern int token_to_value(struct Token token_table[], const char *token);
/* special return values for token_to_value */
#define TOKEN_UNMATCHED -1
#define TOKEN_ERROR -2

#endif

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs ts=8 sw=8 noexpandtab
 */
