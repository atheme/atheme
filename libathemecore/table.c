/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2014 Atheme Project (http://atheme.org/)
 * Copyright (C) 2018-2022 Atheme Development Group (https://atheme.github.io/)
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * atheme-services: A collection of minimalist IRC services
 * table.c: Table rendering class.
 */

#include <atheme.h>
#include "internal.h"

static mowgli_heap_t *table_heap = NULL;
static mowgli_heap_t *table_row_heap = NULL;
static mowgli_heap_t *table_column_heap = NULL;

static void
table_render_separator(void (*const callback)(const char *, void *), void *const restrict cbptr,
                       const struct atheme_table_row *const restrict header, const size_t rowwidth,
                       const size_t colcnt, const size_t colwidth[const restrict static colcnt])
{
	const mowgli_node_t *cn;
	char rowbuf[rowwidth];
	size_t colidx = 0;
	char *p = rowbuf;

	MOWGLI_ITER_FOREACH(cn, header->columns.head)
	{
		if (cn->next)
		{
			// If we're not the last column, we need to draw enough hyphens to cover its entire width
			for (size_t i = 0; i < colwidth[colidx]; i++)
				*p++ = '-';

			*p++ = ' ';
		}
		else
		{
			const struct atheme_table_column *const column = cn->data;

			// If we are the last column, we only need enough hyphens to cover the column's name
			for (size_t i = 0; i < strlen(column->name); i++)
				*p++ = '-';

			// Now that we've reached the last column, null-terminate the output
			*p = '\0';
		}

		// Advance to the next column index
		colidx++;
	}

	(void) (*callback)(rowbuf, cbptr);
}

static bool
table_render_body(void (*const callback)(const char *, void *), void *const restrict cbptr,
                  const struct atheme_table *const restrict table, const size_t rowwidth,
                  const size_t colcnt, const size_t colwidth[const restrict static colcnt])
{
	const struct atheme_table_row *const header = table->rows.head->data;
	const mowgli_node_t *rn;
	const mowgli_node_t *cn;
	char rowbuf[rowwidth];
	size_t colidx = 0;

	// mowgli_strlcat() below requires that this buffer is initialised
	rowbuf[0] = '\0';

	// Draw a blank line and then a row separator
	(void) (*callback)(" ", cbptr);
	(void) table_render_separator(callback, cbptr, header, rowwidth, colcnt, colwidth);

	// Draw the column names
	MOWGLI_ITER_FOREACH(cn, header->columns.head)
	{
		const struct atheme_table_column *const column = cn->data;

		// Add two extra right-padding characters to separate the column names
		const int padding = (int) (colwidth[colidx] + 2);

		// We need the extra padding characters (or the null terminator for the last column)
		char colbuf[colwidth[colidx] + 2];

		if (cn->next)
			// If we're not the last column, we need to pad the name to cover its entire width
			(void) snprintf(colbuf, sizeof colbuf, "%-*s", padding, column->name);
		else
			// If we are the last column, we don't need to pad its name
			(void) snprintf(colbuf, sizeof colbuf, "%s", column->name);

		(void) mowgli_strlcat(rowbuf, colbuf, sizeof rowbuf);

		// Advance to the next column index
		colidx++;
	}

	(void) (*callback)(rowbuf, cbptr);

	// Draw another row separator after the column names too
	(void) table_render_separator(callback, cbptr, header, rowwidth, colcnt, colwidth);

	// Output each row
	MOWGLI_ITER_FOREACH(rn, table->rows.head)
	{
		const struct atheme_table_row *const row = rn->data;

		// Reset our column index to 0 every time we start another row
		colidx = 0;

		// mowgli_strlcat() below requires that this buffer is initialised
		rowbuf[0] = '\0';

		MOWGLI_ITER_FOREACH(cn, row->columns.head)
		{
			const struct atheme_table_column *const column = cn->data;

			/* Add two extra right-padding characters to separate the
			 * column values, but only if we're not the last column.
			 */
			const int padding = (int) (cn->next ? (colwidth[colidx] + 2) : 0);

			// We need the extra padding characters (or the null terminator for the last column)
			char colbuf[colwidth[colidx] + 2];

			// Append the column value (if any), with appropriate padding
			(void) snprintf(colbuf, sizeof colbuf, "%-*s", padding, (column->value ? column->value : ""));
			(void) mowgli_strlcat(rowbuf, colbuf, sizeof rowbuf);

			// Advance to the next column index
			colidx++;
		}

		(void) (*callback)(rowbuf, cbptr);
	}

	// Draw another row separator at the end of the table and then another blank line
	(void) table_render_separator(callback, cbptr, header, rowwidth, colcnt, colwidth);
	(void) (*callback)(" ", cbptr);

	return true;
}

void
table_destroy(struct atheme_table *const restrict table)
{
	return_if_fail(table != NULL);

	// Iterate through every row of the table
	mowgli_node_t *rn;
	mowgli_node_t *rtn;
	MOWGLI_ITER_FOREACH_SAFE(rn, rtn, table->rows.head)
	{
		struct atheme_table_row *const row = rn->data;

		continue_if_fail(row != NULL);

		// Iterate through every column in this row
		mowgli_node_t *cn;
		mowgli_node_t *ctn;
		MOWGLI_ITER_FOREACH_SAFE(cn, ctn, row->columns.head)
		{
			struct atheme_table_column *const column = cn->data;

			continue_if_fail(column != NULL);

			// Free the memory allocated for this column
			(void) sfree(column->name);
			(void) sfree(column->value);
			(void) mowgli_heap_free(table_column_heap, column);
		}

		// Free the memory allocated for this row
		(void) mowgli_heap_free(table_row_heap, row);
	}

	// Free the memory allocated for this table
	(void) mowgli_heap_free(table_heap, table);
}

struct atheme_table * ATHEME_FATTR_MALLOC
table_create(void)
{
	if (! table_heap)
		table_heap = mowgli_heap_create(sizeof(struct atheme_table), 16, BH_NOW);

	return_val_if_fail(table_heap != NULL, NULL);

	struct atheme_table *const table = mowgli_heap_alloc(table_heap);

	return_val_if_fail(table != NULL, NULL);

	return table;
}

struct atheme_table_row * ATHEME_FATTR_MALLOC_UNCHECKED
table_add_row(struct atheme_table *const table)
{
	return_val_if_fail(table != NULL, NULL);

	if (! table_row_heap)
		table_row_heap = mowgli_heap_create(sizeof(struct atheme_table_row), 128, BH_NOW);

	return_val_if_fail(table_row_heap != NULL, NULL);

	struct atheme_table_row *const row = mowgli_heap_alloc(table_row_heap);

	return_val_if_fail(row != NULL, NULL);

	(void) mowgli_node_add(row, &row->node, &table->rows);

	row->table = table;

	return row;
}

struct atheme_table_column * ATHEME_FATTR_MALLOC_UNCHECKED
table_add_column(struct atheme_table_row *const row, const char *const name, const char *const value)
{
	return_val_if_fail(row != NULL, NULL);
	return_val_if_fail(row->table != NULL, NULL);
	return_val_if_fail(MOWGLI_LIST_LENGTH(&row->table->rows) != 0, NULL);
	return_val_if_fail(row->table->rows.head->data != NULL, NULL);
	return_val_if_fail(name != NULL, NULL);
	return_val_if_fail(strlen(name) != 0, NULL);

	if (! table_column_heap)
		table_column_heap = mowgli_heap_create(sizeof(struct atheme_table_column), 16, BH_NOW);

	return_val_if_fail(table_column_heap != NULL, NULL);

	struct atheme_table_column *const column = mowgli_heap_alloc(table_column_heap);

	return_val_if_fail(column != NULL, NULL);

	(void) mowgli_node_add(column, &column->node, &row->columns);

	// Only save the name if this is the first row; it's the only row that this code needs the name in
	if (row == row->table->rows.head->data)
		column->name = sstrdup(name);

	// This may be NULL
	column->value = sstrdup(value);

	return column;
}

bool
table_render(struct atheme_table *const table, void (*callback)(const char *, void *), void *const cbptr)
{
	return_val_if_fail(table != NULL, false);
	return_val_if_fail(callback != NULL, false);

	const mowgli_node_t *rn;
	const mowgli_node_t *cn;

	// We need at least one row
	const size_t rows = MOWGLI_LIST_LENGTH(&table->rows);
	return_val_if_fail(rows != 0, false);

	// Grab the header row
	const struct atheme_table_row *const header = table->rows.head->data;
	return_val_if_fail(header != NULL, false);

	// We need at least one column in the header row
	const size_t colcnt = MOWGLI_LIST_LENGTH(&header->columns);
	return_val_if_fail(colcnt != 0, false);

	// We need the same number of columns in all rows
	MOWGLI_ITER_FOREACH(rn, table->rows.head->next)
	{
		const struct atheme_table_row *const row = rn->data;

		return_val_if_fail(row != NULL, false);
		return_val_if_fail(MOWGLI_LIST_LENGTH(&row->columns) == colcnt, false);
	}

	// We need to calculate the maximum width of a row, and the maximum width of each column
	size_t rowwidth = 0;
	size_t colwidth[colcnt];
	for (size_t colidx = 0; colidx < colcnt; colidx++)
	{
		colwidth[colidx] = 0;
	}
	MOWGLI_ITER_FOREACH(rn, table->rows.head)
	{
		struct atheme_table_row *const row = rn->data;
		size_t colidx = 0;

		MOWGLI_ITER_FOREACH(cn, row->columns.head)
		{
			struct atheme_table_column *const column = cn->data;

			return_val_if_fail(column != NULL, false);

			if (row == header)
			{
				return_val_if_fail(column->name != NULL, false);

				/* Account for the fact that the column's name may be
				 * the widest piece of information in this column.
				 */
				const size_t nlen = strlen(column->name);
				if (nlen > colwidth[colidx])
					colwidth[colidx] = nlen;

				return_val_if_fail(nlen != 0, false);
			}

			if (column->value)
			{
				const size_t vlen = strlen(column->value);
				if (vlen > colwidth[colidx])
					colwidth[colidx] = vlen;
			}

			colidx++;
		}
	}
	for (size_t colidx = 0; colidx < colcnt; colidx++)
	{
		// Enforce a minimum column width of 4 characters to make reading easier
		if (colwidth[colidx] < 4)
			colwidth[colidx] = 4;

		// Account for the space character separating the columns
		rowwidth += (colwidth[colidx] + 1);
	}

	return table_render_body(callback, cbptr, table, rowwidth, colcnt, colwidth);
}
