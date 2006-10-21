/*
 * Copyright (c) 2005-2006 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Translation framework.
 *
 * $Id: culture.c 6809 2006-10-21 19:47:41Z jilles $
 */

#include "atheme.h"

dictionary_tree_t *itranslation_tree; /* internal translations, userserv/nickserv etc */
dictionary_tree_t *translation_tree; /* language translations */

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
	itranslation_tree = dictionary_create("itranslation_tree", HASH_ITRANS, strcmp);
	translation_tree = dictionary_create("translation_tree", HASH_TRANS, strcmp);
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
	node_t *n;
	translation_t *t;

	/* See if an internal substitution is present. */
	if ((t = dictionary_retrieve(itranslation_tree, str)) != NULL)
		str = t->replacement;

	if ((t = dictionary_retrieve(translation_tree, str)) != NULL)
		return t->replacement;

	return str;
}

/*
 * itranslation_create(char *str, char *trans)
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
void itranslation_create(char *str, char *trans)
{
	translation_t *t = smalloc(sizeof(translation_t));

	t->name = sstrdup(str);
	t->replacement = sstrdup(trans);

	dictionary_add(itranslation_tree, t->name, t);
}

/*
 * itranslation_destroy(char *str)
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
void itranslation_destroy(char *str)
{
	node_t *n, *tn;
	translation_t *t = dictionary_delete(itranslation_tree, str);

	if (t == NULL)
		return;

	free(t->name);
	free(t->replacement);
	free(t);
}

/*
 * translation_create(char *str, char *trans)
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
void translation_create(char *str, char *trans)
{
	char buf[BUFSIZE];
	translation_t *t = smalloc(sizeof(translation_t));

	strlcpy(buf, str, BUFSIZE);
	replace(buf, BUFSIZE, "\\2", "\2");

	t->name = sstrdup(buf);

	strlcpy(buf, trans, BUFSIZE);
	replace(buf, BUFSIZE, "\\2", "\2");

	t->replacement = sstrdup(buf);

	dictionary_add(translation_tree, t->name, t);
}

/*
 * translation_destroy(char *str)
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
void translation_destroy(char *str)
{
	node_t *n, *tn;
	translation_t *t = dictionary_delete(translation_tree, str);

	if (t == NULL)
		return;

	free(t->name);
	free(t->replacement);
	free(t);
}
