/*
 * Copyright (c) 2005 William Pitcock et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This is the header which includes all of the internationalization stuff.
 *
 * $Id: i18n.h 8005 2007-03-29 22:27:21Z jilles $
 */

#ifndef __ATHEME_INTL_H__
#define __ATHEME_INTL_H__

#ifdef ENABLE_NLS
# include <locale.h>
# include <libintl.h>
# define _(String) gettext (String)
# ifdef gettext_noop
#  define N_(String) gettext_noop (String)
# else
# define N_(String) (String)
# endif
#else
# define _(x)     	(x)
# define N_(x)    	(x)
# define ngettext(s1, sn, n) ((n) == 1 ? (s1) : (sn))
#endif

#endif
