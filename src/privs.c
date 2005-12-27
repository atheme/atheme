/*
 * Copyright (c) 2005 Jilles Tjoelker, et al
 * privs.c: Fine grained services operator privileges
 *
 * See doc/LICENSE for licensing information.
 *
 * $Id$
 */

#include "atheme.h"

/* name1 name2 name3... */
static boolean_t string_in_list(const char *str, const char *name)
{
	char *p;
	int l;

	if (str == NULL)
		return FALSE;
	l = strlen(name);
	while (*str != '\0')
	{
		p = strchr(str, ' ');
		if (p != NULL ? p - str == l && !strncasecmp(str, name, p - str) : !strcasecmp(str, name))
			return TRUE;
		if (p == NULL)
			return FALSE;
		str = p;
		while (*str == ' ')
			str++;
	}
	return FALSE;
}

boolean_t has_any_privs(user_t *u)
{
	if (is_ircop(u))
		return TRUE;
	if (u->myuser && is_sra(u->myuser))
		return TRUE;
	return FALSE;
}

boolean_t has_priv(user_t *u, const char *priv)
{
	operclass_t *operclass;

	if (priv == NULL)
		return TRUE;
	if (u == NULL)
		return FALSE;
	if (is_ircop(u))
	{
		operclass = operclass_find("ircop");
		if (operclass != NULL && string_in_list(operclass->privs, priv))
			return TRUE;
	}
	return u->myuser && is_sra(u->myuser);
}

boolean_t has_priv_myuser(myuser_t *mu, const char *priv)
{
	if (priv == NULL)
		return TRUE;
	if (mu == NULL)
		return FALSE;
	return is_sra(mu);
}
