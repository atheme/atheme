/*
 * Copyright (c) 2005-2006 Jilles Tjoelker, et al
 * privs.c: Fine grained services operator privileges
 *
 * See doc/LICENSE for licensing information.
 *
 * $Id: privs.c 7089 2006-11-05 17:06:22Z jilles $
 */

#include "atheme.h"

list_t operclasslist;
list_t soperlist;

static BlockHeap *operclass_heap;
static BlockHeap *soper_heap;

void init_privs(void)
{
	operclass_heap = BlockHeapCreate(sizeof(operclass_t), 2);
	soper_heap = BlockHeapCreate(sizeof(soper_t), 2);
	if (!operclass_heap || !soper_heap)
	{
		slog(LG_INFO, "init_privs(): block allocator failed.");
		exit(EXIT_FAILURE);
	}
}

/*************************
 * O P E R C L A S S E S *
 *************************/
operclass_t *operclass_add(char *name, char *privs)
{
	operclass_t *operclass;
	node_t *n = node_create();

	operclass = operclass_find(name);
	if (operclass != NULL)
	{
		slog(LG_DEBUG, "operclass_add(): duplicate class %s", name);
		return NULL;
	}
	slog(LG_DEBUG, "operclass_add(): %s [%s]", name, privs);
	operclass = BlockHeapAlloc(operclass_heap);
	node_add(operclass, n, &operclasslist);
	operclass->name = sstrdup(name);
	operclass->privs = sstrdup(privs);
	cnt.operclass++;
	LIST_FOREACH(n, soperlist.head)
	{
		soper_t *soper = n->data;
		if (soper->operclass == NULL &&
				!strcasecmp(name, soper->classname))
			soper->operclass = operclass;
	}
	return operclass;
}

void operclass_delete(operclass_t *operclass)
{
	node_t *n;

	if (operclass == NULL)
		return;
	slog(LG_DEBUG, "operclass_delete(): %s", operclass->name);
	n = node_find(operclass, &operclasslist);
	node_del(n, &operclasslist);
	node_free(n);
	LIST_FOREACH(n, soperlist.head)
	{
		soper_t *soper = n->data;
		if (soper->operclass == operclass)
			soper->operclass = NULL;
	}
	free(operclass->name);
	free(operclass->privs);
	BlockHeapFree(operclass_heap, operclass);
	cnt.operclass--;
}

operclass_t *operclass_find(char *name)
{
	operclass_t *operclass;
	node_t *n;

	LIST_FOREACH(n, operclasslist.head)
	{
		operclass = (operclass_t *)n->data;

		if (!strcasecmp(operclass->name, name))
			return operclass;
	}

	return NULL;
}

/***************
 * S O P E R S *
 ***************/

soper_t *soper_add(char *name, char *classname, int flags)
{
	soper_t *soper;
	myuser_t *mu = myuser_find(name);
	node_t *n;
	operclass_t *operclass = operclass_find(classname);

	soper = mu ? soper_find(mu) : soper_find_named(name);
	if (soper != NULL)
	{
		if (flags & SOPER_CONF && !(soper->flags & SOPER_CONF))
		{
			slog(LG_INFO, "soper_add(): conf soper %s (%s) is replacing DB soper with class %s", name, classname, soper->classname);
			soper_delete(soper);
		}
		else if (!(flags & SOPER_CONF) && soper->flags & SOPER_CONF)
		{
			slog(LG_INFO, "soper_add(): ignoring DB soper %s (%s) because of conf soper with class %s", name, classname, soper->classname);
			return NULL;
		}
		else
		{
			slog(LG_INFO, "soper_add(): duplicate soper %s", name);
			return NULL;
		}
	}
	slog(LG_DEBUG, "soper_add(): %s -> %s", (mu) ? mu->name : name, operclass ? operclass->name : "<null>");

	soper = BlockHeapAlloc(soper_heap);
	n = node_create();

	node_add(soper, n, &soperlist);

	if (mu)
	{
		soper->myuser = mu;
		mu->soper = soper;
		soper->name = NULL;
	}
	else
	{
		soper->name = sstrdup(name);
		soper->myuser = NULL;
	}
	soper->operclass = operclass;
	soper->classname = sstrdup(classname);
	soper->flags = flags;

	cnt.soper++;

	return soper;
}

