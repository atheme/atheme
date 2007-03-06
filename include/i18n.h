/*
 * Copyright (c) 2005 William Pitcock et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This is the header which includes all of the internationalization stuff.
 *
 * $Id: i18n.h 7837 2007-03-06 00:06:49Z nenolod $
 */

#ifndef 
__ATHEME_INTL_H__

#ifdef ENABLE_NLS
# include <locale.h>
# include <libintl.h>
#else
# define _(x)     x
# define N_(x)    x
#endif

#endif
