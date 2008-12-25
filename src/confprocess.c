/*
 * atheme-services: A collection of minimalist IRC services   
 * confprocess.c: Generic configuration processing.
 *
 * Copyright (c) 2005-2008 Atheme Project (http://www.atheme.org)           
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

enum conftype
{
	CONF_HANDLER,
	CONF_UINT,
	CONF_DUPSTR,
	CONF_BOOL
};

struct ConfTable
{
	char *name;
	enum conftype type;
	union
	{
		int (*handler) (config_entry_t *);
		struct
		{
			unsigned int *var;
			unsigned int min;
			unsigned int max;
		} uint_val;
		char **dupstr_val;
		bool *bool_val;
	} un;
};

static inline int
PARAM_ERROR(config_entry_t *ce)
{
	slog(LG_INFO, "%s:%i: no parameter for configuration option: %s",
	     ce->ce_fileptr->cf_filename,
	     ce->ce_varlinenum,
	     ce->ce_varname);
	return 1;
}

static BlockHeap *conftable_heap;

list_t confblocks;

/* *INDENT-ON* */

static void conf_report_error(config_entry_t *ce, const char *fmt, ...)
{
	va_list va;
	char buf[BUFSIZE];

	return_if_fail(ce != NULL);
	return_if_fail(fmt != NULL);

	va_start(va, fmt);
	vsnprintf(buf, BUFSIZE, fmt, va);
	va_end(va);

	slog(LG_INFO, "%s:%d: configuration error - %s", ce->ce_fileptr->cf_filename, ce->ce_varlinenum, buf);
}

bool process_uint_configentry(config_entry_t *ce, unsigned int *var,
		unsigned int min, unsigned int max)
{
	unsigned long v;
	char *end;

	if (ce->ce_vardata == NULL)
	{
		PARAM_ERROR(ce);
		return false;
	}
	errno = 0;
	v = strtoul(ce->ce_vardata, &end, 10);
	if (errno != 0 || *end != '\0' || end == ce->ce_vardata)
	{
		slog(LG_INFO, "%s:%i: invalid number \"%s\" for configuration option: %s",
				ce->ce_fileptr->cf_filename,
				ce->ce_varlinenum,
				ce->ce_vardata,
				ce->ce_varname);
		return false;
	}
	if (v > max || v < min)
	{
		slog(LG_INFO, "%s:%i: value %lu is out of range [%u,%u] for configuration option: %s",
				ce->ce_fileptr->cf_filename,
				ce->ce_varlinenum,
				v,
				min,
				max,
				ce->ce_varname);
		return false;
	}
	*var = v;
	return true;
}

static void process_configentry(struct ConfTable *ct, config_entry_t *ce)
{
	switch (ct->type)
	{
		case CONF_HANDLER:
			ct->un.handler(ce);
			break;
		case CONF_UINT:
			if (!process_uint_configentry(ce, ct->un.uint_val.var,
					ct->un.uint_val.min,
					ct->un.uint_val.max))
				return;
			break;
		case CONF_DUPSTR:
			if (ce->ce_vardata == NULL)
			{
				PARAM_ERROR(ce);
				return;
			}
			free(*ct->un.dupstr_val);
			*ct->un.dupstr_val = sstrdup(ce->ce_vardata);
			break;
		case CONF_BOOL:
			if (ce->ce_vardata == NULL ||
					!strcasecmp(ce->ce_vardata, "yes") ||
					!strcasecmp(ce->ce_vardata, "on") ||
					!strcasecmp(ce->ce_vardata, "true") ||
					!strcmp(ce->ce_vardata, "1"))
				*ct->un.bool_val = true;
			else if (!strcasecmp(ce->ce_vardata, "no") ||
					!strcasecmp(ce->ce_vardata, "off") ||
					!strcasecmp(ce->ce_vardata, "false") ||
					!strcmp(ce->ce_vardata, "0"))
				*ct->un.bool_val = false;
			else
			{
				slog(LG_INFO, "%s:%i: invalid boolean \"%s\" for configuration option: %s, assuming true for now",
						ce->ce_fileptr->cf_filename,
						ce->ce_varlinenum,
						ce->ce_vardata,
						ce->ce_varname);
				*ct->un.bool_val = true;
				return;
			}
	}
}

void conf_process(config_file_t *cfp)
{
	config_file_t *cfptr;
	config_entry_t *ce;
	node_t *tn;
	struct ConfTable *ct = NULL;

	for (cfptr = cfp; cfptr; cfptr = cfptr->cf_next)
	{
		for (ce = cfptr->cf_entries; ce; ce = ce->ce_next)
		{
			LIST_FOREACH(tn, confblocks.head)
			{
				ct = tn->data;

				if (!strcasecmp(ct->name, ce->ce_varname))
				{
					process_configentry(ct, ce);
					break;
				}
			}

			if (ct == NULL)
				conf_report_error(ce, "invalid configuration option: %s", ce->ce_varname);
		}
	}
}

int subblock_handler(config_entry_t *ce, list_t *entries)
{
	node_t *tn;
	struct ConfTable *ct = NULL;

	for (ce = ce->ce_entries; ce; ce = ce->ce_next)
	{
		LIST_FOREACH(tn, entries->head)
		{
			ct = tn->data;

			if (!strcasecmp(ct->name, ce->ce_varname))
			{
				process_configentry(ct, ce);
				break;
			}
		}

		if (ct == NULL)
			conf_report_error(ce, "invalid configuration option: %s", ce->ce_varname);
	}
	return 0;
}

