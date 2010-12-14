/*
 * atheme-services: A collection of minimalist IRC services
 * privs.c: Fine grained services operator privileges
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
#include "privs.h"

mowgli_list_t operclasslist;
mowgli_list_t soperlist;

mowgli_heap_t *operclass_heap;
mowgli_heap_t *soper_heap;

void init_privs(void)
{
	operclass_heap = mowgli_heap_create(sizeof(operclass_t), 2, BH_NOW);
	soper_heap = mowgli_heap_create(sizeof(soper_t), 2, BH_NOW);
	if (!operclass_heap || !soper_heap)
	{
		slog(LG_INFO, "init_privs(): block allocator failed.");
		exit(EXIT_FAILURE);
	}
}

/*************************
 * O P E R C L A S S E S *
 *************************/
operclass_t *operclass_add(const char *name, const char *privs)
{
	operclass_t *operclass;
	mowgli_node_t *n = mowgli_node_create();

	operclass = operclass_find(name);
	if (operclass != NULL)
	{
		slog(LG_DEBUG, "operclass_add(): duplicate class %s", name);
		return NULL;
	}
	slog(LG_DEBUG, "operclass_add(): %s [%s]", name, privs);
	operclass = mowgli_heap_alloc(operclass_heap);
	mowgli_node_add(operclass, n, &operclasslist);
	operclass->name = sstrdup(name);
	operclass->privs = sstrdup(privs);
	cnt.operclass++;
	MOWGLI_ITER_FOREACH(n, soperlist.head)
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
	mowgli_node_t *n;

	if (operclass == NULL)
		return;
	slog(LG_DEBUG, "operclass_delete(): %s", operclass->name);
	n = mowgli_node_find(operclass, &operclasslist);
	mowgli_node_delete(n, &operclasslist);
	mowgli_node_free(n);
	MOWGLI_ITER_FOREACH(n, soperlist.head)
	{
		soper_t *soper = n->data;
		if (soper->operclass == operclass)
			soper->operclass = NULL;
	}
	free(operclass->name);
	free(operclass->privs);
	mowgli_heap_free(operclass_heap, operclass);
	cnt.operclass--;
}

operclass_t *operclass_find(const char *name)
{
	operclass_t *operclass;
	mowgli_node_t *n;

	MOWGLI_ITER_FOREACH(n, operclasslist.head)
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

soper_t *soper_add(const char *name, const char *classname, int flags, const char *password)
{
	soper_t *soper;
	myuser_t *mu = myuser_find(name);
	mowgli_node_t *n;
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

void soper_delete(soper_t *soper)
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

	if (soper->name)
		free(soper->name);

	free(soper->classname);
	free(soper->password);

	mowgli_heap_free(soper_heap, soper);

	cnt.soper--;
}

soper_t *soper_find(myuser_t *myuser)
{
	soper_t *soper;
	mowgli_node_t *n;

	MOWGLI_ITER_FOREACH(n, soperlist.head)
	{
		soper = (soper_t *)n->data;

		if (soper->myuser && soper->myuser == myuser)
			return soper;
	}

	return NULL;
}

soper_t *soper_find_named(const char *name)
{
	soper_t *soper;
	mowgli_node_t *n;

	MOWGLI_ITER_FOREACH(n, soperlist.head)
	{
		soper = (soper_t *)n->data;

		if (soper->name && !irccasecmp(soper->name, name))
			return soper;
	}

	return NULL;
}

bool is_soper(myuser_t *myuser)
{
	if (!myuser)
		return false;

	if (myuser->soper)
		return true;

	return false;
}

bool is_conf_soper(myuser_t *myuser)
{
	if (!myuser)
		return false;

	if (myuser->soper && myuser->soper->flags & SOPER_CONF)
		return true;

	return false;
}

/* name1 name2 name3... */
static bool string_in_list(const char *str, const char *name)
{
	char *p;
	int l;

	if (str == NULL)
		return false;
	l = strlen(name);
	while (*str != '\0')
	{
		p = strchr(str, ' ');
		if (p != NULL ? p - str == l && !strncasecmp(str, name, p - str) : !strcasecmp(str, name))
			return true;
		if (p == NULL)
			return false;
		str = p;
		while (*str == ' ')
			str++;
	}
	return false;
}

bool has_any_privs(sourceinfo_t *si)
{
	if (si->su != NULL && is_ircop(si->su))
		return true;
	if (si->smu && is_soper(si->smu))
		return true;
	return false;
}

bool has_any_privs_user(user_t *u)
{
	if (u == NULL)
		return false;
	if (is_ircop(u))
		return true;
	if (u->myuser && is_soper(u->myuser))
		return true;
	return false;
}

bool has_priv(sourceinfo_t *si, const char *priv)
{
	return si->su != NULL ? has_priv_user(si->su, priv) :
		has_priv_myuser(si->smu, priv);
}

bool has_priv_user(user_t *u, const char *priv)
{
	operclass_t *operclass;

	if (priv == NULL)
		return true;
	if (u == NULL)
		return false;

	operclass = operclass_find("user");
	if (operclass != NULL && string_in_list(operclass->privs, priv))
		return true;

	if (is_ircop(u))
	{
		operclass = operclass_find("ircop");
		if (operclass != NULL && string_in_list(operclass->privs, priv))
			return true;
	}
	if (u->myuser && is_soper(u->myuser))
	{
		operclass = u->myuser->soper->operclass;
		if (operclass == NULL)
			return false;
		if (operclass->flags & OPERCLASS_NEEDOPER && !is_ircop(u))
			return false;
		if (u->myuser->soper->password != NULL && !(u->flags & UF_SOPER_PASS))
			return false;
		if (string_in_list(operclass->privs, priv))
			return true;
	}
	return false;
}

bool has_priv_myuser(myuser_t *mu, const char *priv)
{
	operclass_t *operclass;

	if (priv == NULL)
		return true;
	if (mu == NULL)
		return false;
	if (!is_soper(mu))
		return false;
	operclass = mu->soper->operclass;
	if (operclass == NULL)
		return false;
	if (string_in_list(operclass->privs, priv))
		return true;
	return false;
}

bool has_priv_operclass(operclass_t *operclass, const char *priv)
{
	if (operclass == NULL)
		return false;
	if (string_in_list(operclass->privs, priv))
		return true;
	return false;
}

bool has_all_operclass(sourceinfo_t *si, operclass_t *operclass)
{
	char *privs2;
	char *priv;

	privs2 = sstrdup(operclass->privs);
	priv = strtok(privs2, " ");
	while (priv != NULL)
	{
		if (!has_priv(si, priv))
		{
			free(privs2);
			return false;
		}
		priv = strtok(NULL, " ");
	}
	free(privs2);
	return true;
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
