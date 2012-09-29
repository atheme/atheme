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

static void canonicalize_gmail(hook_email_canonicalize_t *data)
{
	char *p, *p_out, *p_at;

	p_at = strchr(data->email, '@');
	if (!p_at || strcasecmp(p_at, "@gmail.com"))
		return;

	for (p = p_out = data->email; p < p_at; p++) {
		if (*p == '.')
			continue;

		if (*p == '+')
			break;

		*p_out++ = *p;
	}

	strcpy(p_out, "@gmail.com");
}

void _modinit(module_t *m)
{
	hook_add_event("email_canonicalize");
	hook_add_email_canonicalize(canonicalize_gmail);
	canonicalize_emails();
}

void _moddeinit(module_unload_intent_t intent)
{
	hook_del_email_canonicalize(canonicalize_gmail);
	canonicalize_emails();
}
