/*
 * atheme-services: A collection of minimalist IRC services
 * table.c: Table rendering class.
 *
 * NOTE: This is a work in progress and will probably change considerably
 * later on.
 *
 * Copyright (c) 2005-2007 Atheme Project (http://www.atheme.org)
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "atheme.h"

static void table_destroy(void *obj)
{
	table_t *table = (table_t *) obj;
	mowgli_node_t *n, *tn;

	return_if_fail(table != NULL);

	MOWGLI_ITER_FOREACH_SAFE(n, tn, table->rows.head)
	{
		table_row_t *r = (table_row_t *) n->data;
		mowgli_node_t *n2, *tn2;

		return_if_fail(r != NULL);

		MOWGLI_ITER_FOREACH_SAFE(n2, tn2, r->cells.head)
		{
			table_cell_t *c = (table_cell_t *) n2->data;

			free(c->name);
			free(c->value);
			free(c);
			mowgli_node_delete(n2, &r->cells);
			mowgli_node_free(n2);
		}

		free(r);

		mowgli_node_delete(n, &table->rows);
		mowgli_node_free(n);
	}

	metadata_delete_all(table);

	free(table);
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
table_t *table_new(const char *fmt, ...)
{
	va_list vl;
	char *buf;
	table_t *out;

	return_val_if_fail(fmt != NULL, NULL);

	va_start(vl, fmt);
	if (vasprintf(&buf, fmt, vl) < 0)
	{
		va_end(vl);
		return NULL;
	}
	va_end(vl);

	out = scalloc(sizeof(table_t), 1);
	object_init(&out->parent, buf, table_destroy);
	free(buf);

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
table_row_t *table_row_new(table_t *t)
{
	table_row_t *out;

	return_val_if_fail(t != NULL, NULL);

	out = scalloc(sizeof(table_row_t), 1);

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
void table_cell_associate(table_row_t *r, const char *name, const char *value)
{
	table_cell_t *c;

	return_if_fail(r != NULL);
	return_if_fail(name != NULL);
	return_if_fail(value != NULL);

	c = scalloc(sizeof(table_cell_t), 1);

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
void table_render(table_t *t, void (*callback)(const char *line, void *data), void *data)
{
	mowgli_node_t *n;
	table_row_t *f;
	size_t bufsz = 0;
	char *buf = NULL;
	char *p;
	int i;

	return_if_fail(t != NULL);
	return_if_fail(callback != NULL);

	f = (table_row_t *) t->rows.head->data;

	MOWGLI_ITER_FOREACH(n, t->rows.head)
	{
		table_row_t *r = (table_row_t *) n->data;
		mowgli_node_t *n2, *rn;

		/* we, uhh... we don't provide a macro for dealing with two lists at once ;) */
		for (n2 = r->cells.head, rn = f->cells.head;
		     n2 != NULL && rn != NULL; n2 = n2->next, rn = rn->next)
		{
			table_cell_t *c, *rc;
			size_t sz;

			c  = (table_cell_t *) n2->data;
			rc = (table_cell_t *) rn->data;

			if ((sz = strlen(c->name)) > (size_t)rc->width)
				rc->width = sz;

			if ((sz = strlen(c->value)) > (size_t)rc->width)
				rc->width = sz;		
		}
	}

	/* now total up the result. */
	MOWGLI_ITER_FOREACH(n, f->cells.head)
	{
		table_cell_t *c = (table_cell_t *) n->data;
		bufsz += c->width + 1;
	}

	buf = smalloc(bufsz);
	*buf = '\0';

	/* start outputting the header. */
	callback(object(t)->name, data);
	MOWGLI_ITER_FOREACH(n, f->cells.head)
	{
		table_cell_t *c = (table_cell_t *) n->data;
		char buf2[1024];

		snprintf(buf2, 1024, "%-*s", n->next != NULL ? c->width + 1 : 0, c->name);
		strlcat(buf, buf2, bufsz);
	}
	callback(buf, data);
	*buf = '\0';

	/* separator line */
	p = buf;
	MOWGLI_ITER_FOREACH(n, f->cells.head)
	{
		table_cell_t *c = (table_cell_t *) n->data;

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
		table_row_t *r = (table_row_t *) n->data;
		mowgli_node_t *n2, *rn;

		for (n2 = r->cells.head, rn = f->cells.head;
		     n2 != NULL && rn != NULL; n2 = n2->next, rn = rn->next)
		{
			table_cell_t *c, *rc;
			char buf2[1024];

			c  = (table_cell_t *) n2->data;
			rc = (table_cell_t *) rn->data;

			snprintf(buf2, 1024, "%-*s", n2->next != NULL ? rc->width + 1 : 0, c->value);
			strlcat(buf, buf2, bufsz);
		}
		callback(buf, data);
		*buf = '\0';
	}

	/* separator line */
	p = buf;
	MOWGLI_ITER_FOREACH(n, f->cells.head)
	{
		table_cell_t *c = (table_cell_t *) n->data;

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

