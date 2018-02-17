/*
 * Copyright (C) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Config reader.
 */

#ifndef CONFPROCESS_H
#define CONFPROCESS_H

#define CONF_NO_REHASH 0x1

struct Token
{
	const char *text;
	int value;
};

struct ConfTable;

extern void init_confprocess(void);
extern struct ConfTable *find_top_conf(const char *name);
extern struct ConfTable *find_conf_item(const char *name, mowgli_list_t *conflist);
extern void add_top_conf(const char *name, int (*handler)(mowgli_config_file_entry_t *ce));
extern void add_subblock_top_conf(const char *name, mowgli_list_t *list);
extern void add_conf_item(const char *name, mowgli_list_t *conflist, int (*handler)(mowgli_config_file_entry_t *ce));
extern void add_uint_conf_item(const char *name, mowgli_list_t *conflist, unsigned int flags, unsigned int *var, unsigned int min, unsigned int max, unsigned int def);
extern void add_duration_conf_item(const char *name, mowgli_list_t *conflist, unsigned int flags, unsigned int *var, const char *defunit, unsigned int def);
extern void add_dupstr_conf_item(const char *name, mowgli_list_t *conflist, unsigned int flags, char **var, const char *def);
extern void add_bool_conf_item(const char *name, mowgli_list_t *conflist, unsigned int flags, bool *var, bool def);
extern void del_top_conf(const char *name);
extern void del_conf_item(const char *name, mowgli_list_t *conflist);
extern int subblock_handler(mowgli_config_file_entry_t *ce, mowgli_list_t *entries);
extern bool process_uint_configentry(mowgli_config_file_entry_t *ce, unsigned int *var,
		unsigned int min, unsigned int max);
extern bool process_duration_configentry(mowgli_config_file_entry_t *ce, unsigned int *var,
		const char *defunit);
extern void conf_report_warning(mowgli_config_file_entry_t *ce, const char *fmt, ...) ATHEME_FATTR_PRINTF (2, 3);
/* sort of a hack for servtree.c */
typedef int (*conf_handler_t)(mowgli_config_file_entry_t *);
extern conf_handler_t conftable_get_conf_handler(struct ConfTable *ct);

extern void conf_process(mowgli_config_file_t *cfp);

extern int token_to_value(struct Token token_table[], const char *token);
/* special return values for token_to_value */
#define TOKEN_UNMATCHED -1
#define TOKEN_ERROR -2

extern bool conf_need_rehash;

#endif
