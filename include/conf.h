/*
 * Copyright (C) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Config reader.
 */

#ifndef CONF_H
#define CONF_H

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

/* conf.c */
E bool conf_parse(const char *);
E void conf_init(void);
E bool conf_rehash(void);
E bool conf_check(void);

E void init_newconf(void);
E struct ConfTable *find_top_conf(const char *name);
E struct ConfTable *find_conf_item(const char *name, list_t *conflist);
E void add_top_conf(const char *name, int (*handler)(config_entry_t *ce));
E void add_conf_item(const char *name, list_t *conflist, int (*handler)(config_entry_t *ce));
E void del_top_conf(const char *name);
E void del_conf_item(const char *name, list_t *conflist);
E int subblock_handler(config_entry_t *ce, list_t *entries);

E int token_to_value(struct Token token_table[], const char *token);
/* special return values for token_to_value */
#define TOKEN_UNMATCHED -1
#define TOKEN_ERROR -2

/* XXX */
E list_t conf_ci_table;
E list_t conf_ni_table;

#endif

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs ts=8 sw=8 noexpandtab
 */
