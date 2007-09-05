/*
 * atheme-services: A collection of minimalist IRC services
 * symbolmatrix.c: Module symbol management.
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

void *module_symbol_get(module_t *mod, module_symbol_t *sym)
{
	return_val_if_fail(mod != NULL, NULL);
	return_val_if_fail(sym != NULL, NULL);
	return_val_if_fail(sym->sym != NULL, NULL);

	/* XXX sym->mod->header->name is wrong. :( */
	sym->mod = mod;
	sym->addr = module_locate_symbol(sym->mod->header->name, sym->sym);

	return sym->addr;
}

int module_symbol_getn(module_symbol_source_t *map, size_t n)
{
	int i;

	return_val_if_fail(map != NULL, -1);
	return_val_if_fail(n > 0, -1);

	for (i = 0; i < n; i++)
	{
		module_t *m = module_find_published(map[i].mod);
		map[i].sym.sym = map[i].symn;

		/* handle failed lookups. */
		if (module_symbol_get(m, &map[i].sym) == NULL)
			i--;
	}

	return i;
}