struct ConfTable *find_top_conf(const char *name)
{
	node_t *n;
	struct ConfTable *ct;

	LIST_FOREACH(n, confblocks.head)
	{
		ct = n->data;

		if (!strcasecmp(ct->name, name))
			return ct;
	}

	return NULL;
}

struct ConfTable *find_conf_item(const char *name, list_t *conflist)
{
	node_t *n;
	struct ConfTable *ct;

	LIST_FOREACH(n, conflist->head)
	{
		ct = n->data;

		if (!strcasecmp(ct->name, name))
			return ct;
	}

	return NULL;
}

void add_top_conf(const char *name, int (*handler) (config_entry_t *ce))
{
	struct ConfTable *ct;

	if ((ct = find_top_conf(name)))
	{
		slog(LG_DEBUG, "add_top_conf(): duplicate config block '%s'.", name);
		return;
	}

	ct = BlockHeapAlloc(conftable_heap);

	ct->name = sstrdup(name);
	ct->type = CONF_HANDLER;
	ct->un.handler = handler;

	node_add(ct, node_create(), &confblocks);
}

void add_conf_item(const char *name, list_t *conflist, int (*handler) (config_entry_t *ce))
{
	struct ConfTable *ct;

	if ((ct = find_conf_item(name, conflist)))
	{
		slog(LG_DEBUG, "add_conf_item(): duplicate item %s", name);
		return;
	}

	ct = BlockHeapAlloc(conftable_heap);

	ct->name = sstrdup(name);
	ct->type = CONF_HANDLER;
	ct->un.handler = handler;

	node_add(ct, node_create(), conflist);
}

void add_uint_conf_item(const char *name, list_t *conflist, unsigned int *var, unsigned int min, unsigned int max)
{
	struct ConfTable *ct;

	if ((ct = find_conf_item(name, conflist)))
	{
		slog(LG_DEBUG, "add_uint_conf_item(): duplicate item %s", name);
		return;
	}

	ct = BlockHeapAlloc(conftable_heap);

	ct->name = sstrdup(name);
	ct->type = CONF_UINT;
	ct->un.uint_val.var = var;
	ct->un.uint_val.min = min;
	ct->un.uint_val.max = max;

	node_add(ct, node_create(), conflist);
}

void add_dupstr_conf_item(const char *name, list_t *conflist, char **var)
{
	struct ConfTable *ct;

	if ((ct = find_conf_item(name, conflist)))
	{
		slog(LG_DEBUG, "add_dupstr_conf_item(): duplicate item %s", name);
		return;
	}

	ct = BlockHeapAlloc(conftable_heap);

	ct->name = sstrdup(name);
	ct->type = CONF_DUPSTR;
	ct->un.dupstr_val = var;

	node_add(ct, node_create(), conflist);
}

void add_bool_conf_item(const char *name, list_t *conflist, bool *var)
{
	struct ConfTable *ct;

	if ((ct = find_conf_item(name, conflist)))
	{
		slog(LG_DEBUG, "add_bool_conf_item(): duplicate item %s", name);
		return;
	}

	ct = BlockHeapAlloc(conftable_heap);

	ct->name = sstrdup(name);
	ct->type = CONF_BOOL;
	ct->un.bool_val = var;

	node_add(ct, node_create(), conflist);
}

void del_top_conf(const char *name)
{
	node_t *n;
	struct ConfTable *ct;

	if (!(ct = find_top_conf(name)))
	{
		slog(LG_DEBUG, "del_top_conf(): cannot delete nonexistant block %s", name);
		return;
	}

	n = node_find(ct, &confblocks);
	node_del(n, &confblocks);

	free(ct->name);

	BlockHeapFree(conftable_heap, ct);
}

void del_conf_item(const char *name, list_t *conflist)
{
	node_t *n;
	struct ConfTable *ct;

	if (!(ct = find_conf_item(name, conflist)))
	{
		slog(LG_DEBUG, "del_conf_item(): cannot delete nonexistant item %s", name);
		return;
	}

	n = node_find(ct, conflist);
	node_del(n, conflist);

	free(ct->name);

	BlockHeapFree(conftable_heap, ct);
}

conf_handler_t conftable_get_conf_handler(struct ConfTable *ct)
{
	return ct->type == CONF_HANDLER ? ct->un.handler : NULL;
}

/* stolen from Sentinel */
int token_to_value(struct Token token_table[], const char *token)
{
	int i;

	if ((token_table != NULL) && (token != NULL))
	{
		for (i = 0; token_table[i].text != NULL; i++)
		{
			if (strcasecmp(token_table[i].text, token) == 0)
			{
				return token_table[i].value;
			}
		}
		/* If no match... */
		return TOKEN_UNMATCHED;
	}

	/* Otherwise... */
	return TOKEN_ERROR;
}

void init_confprocess(void)
{
	conftable_heap = BlockHeapCreate(sizeof(struct ConfTable), 32);

	if (!conftable_heap)
	{
		slog(LG_ERROR, "init_confprocess(): block allocator failure.");
		exit(EXIT_FAILURE);
	}
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
