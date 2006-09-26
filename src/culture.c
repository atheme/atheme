/*
 * Copyright (c) 2005-2006 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Translation framework.
 *
 * $Id: culture.c 6485 2006-09-26 15:31:48Z jilles $
 */

#include "atheme.h"

list_t itranshash[HASHSIZE]; /* internal translations, userserv/nickserv etc */
list_t transhash[HASHSIZE]; /* language translations */

/*
 * translation_get()
 *
 * Inputs:
 *       string to get translation for
 *
 * Outputs:
 *       translated string, or the original string if no translation
 *       was found
 *
 * Side Effects:
 *       none
 */
const char *translation_get(const char *str)
{
	node_t *n;

	LIST_FOREACH(n, itranshash[shash((unsigned char *) str)].head)
	{
		translation_t *t = (translation_t *) n->data;

		if (!strcmp(str, t->name))
		{
			str = t->replacement;
			break;
		}
	}

	LIST_FOREACH(n, transhash[shash((unsigned char *) str)].head)
	{
		translation_t *t = (translation_t *) n->data;

		if (!strcmp(str, t->name))
			return t->replacement;
	}

	return str;
}

/*
 * itranslation_create()
 *
 * Inputs:
 *       string to translate, translation of the string
 *
 * Outputs:
 *       none
 *
 * Side Effects:
 *       a new translation is added to the cache
 */
void itranslation_create(char *str, char *trans)
{
	translation_t *t = smalloc(sizeof(translation_t));

	t->name = sstrdup(str);
	t->replacement = sstrdup(trans);

	node_add(t, node_create(), &itranshash[shash((unsigned char *) t->name)]);
}

/*
 * itranslation_destroy()
 *
 * Inputs:
 *       string to remove from the translation cache
 *
 * Outputs:
 *       none
 *
 * Side Effects:
 *       a string is removed from the translation cache
 */
void itranslation_destroy(char *str)
{
	node_t *n, *tn;
	translation_t *t;

	LIST_FOREACH_SAFE(n, tn, itranshash[shash((unsigned char *) str)].head)
	{
		t = n->data;
		if (!strcmp(str, t->name))
		{
			node_del(n, &itranshash[shash((unsigned char *) str)]);
			node_free(n);

			free(t->name);
			free(t->replacement);
			free(t);
		}
	}
}
/*
 * translation_create()
 *
 * Inputs:
 *       string to translate, translation of the string
 *
 * Outputs:
 *       none
 *
 * Side Effects:
 *       a new translation is added to the cache
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

	node_add(t, node_create(), &transhash[shash((unsigned char *) t->name)]);
}

/*
 * translation_destroy()
 *
 * Inputs:
 *       string to remove from the translation cache
 *
 * Outputs:
 *       none
 *
 * Side Effects:
 *       a string is removed from the translation cache
 */
void translation_destroy(char *str)
{
	node_t *n, *tn;
	translation_t *t;

	LIST_FOREACH_SAFE(n, tn, transhash[shash((unsigned char *) str)].head)
	{
		t = n->data;
		if (!strcmp(str, t->name))
		{
			node_del(n, &transhash[shash((unsigned char *) str)]);
			node_free(n);

			free(t->name);
			free(t->replacement);
			free(t);
		}
	}
}
