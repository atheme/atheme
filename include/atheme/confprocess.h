/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005 William Pitcock, et al.
 *
 * Config reader.
 */

#ifndef ATHEME_INC_CONFPROCESS_H
#define ATHEME_INC_CONFPROCESS_H 1

#include <atheme/attributes.h>
#include <atheme/stdheaders.h>

#define CONF_NO_REHASH 0x1U

struct Token
{
	const char *    text;
	int             value;
};

struct ConfTable;

void init_confprocess(void);
struct ConfTable *find_top_conf(const char *name);
struct ConfTable *find_conf_item(const char *name, mowgli_list_t *conflist);
void add_top_conf(const char *name, int (*handler)(mowgli_config_file_entry_t *ce));
void add_subblock_top_conf(const char *name, mowgli_list_t *list);
void add_conf_item(const char *name, mowgli_list_t *conflist, int (*handler)(mowgli_config_file_entry_t *ce));
void add_uint_conf_item(const char *name, mowgli_list_t *conflist, unsigned int flags, unsigned int *var, unsigned int min, unsigned int max, unsigned int def);
void add_double_conf_item(const char *name, mowgli_list_t *conflist, unsigned int flags, double *var, double min, double max, double def);
void add_duration_conf_item(const char *name, mowgli_list_t *conflist, unsigned int flags, unsigned int *var, const char *defunit, unsigned int def);
void add_dupstr_conf_item(const char *name, mowgli_list_t *conflist, unsigned int flags, char **var, const char *def);
void add_bool_conf_item(const char *name, mowgli_list_t *conflist, unsigned int flags, bool *var, bool def);
void del_top_conf(const char *name);
void del_conf_item(const char *name, mowgli_list_t *conflist);
int subblock_handler(mowgli_config_file_entry_t *ce, mowgli_list_t *entries);

bool process_double_configentry(mowgli_config_file_entry_t *, double *, double, double);
bool process_duration_configentry(mowgli_config_file_entry_t *, unsigned int *, const char *);
bool process_uint_configentry(mowgli_config_file_entry_t *, unsigned int *, unsigned int, unsigned int);

void conf_report_warning(mowgli_config_file_entry_t *ce, const char *fmt, ...) ATHEME_FATTR_PRINTF (2, 3);
/* sort of a hack for servtree.c */
typedef int (*conf_handler_fn)(mowgli_config_file_entry_t *);
conf_handler_fn conftable_get_conf_handler(struct ConfTable *ct);

void conf_process(mowgli_config_file_t *cfp);

int token_to_value(struct Token token_table[], const char *token);
/* special return values for token_to_value */
#define TOKEN_UNMATCHED -1
#define TOKEN_ERROR -2

extern bool conf_need_rehash;

#endif /* !ATHEME_INC_CONFPROCESS_H */
