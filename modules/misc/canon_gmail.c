/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2012 Marien Zwart
 * Copyright (C) 2018 Atheme Development Group (https://atheme.github.io/)
 *
 * Canonicalize GMail addresses:
 * - Remove dots from the address.
 * - Remove labels (+foo).
 */

#include <atheme.h>

#define GMAIL_SUFFIX "@gmail.com"

static void
email_canonicalize_gmail(char email[static (EMAILLEN + 1)],
                         void ATHEME_VATTR_UNUSED *const restrict user_data)
{
	const char *const p_at = strchr(email, '@');

	if (! p_at || strcasecmp(p_at, GMAIL_SUFFIX) != 0)
		return;

	char buf[EMAILLEN + 1];
	char *p_out = buf;

	(void) memset(buf, 0x00, sizeof buf);

	for (const char *p = email; p < p_at; p++)
	{
		if (*p == '.')
			continue;

		if (*p == '+')
			break;

		*p_out++ = *p;
	}

	if (mowgli_strlcat(buf, GMAIL_SUFFIX, sizeof buf) >= sizeof buf)
	{
		(void) slog(LG_ERROR, "%s: buffer is too short (BUG)", MOWGLI_FUNC_NAME);
		return;
	}

	(void) slog(LG_DEBUG, "%s: '%s' -> '%s'", MOWGLI_FUNC_NAME, email, buf);
	(void) memcpy(email, buf, sizeof buf);
}

static void
mod_init(struct module ATHEME_VATTR_UNUSED *const restrict m)
{
	(void) register_email_canonicalizer(&email_canonicalize_gmail, NULL);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	(void) unregister_email_canonicalizer(&email_canonicalize_gmail, NULL);
}

SIMPLE_DECLARE_MODULE_V1("misc/canon_gmail", MODULE_UNLOAD_CAPABILITY_OK)
