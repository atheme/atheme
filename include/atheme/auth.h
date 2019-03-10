/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2009 Atheme Project (http://atheme.org/)
 *
 * Authentication.
 */

#ifndef ATHEME_INC_AUTH_H
#define ATHEME_INC_AUTH_H 1

#include <atheme/attributes.h>
#include <atheme/stdheaders.h>
#include <atheme/structures.h>

void set_password(struct myuser *mu, const char *newpassword);
bool verify_password(struct myuser *mu, const char *password) ATHEME_FATTR_WUR;

extern bool auth_module_loaded;
extern bool (*auth_user_custom)(struct myuser *mu, const char *password) ATHEME_FATTR_WUR;

#endif /* !ATHEME_INC_AUTH_H */
