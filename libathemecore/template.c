/*
 * atheme-services: A collection of minimalist IRC services
 * template.c: Functions to work with predefined flags collections
 *
 * Copyright (c) 2005-2010 Atheme Project (http://www.atheme.org)
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
#include "template.h"

mowgli_patricia_t *global_template_dict = NULL;

void set_global_template_flags(const char *name, unsigned int flags)
{
	default_template_t *def_t;

	if (global_template_dict == NULL)
		global_template_dict = mowgli_patricia_create(strcasecanon);

	def_t = mowgli_patricia_retrieve(global_template_dict, name);
	if (def_t != NULL)
	{
		def_t->flags = flags;
		return;
	}

	def_t = smalloc(sizeof(default_template_t));
	def_t->flags = flags;
	mowgli_patricia_add(global_template_dict, name, def_t);

	slog(LG_DEBUG, "set_global_template_flags(): add %s", name);
}

unsigned int get_global_template_flags(const char *name)
{
	default_template_t *def_t;

	if (global_template_dict == NULL)
		global_template_dict = mowgli_patricia_create(strcasecanon);

	def_t = mowgli_patricia_retrieve(global_template_dict, name);
	if (def_t == NULL)
		return 0;

	return def_t->flags;
}

static void release_global_template_data(const char *key, void *data, void *privdata)
{
	slog(LG_DEBUG, "release_global_template_data(): delete %s", key);

	free(data);
}

void clear_global_template_flags(void)
{
	if (global_template_dict == NULL)
		return;

	mowgli_patricia_destroy(global_template_dict, release_global_template_data, NULL);	
	global_template_dict = NULL;
}

/* name1=value1 name2=value2 name3=value3... */
const char *getitem(const char *str, const char *name)
{
	char *p;
	static char result[300];
	int l;

	l = strlen(name);
	for (;;)
	{
		p = strchr(str, '=');
		if (p == NULL)
			return NULL;
		if (p - str == l && !strncasecmp(str, name, p - str))
		{
			strlcpy(result, p, sizeof result);
			p = strchr(result, ' ');
			if (p != NULL)
				*p = '\0';
			return result;
		}
		str = strchr(p, ' ');
		if (str == NULL)
			return NULL;
		while (*str == ' ')
			str++;
	}
}

unsigned int get_template_flags(mychan_t *mc, const char *name)
{
	metadata_t *md;
	const char *d;

	if (mc != NULL)
	{
		md = metadata_find(mc, "private:templates");
		if (md != NULL)
		{
			d = getitem(md->value, name);
			if (d != NULL)
				return flags_to_bitmask(d, 0);
		}
	}

	return get_global_template_flags(name);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
