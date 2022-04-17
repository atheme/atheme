/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2012 Atheme Project (http://atheme.org/)
 * Copyright (C) 2018 Atheme Development Group (https://atheme.github.io/)
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * atheme-services: A collection of minimalist IRC services
 * privs.c: Fine grained services operator privileges
 */

#include <atheme.h>
#include "internal.h"

mowgli_list_t operclasslist;
mowgli_list_t soperlist;

static mowgli_heap_t *operclass_heap = NULL;
static mowgli_heap_t *soper_heap = NULL;

static struct operclass *user_r = NULL;
static struct operclass *authenticated_r = NULL;
static struct operclass *ircop_r = NULL;

void
init_privs(void)
{
	operclass_heap = sharedheap_get(sizeof(struct operclass));
	soper_heap = sharedheap_get(sizeof(struct soper));

	if (!operclass_heap || !soper_heap)
	{
		slog(LG_INFO, "init_privs(): block allocator failed.");
		exit(EXIT_FAILURE);
	}

	/* create built-in operclasses. */
	user_r = operclass_add("user", "", OPERCLASS_BUILTIN);
	authenticated_r = operclass_add("authenticated", AC_AUTHENTICATED, OPERCLASS_BUILTIN);
	ircop_r = operclass_add("ircop", "", OPERCLASS_BUILTIN);
}

/*************************
 * O P E R C L A S S E S *
 *************************/
/*
 * operclass_add(const char *name, const char *privs)
 *
 * Add or override an operclass.
 */
struct operclass *
operclass_add(const char *name, const char *privs, int flags)
{
	struct operclass *operclass;
	mowgli_node_t *n;

	operclass = operclass_find(name);

	if (operclass != NULL)
	{
		bool builtin = operclass->flags & OPERCLASS_BUILTIN;

		slog(LG_DEBUG, "operclass_add(): update %s [%s]", name, privs);

		sfree(operclass->privs);
		operclass->privs = sstrdup(privs);
		operclass->flags = flags | (builtin ? OPERCLASS_BUILTIN : 0);

		return operclass;
	}

	slog(LG_DEBUG, "operclass_add(): create %s [%s]", name, privs);

	operclass = mowgli_heap_alloc(operclass_heap);
	operclass->name = sstrdup(name);
	operclass->privs = sstrdup(privs);
	operclass->flags = flags;

	mowgli_node_add(operclass, &operclass->node, &operclasslist);

	cnt.operclass++;

	MOWGLI_ITER_FOREACH(n, soperlist.head)
	{
		struct soper *soper = n->data;
		if (soper->operclass == NULL &&
				!strcasecmp(name, soper->classname))
			soper->operclass = operclass;
	}

	return operclass;
}

void
operclass_delete(struct operclass *operclass)
{
	mowgli_node_t *n;

	if (operclass == NULL)
		return;

	if (operclass->flags & OPERCLASS_BUILTIN)
		return;

	slog(LG_DEBUG, "operclass_delete(): %s", operclass->name);
	mowgli_node_delete(&operclass->node, &operclasslist);

	MOWGLI_ITER_FOREACH(n, soperlist.head)
	{
		struct soper *soper = n->data;
		if (soper->operclass == operclass)
			soper->operclass = NULL;
	}

	sfree(operclass->name);
	sfree(operclass->privs);

	mowgli_heap_free(operclass_heap, operclass);
	cnt.operclass--;
}

struct operclass *
operclass_find(const char *name)
{
	struct operclass *operclass;
	mowgli_node_t *n;

	MOWGLI_ITER_FOREACH(n, operclasslist.head)
	{
		operclass = (struct operclass *)n->data;

		if (!strcasecmp(operclass->name, name))
			return operclass;
	}

	return NULL;
}

/***************
 * S O P E R S *
 ***************/

struct soper *
soper_add(const char *name, const char *classname, int flags, const char *password)
{
	struct soper *soper;
	struct myuser *mu = myuser_find(name);
	mowgli_node_t *n;
	struct operclass *operclass = operclass_find(classname);

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

	slog(LG_DEBUG, "soper_add(): %s -> %s", (mu) ? entity(mu)->name : name, operclass ? operclass->name : "<null>");

	soper = mowgli_heap_alloc(soper_heap);
	n = mowgli_node_create();

	mowgli_node_add(soper, n, &soperlist);

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
	soper->password = sstrdup(password);

	cnt.soper++;

	return soper;
}

void
soper_delete(struct soper *soper)
{
	mowgli_node_t *n;

	if (!soper)
	{
		slog(LG_DEBUG, "soper_delete(): called for null soper");

		return;
	}

	slog(LG_DEBUG, "soper_delete(): %s", (soper->myuser) ? entity(soper->myuser)->name : soper->name);

	n = mowgli_node_find(soper, &soperlist);
	mowgli_node_delete(n, &soperlist);
	mowgli_node_free(n);

	if (soper->myuser)
		soper->myuser->soper = NULL;

	sfree(soper->name);
	sfree(soper->classname);
	sfree(soper->password);

	mowgli_heap_free(soper_heap, soper);

	cnt.soper--;
}

struct soper *
soper_find(struct myuser *myuser)
{
	struct soper *soper;
	mowgli_node_t *n;

	MOWGLI_ITER_FOREACH(n, soperlist.head)
	{
		soper = (struct soper *)n->data;

		if (soper->myuser && soper->myuser == myuser)
			return soper;
	}

	return NULL;
}

struct soper *
soper_find_named(const char *name)
{
	struct soper *soper;
	mowgli_node_t *n;

	MOWGLI_ITER_FOREACH(n, soperlist.head)
	{
		soper = (struct soper *)n->data;

		if (soper->name && !irccasecmp(soper->name, name))
			return soper;
	}

	return NULL;
}
struct soper *
soper_find_eid(const char *eid)
{
	struct soper *soper;
	mowgli_node_t *n;
	char lookup_eid[IDLEN+2]; // 2 for ? and \0

	snprintf(lookup_eid, sizeof lookup_eid, "?%s", eid);

	MOWGLI_ITER_FOREACH(n, soperlist.head)
	{
		soper = (struct soper *)n->data;

		if (soper->name && !irccasecmp(soper->name, lookup_eid))
			return soper;
	}

	return NULL;
}

bool
is_soper(struct myuser *myuser)
{
	if (!myuser)
		return false;

	if (myuser->soper)
		return true;

	return false;
}

bool
is_conf_soper(struct myuser *myuser)
{
	if (!myuser)
		return false;

	if (myuser->soper && myuser->soper->flags & SOPER_CONF)
		return true;

	return false;
}

bool
is_conf_named_soper(struct myuser *myuser)
{
	return is_conf_soper(myuser) && !(myuser->soper->flags & SOPER_EID);
}

bool
has_priv_operclass(struct operclass *operclass, const char *priv)
{
	if (operclass == NULL)
		return false;
	if (string_in_list(priv, operclass->privs))
		return true;
	return false;
}

bool
has_any_privs(struct sourceinfo *si)
{
	if (si->su != NULL && is_ircop(si->su))
		return true;
	if (si->smu && is_soper(si->smu))
		return true;
	return false;
}

bool
has_any_privs_user(struct user *u)
{
	if (u == NULL)
		return false;
	if (is_ircop(u))
		return true;
	if (u->myuser && is_soper(u->myuser))
		return true;
	return false;
}

bool
has_priv(struct sourceinfo *si, const char *priv)
{
	return si->su != NULL ? has_priv_user(si->su, priv) :
		has_priv_myuser(si->smu, priv);
}

bool
has_priv_user(struct user *u, const char *priv)
{
	struct operclass *operclass;

	if (priv == NULL)
		return true;

	if (u == NULL)
		return false;

	if (has_priv_operclass(user_r, priv))
		return true;

	if (is_ircop(u) && has_priv_operclass(ircop_r, priv))
		return true;

	if (u->myuser != NULL && has_priv_operclass(authenticated_r, priv))
		return true;

	if (u->myuser && is_soper(u->myuser))
	{
		operclass = u->myuser->soper->operclass;
		if (operclass == NULL)
			return false;
		if (operclass->flags & OPERCLASS_NEEDOPER && !is_ircop(u))
			return false;
		if (u->myuser->soper->password != NULL && !(u->flags & UF_SOPER_PASS))
			return false;
		if (has_priv_operclass(operclass, priv))
			return true;
	}

	return false;
}

bool
has_priv_myuser(struct myuser *mu, const char *priv)
{
	struct operclass *operclass;

	if (priv == NULL)
		return true;
	if (mu == NULL)
		return false;

	if (has_priv_operclass(authenticated_r, priv))
		return true;

	if (!is_soper(mu))
		return false;
	operclass = mu->soper->operclass;
	if (operclass == NULL)
		return false;
	if (has_priv_operclass(operclass, priv))
		return true;

	return false;
}

bool
has_all_operclass(struct sourceinfo *si, struct operclass *operclass)
{
	char *privs2;
	char *priv;

	privs2 = sstrdup(operclass->privs);
	priv = strtok(privs2, " ");
	while (priv != NULL)
	{
		if (!has_priv(si, priv))
		{
			sfree(privs2);
			return false;
		}
		priv = strtok(NULL, " ");
	}
	sfree(privs2);
	return true;
}

/**********************************************************************************/

const struct soper *
get_sourceinfo_soper(struct sourceinfo *si)
{
	if (si->smu != NULL && is_soper(si->smu))
		return si->smu->soper;

	return NULL;
}

const struct operclass *
get_sourceinfo_operclass(struct sourceinfo *si)
{
	struct operclass *out = NULL;
	const struct soper *soper;

	if ((soper = get_sourceinfo_soper(si)) != NULL)
		return soper->operclass;

	if (si->su != NULL && is_ircop(si->su))
	{
		if ((out = operclass_find("ircop")) != NULL)
			return out;
	}

	if ((out = operclass_find("user")) != NULL)
		return out;

	return NULL;
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
