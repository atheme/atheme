/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2014 Atheme Project (http://atheme.org/)
 * Copyright (C) 2018 Atheme Development Group (https://atheme.github.io/)
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

static void
table_destroy(void *obj)
{
	struct atheme_table *table = (struct atheme_table *) obj;
	mowgli_node_t *n, *tn;

	return_if_fail(table != NULL);

	MOWGLI_ITER_FOREACH_SAFE(n, tn, table->rows.head)
	{
		struct atheme_table_row *r = (struct atheme_table_row *) n->data;
		mowgli_node_t *n2, *tn2;

		return_if_fail(r != NULL);

		MOWGLI_ITER_FOREACH_SAFE(n2, tn2, r->cells.head)
		{
			struct atheme_table_cell *c = (struct atheme_table_cell *) n2->data;

			sfree(c->name);
			sfree(c->value);
			sfree(c);
			mowgli_node_delete(n2, &r->cells);
			mowgli_node_free(n2);
		}

		sfree(r);

		mowgli_node_delete(n, &table->rows);
		mowgli_node_free(n);
	}

	metadata_delete_all(table);

	sfree(table);
}

/*
 * table_new(const char *fmt, ...)
 *
 * Table constructor.
 *
 * Inputs:
 *     - printf-style string to name the table with.
 *
 * Outputs:
 *     - a table object.
 *
 * Side Effects:
 *     - none
 */
struct atheme_table * ATHEME_FATTR_MALLOC ATHEME_FATTR_PRINTF(1, 2)
table_new(const char *fmt, ...)
{
	va_list vl;
	char buf[BUFSIZE];

	return_val_if_fail(fmt != NULL, NULL);

	va_start(vl, fmt);
	if (vsnprintf(buf, sizeof buf, fmt, vl) < 0)
	{
		va_end(vl);
		return NULL;
	}
	va_end(vl);

	struct atheme_table *const out = smalloc(sizeof *out);
	atheme_object_init(&out->parent, buf, table_destroy);

	return out;
}

/*
 * table_row_new
 *
 * Creates a table row. This isn't an object.
 *
 * Inputs:
 *     - table to associate to.
 *
 * Outputs:
 *     - a table row
 *
 * Side Effects:
 *     - none
 */
struct atheme_table_row * ATHEME_FATTR_MALLOC
table_row_new(struct atheme_table *t)
{
	return_val_if_fail(t != NULL, NULL);

	struct atheme_table_row *const out = smalloc(sizeof *out);
	mowgli_node_add(out, mowgli_node_create(), &t->rows);

	return out;
}

/*
 * table_cell_associate
 *
 * Associates a cell with a row.
 *
 * Inputs:
 *     - row to add cell to.
 *
 * Outputs:
 *     - nothing
 *
 * Side Effects:
 *     - none
 */
void
table_cell_associate(struct atheme_table_row *r, const char *name, const char *value)
{
	return_if_fail(r != NULL);
	return_if_fail(name != NULL);
	return_if_fail(value != NULL);

	struct atheme_table_cell *const c = smalloc(sizeof *c);

	c->name = sstrdup(name);
	c->value = sstrdup(value);

	mowgli_node_add(c, mowgli_node_create(), &r->cells);
}

/*
 * table_render
 *
 * Renders a table. This is a two-pass operation, the first one to find out
 * the width of each cell, and then the second to render the data.
 *
 * Inputs:
 *     - a table
 *     - a callback function
 *     - opaque data
 *
 * Outputs:
 *     - none
 *
 * Side Effects:
 *     - a callback function is called for each row of the table.
 */
void
table_render(struct atheme_table *t, void (*callback)(const char *line, void *data), void *data)
{
	mowgli_node_t *n;
	struct atheme_table_row *f;
	size_t bufsz = 0;
	char *buf = NULL;
	char *p;
	int i;

	return_if_fail(t != NULL);
	return_if_fail(callback != NULL);

	f = (struct atheme_table_row *) t->rows.head->data;

	MOWGLI_ITER_FOREACH(n, t->rows.head)
	{
		struct atheme_table_row *r = (struct atheme_table_row *) n->data;
		mowgli_node_t *n2, *rn;

		/* we, uhh... we don't provide a macro for dealing with two lists at once ;) */
		for (n2 = r->cells.head, rn = f->cells.head;
		     n2 != NULL && rn != NULL; n2 = n2->next, rn = rn->next)
		{
			struct atheme_table_cell *c, *rc;
			size_t sz;

			c  = (struct atheme_table_cell *) n2->data;
			rc = (struct atheme_table_cell *) rn->data;

			if ((sz = strlen(c->name)) > (size_t)rc->width)
				rc->width = sz;

			if ((sz = strlen(c->value)) > (size_t)rc->width)
				rc->width = sz;
		}
	}

	/* now total up the result. */
	MOWGLI_ITER_FOREACH(n, f->cells.head)
	{
		struct atheme_table_cell *c = (struct atheme_table_cell *) n->data;
		bufsz += c->width + 1;
	}

	buf = smalloc(bufsz);

	/* start outputting the header. */
	callback("", data);
	MOWGLI_ITER_FOREACH(n, f->cells.head)
	{
		struct atheme_table_cell *c = (struct atheme_table_cell *) n->data;
		char buf2[1024];

		snprintf(buf2, 1024, "%-*s", n->next != NULL ? c->width + 1 : 0, c->name);
		mowgli_strlcat(buf, buf2, bufsz);
	}
	callback(buf, data);
	*buf = '\0';

	/* separator line */
	p = buf;
	MOWGLI_ITER_FOREACH(n, f->cells.head)
	{
		struct atheme_table_cell *c = (struct atheme_table_cell *) n->data;

		if (n->next != NULL)
		{
			for (i = 0; i < c->width; i++)
				*p++ = '-';
			*p++ = ' ';
		}
		else
			for (i = 0; i < (int)strlen(c->name); i++)
				*p++ = '-';
	}
	*p = '\0';
	callback(buf, data);
	*buf = '\0';

	MOWGLI_ITER_FOREACH(n, t->rows.head)
	{
		struct atheme_table_row *r = (struct atheme_table_row *) n->data;
		mowgli_node_t *n2, *rn;

		for (n2 = r->cells.head, rn = f->cells.head;
		     n2 != NULL && rn != NULL; n2 = n2->next, rn = rn->next)
		{
			struct atheme_table_cell *c, *rc;
			char buf2[1024];

			c  = (struct atheme_table_cell *) n2->data;
			rc = (struct atheme_table_cell *) rn->data;

			snprintf(buf2, 1024, "%-*s", n2->next != NULL ? rc->width + 1 : 0, c->value);
			mowgli_strlcat(buf, buf2, bufsz);
		}
		callback(buf, data);
		*buf = '\0';
	}

	/* separator line */
	p = buf;
	MOWGLI_ITER_FOREACH(n, f->cells.head)
	{
		struct atheme_table_cell *c = (struct atheme_table_cell *) n->data;

		if (n->next != NULL)
		{
			for (i = 0; i < c->width; i++)
				*p++ = '-';
			*p++ = ' ';
		}
		else
			for (i = 0; i < (int)strlen(c->name); i++)
				*p++ = '-';
	}
	*p = '\0';
	callback(buf, data);
	*buf = '\0';
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */

