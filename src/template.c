/*
 * Copyright (c) 2005 Atheme Development Group
 * template.c: Functions to work with predefined flags collections
 *
 * See doc/LICENSE for licensing information.
 *
 * $Id: template.c 8319 2007-05-24 20:09:37Z jilles $
 */

#include "atheme.h"
#include "template.h"

/* name1=value1 name2=value2 name3=value3... */
char *getitem(char *str, char *name)
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

unsigned int get_template_flags(mychan_t *mc, char *name)
{
	metadata_t *md;
	char *d;

	if (mc != NULL)
	{
		md = metadata_find(mc, METADATA_CHANNEL, "private:templates");
		if (md != NULL)
		{
			d = getitem(md->value, name);
			if (d != NULL)
				return flags_to_bitmask(d, chanacs_flags, 0);
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
