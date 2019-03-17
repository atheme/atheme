/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005 William Pitcock, et al.
 *
 * This is the header which includes all of the internationalization stuff.
 */

#ifndef ATHEME_INC_INTL_H
#define ATHEME_INC_INTL_H 1

#include <atheme/stdheaders.h>

#ifdef ENABLE_NLS
#  ifdef HAVE_LOCALE_H
#    include <locale.h>
#  endif
#  ifdef HAVE_LIBINTL_H
#    include <libintl.h>
#  endif
#  define _(String)             gettext((String))
#  ifdef gettext_noop
#    define N_(String)          gettext_noop((String))
#  else
#    define N_(String)          ((String))
#  endif
#else
#  define _(x)                  (x)
#  define N_(x)                 (x)
#  define ngettext(s1, sn, n)   ((n) == 1 ? (s1) : (sn))
#endif

#endif /* !ATHEME_INC_INTL_H */
