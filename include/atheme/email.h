/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2003-2004 E. Will, et al.
 * Copyright (C) 2005-2006 Atheme Project (http://atheme.org/)
 *
 * E-mail utilities
 */

#ifndef ATHEME_INC_EMAIL_H
#define ATHEME_INC_EMAIL_H 1

#include <atheme/common.h>
#include <atheme/constants.h>
#include <atheme/stdheaders.h>
#include <atheme/structures.h>

#define EMAIL_REGISTER	"register"	/* register an account/nick (verification code) */
#define EMAIL_SENDPASS	"sendpass"	/* send a password to a user (password) */
#define EMAIL_SETEMAIL	"setemail"	/* change email address (verification code) */
#define EMAIL_MEMO	"memo"		/* emailed memos (memo text) */
#define EMAIL_SETPASS	"setpass"	/* send a password change key (verification code) */

typedef void (*email_canonicalizer_fn)(char email[static (EMAILLEN + 1)], void *user_data);

struct email_canonicalizer_item
{
	email_canonicalizer_fn  func;
	void *                  user_data;
	mowgli_node_t           node;
};

int sendemail(struct user *u, struct myuser *mu, const char *type, const char *email, const char *param);
int validemail(const char *email);
stringref canonicalize_email(const char *email);
void canonicalize_email_case(char email[static (EMAILLEN + 1)], void *user_data);
void register_email_canonicalizer(email_canonicalizer_fn func, void *user_data);
void unregister_email_canonicalizer(email_canonicalizer_fn func, void *user_data);
bool email_within_limits(const char *email);

#endif /* !ATHEME_INC_EMAIL_H */
