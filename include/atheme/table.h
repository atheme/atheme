/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2007 William Pitcock <nenolod -at- sacredspiral.co.uk>
 * Copyright (C) 2022 Atheme Development Group (https://atheme.github.io/)
 *
 * Table rendering class.
 */

#ifndef ATHEME_INC_TABLE_H
#define ATHEME_INC_TABLE_H 1

#include <atheme/attributes.h>
#include <atheme/stdheaders.h>

struct atheme_table
{
	mowgli_list_t           rows;
};

struct atheme_table_row
{
	mowgli_node_t           node;
	mowgli_list_t           columns;
	struct atheme_table *   table;
};

struct atheme_table_column
{
	mowgli_node_t           node;
	char *                  name;   // NULL except in first row
	char *                  value;
};

struct atheme_table *table_create(void) ATHEME_FATTR_MALLOC;
struct atheme_table_row *table_add_row(struct atheme_table *) ATHEME_FATTR_MALLOC_UNCHECKED;
struct atheme_table_column *table_add_column(struct atheme_table_row *, const char *, const char *)
    ATHEME_FATTR_MALLOC_UNCHECKED;

bool table_render(struct atheme_table *, void (*)(const char *, void *), void *);
void table_destroy(struct atheme_table *);

#endif /* !ATHEME_INC_TABLE_H */
