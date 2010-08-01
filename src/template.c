/*
 * atheme-services: A collection of minimalist IRC services
 * template.c: Functions to work with predefined flags collections
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
#include "template.h"

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
	if (*name != '\0' && !strcasecmp(name + 1, "op"))
	{
		switch (*name)
		{
			case 's': case 'S': return chansvs.ca_sop;
			case 'a': case 'A': return chansvs.ca_aop;
			case 'h': case 'H': return chansvs.ca_hop;
			case 'v': case 'V': return chansvs.ca_vop;
		}
	}
	return 0;
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
