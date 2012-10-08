/*
 * Copyright (c) 2012 Marien Zwart
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Canonicalize gmail addresses:
 * - Remove dots from the address.
 * - Remove labels (+foo).
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"misc/canon_gmail", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void canonicalize_gmail(char email[EMAILLEN + 1], void *user_data)
{
	static char buf[EMAILLEN + 1];
	const char *p, *p_at;
	char *p_out;
	stringref result;

	p_at = strchr(email, '@');
	if (!p_at || strcasecmp(p_at, "@gmail.com"))
		return;

	for (p = email, p_out = buf; p < p_at; p++) {
		if (*p == '.')
			continue;

		if (*p == '+')
			break;

		*p_out++ = *p;
	}

	/* Note the string handling here relies on the email address
	 * passed in being at most EMAILLEN long, and the canonical
	 * address being at most equally long. That is true for the
	 * loop above, but if we start mapping equivalent domains here
	 * be careful not to overflow the buffer.
	 */

	strcpy(p_out, "@gmail.com");

	strcpy(email, buf);
}

void _modinit(module_t *m)
{
	register_email_canonicalizer(canonicalize_gmail, NULL);
}

void _moddeinit(module_unload_intent_t intent)
{
	unregister_email_canonicalizer(canonicalize_gmail, NULL);
}