void soper_delete(soper_t *soper)
{
	node_t *n;

	if (!soper)
	{
		slog(LG_DEBUG, "soper_delete(): called for null soper");

		return;
	}

	slog(LG_DEBUG, "soper_delete(): %s", (soper->myuser) ? soper->myuser->name : soper->name);

	n = node_find(soper, &soperlist);
	node_del(n, &soperlist);
	node_free(n);

	if (soper->myuser)
		soper->myuser->soper = NULL;

	if (soper->name)
		free(soper->name);

	free(soper->classname);

	BlockHeapFree(soper_heap, soper);

	cnt.soper--;
}

soper_t *soper_find(myuser_t *myuser)
{
	soper_t *soper;
	node_t *n;

	LIST_FOREACH(n, soperlist.head)
	{
		soper = (soper_t *)n->data;

		if (soper->myuser && soper->myuser == myuser)
			return soper;
	}

	return NULL;
}

soper_t *soper_find_named(char *name)
{
	soper_t *soper;
	node_t *n;

	LIST_FOREACH(n, soperlist.head)
	{
		soper = (soper_t *)n->data;

		if (soper->name && !irccasecmp(soper->name, name))
			return soper;
	}

	return NULL;
}

boolean_t is_soper(myuser_t *myuser)
{
	if (!myuser)
		return FALSE;

	if (myuser->soper)
		return TRUE;

	return FALSE;
}

boolean_t is_conf_soper(myuser_t *myuser)
{
	if (!myuser)
		return FALSE;

	if (myuser->soper && myuser->soper->flags & SOPER_CONF)
		return TRUE;

	return FALSE;
}

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

boolean_t has_any_privs(sourceinfo_t *si)
{
	if (si->su != NULL && is_ircop(si->su))
		return TRUE;
	if (si->smu && is_soper(si->smu))
		return TRUE;
	return FALSE;
}

boolean_t has_any_privs_user(user_t *u)
{
	if (u == NULL)
		return FALSE;
	if (is_ircop(u))
		return TRUE;
	if (u->myuser && is_soper(u->myuser))
		return TRUE;
	return FALSE;
}

boolean_t has_priv(sourceinfo_t *si, const char *priv)
{
	return si->su != NULL ? has_priv_user(si->su, priv) :
		has_priv_myuser(si->smu, priv);
}

boolean_t has_priv_user(user_t *u, const char *priv)
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
	if (u->myuser && is_soper(u->myuser))
	{
		operclass = u->myuser->soper->operclass;
		if (operclass == NULL)
			return FALSE;
		if (string_in_list(operclass->privs, priv))
			return TRUE;
	}
	return FALSE;
}

boolean_t has_priv_myuser(myuser_t *mu, const char *priv)
{
	operclass_t *operclass;

	if (priv == NULL)
		return TRUE;
	if (mu == NULL)
		return FALSE;
	if (!is_soper(mu))
		return FALSE;
	operclass = mu->soper->operclass;
	if (operclass == NULL)
		return FALSE;
	if (string_in_list(operclass->privs, priv))
		return TRUE;
	return FALSE;
}

boolean_t has_priv_operclass(operclass_t *operclass, const char *priv)
{
	if (operclass == NULL)
		return FALSE;
	if (string_in_list(operclass->privs, priv))
		return TRUE;
	return FALSE;
}

boolean_t has_all_operclass(sourceinfo_t *si, operclass_t *operclass)
{
	char *dup;
	char *priv;

	dup = sstrdup(operclass->privs);
	priv = strtok(dup, " ");
	while (priv != NULL)
	{
		if (!has_priv(si, priv))
		{
			free(dup);
			return FALSE;
		}
		priv = strtok(NULL, " ");
	}
	free(dup);
	return TRUE;
}
