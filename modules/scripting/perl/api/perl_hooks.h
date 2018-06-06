/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2011 Atheme Project (http://atheme.org/)
 */

#ifndef ATHEME_MOD_SCRIPTING_PERL_API_PERL_HOOKS_H
#define ATHEME_MOD_SCRIPTING_PERL_API_PERL_HOOKS_H 1

#include "atheme_perl.h"

void enable_perl_hook_handler(const char * hookname);
void disable_perl_hook_handler(const char * hookname);

#endif /* !ATHEME_MOD_SCRIPTING_PERL_API_PERL_HOOKS_H */
