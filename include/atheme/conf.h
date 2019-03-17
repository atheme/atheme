/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005 William Pitcock, et al.
 *
 * Config reader.
 */

#ifndef ATHEME_INC_CONF_H
#define ATHEME_INC_CONF_H 1

#include <atheme/stdheaders.h>

bool conf_parse(const char *);
void conf_init(void);
bool conf_rehash(void);
bool conf_check(void);

void init_newconf(void);

/* XXX Unstable module api to add things to the standard conf blocks */
extern mowgli_list_t conf_si_table; /* serverinfo{} */
extern mowgli_list_t conf_gi_table; /* general{} */
extern mowgli_list_t conf_la_table; /* language{} */

#endif /* !ATHEME_INC_CONF_H */
