/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2015 Atheme Project (http://atheme.org/)
 * Copyright (C) 2018 Atheme Development Group (https://atheme.github.io/)
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * atheme-services: A collection of minimalist IRC services
 * culture.c: Translation framework.
 */

#include <atheme.h>
#include "internal.h"

static mowgli_patricia_t *itranslation_tree; /* internal translations, userserv/nickserv etc */
static mowgli_patricia_t *translation_tree; /* language translations */

static bool languages_available = false;

/*
 * translation_init()
 *
 * Initializes the translation DTrees.
 *
 * Inputs:
 *     - none
 *
 * Outputs:
 *     - nothing
 *
 * Side Effects:
 *     - on success, nothing
 *     - on failure, the program will abort
 */
void
translation_init(void)
{
	itranslation_tree = mowgli_patricia_create(noopcanon);
	translation_tree = mowgli_patricia_create(noopcanon);
}

/*
 * translation_get(const char *str)
 *
 * Inputs:
 *     - string to get translation for
 *
 * Outputs:
 *     - translated string, or the original string if no translation
 *       was found
 *
 * Side Effects:
 *     - none
 */
const char *
translation_get(const char *str)
{
	struct translation *t;

	/* See if an internal substitution is present. */
	if ((t = mowgli_patricia_retrieve(itranslation_tree, str)) != NULL)
		str = t->replacement;

	if ((t = mowgli_patricia_retrieve(translation_tree, str)) != NULL)
		return t->replacement;

	return str;
}

/*
 * itranslation_create(const char *str, const char *trans)
 *
 * Inputs:
 *     - string to translate, translation of the string
 *
 * Outputs:
 *     - none
 *
 * Side Effects:
 *     - a new translation is added to the cache
 */
void
itranslation_create(const char *str, const char *trans)
{
	struct translation *const t = smalloc(sizeof *t);

	t->name = sstrdup(str);
	t->replacement = sstrdup(trans);

	mowgli_patricia_add(itranslation_tree, t->name, t);
}

/*
 * itranslation_destroy(const char *str)
 *
 * Inputs:
 *     - string to remove from the translation cache
 *
 * Outputs:
 *     - none
 *
 * Side Effects:
 *     - a string is removed from the translation cache
 */
void
itranslation_destroy(const char *str)
{
	struct translation *t = mowgli_patricia_delete(itranslation_tree, str);

	if (t == NULL)
		return;

	sfree(t->name);
	sfree(t->replacement);
	sfree(t);
}

/*
 * translation_create(const char *str, const char *trans)
 *
 * Inputs:
 *     - string to translate, translation of the string
 *
 * Outputs:
 *     - none
 *
 * Side Effects:
 *     - a new translation is added to the cache
 */
void
translation_create(const char *str, const char *trans)
{
	char buf[BUFSIZE];
	struct translation *const t = smalloc(sizeof *t);

	mowgli_strlcpy(buf, str, BUFSIZE);
	replace(buf, BUFSIZE, "\\2", "\2");

	t->name = sstrdup(buf);

	mowgli_strlcpy(buf, trans, BUFSIZE);
	replace(buf, BUFSIZE, "\\2", "\2");

	t->replacement = sstrdup(buf);

	mowgli_patricia_add(translation_tree, t->name, t);
}

/*
 * translation_destroy(const char *str)
 *
 * Inputs:
 *     - string to remove from the translation cache
 *
 * Outputs:
 *     - none
 *
 * Side Effects:
 *     - a string is removed from the translation cache
 */
void
translation_destroy(const char *str)
{
	struct translation *t = mowgli_patricia_delete(translation_tree, str);

	if (t == NULL)
		return;

	sfree(t->name);
	sfree(t->replacement);
	sfree(t);
}

enum
{
	LANG_VALID = 1		/* have catalogs for this */
};

static mowgli_list_t language_list;

void
language_init(void)
{
	DIR *dir;
	struct dirent *ent;
	struct language *lang;

	lang = language_add("en");
	lang->flags |= LANG_VALID;
	dir = opendir(LOCALEDIR);
	if (dir != NULL)
	{
		while ((ent = readdir(dir)) != NULL)
		{
			if (ent->d_name[0] != '.' &&
					strcmp(ent->d_name, "all_languages") &&
					strcmp(ent->d_name, "locale.alias") &&
					strcmp(ent->d_name, "default"))
			{
				lang = language_add(ent->d_name);
				lang->flags |= LANG_VALID;
			}
		}
		closedir(dir);
	}
}

struct language *
language_add(const char *name)
{
	struct language *lang;

	if (name == NULL || !strcmp(name, "default"))
		return NULL;

	lang = language_find(name);
	if (lang != NULL)
		return lang;

	slog(LG_DEBUG, "language_add(): %s", name);
	lang = smalloc(sizeof *lang);
	lang->name = sstrdup(name);
	mowgli_node_add(lang, &lang->node, &language_list);
	return lang;
}

struct language *
language_find(const char *name)
{
	mowgli_node_t *n;
	struct language *lang;

	if (name == NULL)
		return NULL;

	MOWGLI_ITER_FOREACH(n, language_list.head)
	{
		lang = n->data;
		if (!strcmp(lang->name, name))
			return lang;
	}
	return NULL;
}

const char *
language_names(void)
{
	static char names[512];
	mowgli_node_t *n;
	struct language *lang;

	names[0] = '\0';
	MOWGLI_ITER_FOREACH(n, language_list.head)
	{
		lang = n->data;
		if (lang->flags & LANG_VALID)
		{
			if (names[0] != '\0')
				mowgli_strlcat(names, " ", sizeof names);
			mowgli_strlcat(names, lang->name, sizeof names);
		}
	}
	return names;
}

const char *
language_get_name(const struct language *lang)
{
	if (lang == NULL)
		return "default";
	return lang->name;
}

const char *
language_get_real_name(const struct language *lang)
{
	if (lang == NULL)
		return config_options.language;
	return lang->name;
}

bool
language_is_valid(const struct language *lang)
{
	if (lang == NULL)
		return true;
	return (lang->flags & LANG_VALID) != 0;
}

void
language_set_active(struct language *lang)
{
#ifdef ENABLE_NLS
	if (! languages_available)
		return;

	static struct language *currlang;

	if (lang == NULL)
	{
		lang = language_find(config_options.language);
		if (lang == NULL)
			lang = language_find("en");
	}
	if (currlang == lang)
		return;
	slog(LG_DEBUG, "language_set_active(): changing language from [%s] to [%s]",
			currlang != NULL ? currlang->name : "default",
			lang->name);
	setlocale(LC_MESSAGES, lang->name);
	textdomain(PACKAGE_TARNAME);
	bindtextdomain(PACKAGE_TARNAME, LOCALEDIR);
	currlang = lang;
	setenv("LANGUAGE", currlang->name, 1);
#endif
}

bool
languages_get_available(void)
{
	return languages_available;
}

void
languages_set_available(const bool avail)
{
	languages_available = avail;
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
