/*
 * atheme-services: A collection of minimalist IRC services   
 * culture.c: Translation framework.
 *
 * Copyright (c) 2005-2009 Atheme Project (http://www.atheme.org)           
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
#include "internal.h"

struct translation_
{
	char *name;
	char *replacement;
};

typedef struct translation_ translation_t;

static mowgli_patricia_t *itranslation_tree; /* internal translations, userserv/nickserv etc */
static mowgli_patricia_t *translation_tree; /* language translations */

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
void translation_init(void)
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
const char *translation_get(const char *str)
{
	translation_t *t;

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
void itranslation_create(const char *str, const char *trans)
{
	translation_t *t = smalloc(sizeof(translation_t));

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
void itranslation_destroy(const char *str)
{
	translation_t *t = mowgli_patricia_delete(itranslation_tree, str);

	if (t == NULL)
		return;

	free(t->name);
	free(t->replacement);
	free(t);
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
void translation_create(const char *str, const char *trans)
{
	char buf[BUFSIZE];
	translation_t *t = smalloc(sizeof(translation_t));

	strlcpy(buf, str, BUFSIZE);
	replace(buf, BUFSIZE, "\\2", "\2");

	t->name = sstrdup(buf);

	strlcpy(buf, trans, BUFSIZE);
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
void translation_destroy(const char *str)
{
	translation_t *t = mowgli_patricia_delete(translation_tree, str);

	if (t == NULL)
		return;

	free(t->name);
	free(t->replacement);
	free(t);
}

enum
{
	LANG_VALID = 1		/* have catalogs for this */
};

struct language_
{
	char *name;
	unsigned int flags; /* LANG_* */
	mowgli_node_t node;
};

static mowgli_list_t language_list;

void
language_init(void)
{
	DIR *dir;
	struct dirent *ent;
	language_t *lang;

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

language_t *
language_add(const char *name)
{
	language_t *lang;

	if (!strcmp(name, "default"))
		return NULL;
	lang = language_find(name);
	if (lang != NULL)
		return lang;
	slog(LG_DEBUG, "language_add(): %s", name);
	lang = smalloc(sizeof(*lang));
	lang->name = sstrdup(name);
	mowgli_node_add(lang, &lang->node, &language_list);
	return lang;
}

language_t *
language_find(const char *name)
{
	mowgli_node_t *n;
	language_t *lang;

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
	language_t *lang;

	names[0] = '\0';
	MOWGLI_ITER_FOREACH(n, language_list.head)
	{
		lang = n->data;
		if (lang->flags & LANG_VALID)
		{
			if (names[0] != '\0')
				strlcat(names, " ", sizeof names);
			strlcat(names, lang->name, sizeof names);
		}
	}
	return names;
}

const char *
language_get_name(const language_t *lang)
{
	if (lang == NULL)
		return "default";
	return lang->name;
}

const char *
language_get_real_name(const language_t *lang)
{
	if (lang == NULL)
		return config_options.language;
	return lang->name;
}

bool
language_is_valid(const language_t *lang)
{
	if (lang == NULL)
		return true;
	return (lang->flags & LANG_VALID) != 0;
}

void
language_set_active(language_t *lang)
{
#ifdef ENABLE_NLS
	static language_t *currlang;

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
	textdomain(PACKAGE_NAME);
	bindtextdomain(PACKAGE_NAME, LOCALEDIR);
	currlang = lang;
	setenv("LANGUAGE", currlang->name, 1);
#endif
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
